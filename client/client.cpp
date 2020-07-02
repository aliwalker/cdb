#include <iostream>
#include <sstream>
#include "client.hpp"
#include "command_parser.hpp"

namespace cdb {

cdb_client::cdb_client(std::string const &ip, std::uint16_t port)
    : reactor_(new cdb_tcp_server::reactor{1})
    , tcp_client_(reactor_.get())
    , ip_(ip), port_(port) {}

bool cdb_client::ensure_connection()
{
    if (!tcp_client_.is_connected())
    {
        try
        {
            tcp_client_.connect(ip_, port_);
        }
        catch (std::exception &e)
        {
            return false;
        }
    }

    return true;
}

std::string cdb_client::decode_result(std::string const &value)
{
    if (value == "+OK\r\n")
        return value;

    if (value == "-ERROR\r\n")
        return "";

    if (!value.empty() && value[0] == ':')
        return value;

    if (value.empty() || value[0] != '*')
        return "";

    size_t idx = 1;

    int count = decode_int(value, idx);
    if (count <= 0) return "";
    
    std::stringstream ss;
    while (idx < value.length())
    {
        if (value.substr(idx, 2) != command_parser::separator)
            return "";

        idx += 2;
        if (idx >= value.length())   break;
        if (value[idx] != '$')  return "";

        idx += 1;
        int size = decode_int(value, idx);
        if (size <= 0)  return "";

        if (idx >= value.length() || value.substr(idx, 2) != command_parser::separator)
            return "";

        idx += 2;
        ss << value.substr(idx, size) << " ";
        idx += size;
    }

    auto ret = ss.str();
    return ret.substr(0, ret.length() - 1);
}

/// Sync GET
bool cdb_client::get(std::string const &key, std::string &value)
{
    if (!ensure_connection())
        return false;

    std::string cmd_str = command_parser::encode_get(key);
    return send_cmd(cmd_str, value);
}

/// Sync SET
bool cdb_client::set(std::string const &key, const std::string &value)
{
    if (!ensure_connection())
        return false;

    std::string cmd_str = command_parser::encode_set(key, value);
    std::string result;
    return send_cmd(cmd_str, result);
}

/// Sync DEL
bool cdb_client::del(std::string const &key)
{
    return del(std::vector<std::string>{key});
}

/// Sync DEL
size_t cdb_client::del(std::vector<std::string> const &keys)
{
    if (!ensure_connection())
        return false;

    std::string cmd_str = command_parser::encode_del(keys);
    std::string result;
    return send_cmd(cmd_str, result);
}

bool cdb_client::send_cmd(std::string const &cmd_str, std::string &result)
{
    int id = counter.fetch_add(1);

    /// Adds a promise to the map.
    {
        std::unique_lock<std::mutex> lock(m_);
        value_promises_.insert(std::make_pair(id, std::promise<std::string>{}));
    }
    auto value_future = value_promises_[id].get_future();

    try
    {
        tcp_client_.async_write({
            { cmd_str.begin(), cmd_str.end() },
            std::bind(&cdb_client::cmd_sent, this, id, std::placeholders::_1)
        });
    }
    catch(std::exception &e)
    {
        return false;
    }

    /// Synchronously wait.
    result = value_future.get();
    
    /// Remove the promise from the map.
    {
        std::unique_lock<std::mutex> lock(m_);
        value_promises_.erase(id);
    }

    return !result.empty();
}

/// NOTE: value_promises_[id] will only be accessed by 1 thread, so no lock is required.
void cdb_client::cmd_sent(int id,
                          cdb_tcp_server::tcp_client::write_result &result)
{
    assert(value_promises_.count(id) && "bug encountered with value_promises");

    if (!result.success)
    {
        value_promises_[id].set_value("");
        return;
    }

    try
    {
        tcp_client_.async_read({
            1024,
            std::bind(&cdb_client::cmd_done, this, id, std::placeholders::_1)
        });
    }
    catch(const std::exception& e)
    {
        value_promises_[id].set_value("");
    }
}

/// NOTE: value_promises_[id] will only be accessed by 1 thread, so no lock is required.
void cdb_client::cmd_done(int id, cdb_tcp_server::tcp_client::read_result &result)
{
    assert(value_promises_.count(id) && "bug encountered with value_promises");

    /// Empty value means failure.
    if (!result.success)
        value_promises_[id].set_value("");
    else
        value_promises_[id].set_value(decode_result(std::string{result.data.begin(), result.data.end()}));
}

} // namespace cdb
