#ifndef CDB_CLIENT_HPP
#define CDB_CLIENT_HPP

#include <atomic>
#include <future>
#include <mutex>
#include <map>
#include <string>
#include <vector>
#include <condition_variable>
#include "tcp_server/tcp_client.hpp"
#include "tcp_server/reactor.hpp"

namespace cdb {

/// Client class used for connecting to a cooridnator.
class cdb_client {
public:
    /// Ctor.
    cdb_client(const std::string &ip, std::uint16_t port);
    ~cdb_client() { reactor_->stop(); }

    /// Disallow copy/move explicitly.
    cdb_client(const cdb_client&) = delete;
    cdb_client(cdb_client &&) = delete;
    cdb_client &operator=(const cdb_client&) = delete;
    cdb_client &operator=(cdb_client&&) = delete;

    /// GET key value
    bool get(std::string const &key, std::string &value);

    /// SET key value
    bool set(std::string const &key, const std::string &value);

    /// DEL key
    bool del(std::string const &key);

    /// DEL key1, key2, key3...
    size_t del(std::vector<std::string> const &keys);

    /// Whether the coordinator is reachable.
    bool is_connected() const;

private:
    /// Ensure connection.
    bool ensure_connection();

    /// Send a command.
    bool send_cmd(std::string const &cmd_str, std::string &result);

    /// Called when a GET cmd response is sent. [id] was assigned by get()
    void cmd_sent(int id,
                  cdb_tcp_server::tcp_client::write_result &result);

    /// Called when a GET cmd response is received.
    void cmd_done(int id, cdb_tcp_server::tcp_client::read_result &result);

private:
    std::string decode_result(std::string const &value);
    inline std::string decode_bulk_string(std::string const &str)
    {
        size_t idx = 1;
        while (idx < str.length() && str[idx] >= '0' && str[idx] <= '9')
            idx++;

        /// FIXME: We've assumed that this is a valid bulk string.
        return str.substr(idx);
    }

    inline int decode_int(std::string const &value, size_t &idx)
    {
        size_t old_idx = idx;
        while (idx < value.length() && value[idx] >= '0' && value[idx] <= '9')
            idx++;

        try
        {
            int count = std::stoi(value.substr(old_idx, idx - old_idx));
            return count;
        }
        catch(const std::exception& e)
        {
            return -1;
        }
    }

private:
    /// Used as keys to [value_promises].
    std::atomic<int> counter = ATOMIC_VAR_INIT(0);

private:
    /// Underlying TCP client and reactor.
    std::unique_ptr<cdb_tcp_server::reactor> reactor_;
    cdb_tcp_server::tcp_client tcp_client_;

    std::mutex m_;
    std::map<int, std::promise<std::string>> value_promises_;

    std::string ip_;
    std::uint16_t port_;
};

} // namespace cdb


#endif