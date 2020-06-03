#include <iostream>
#include "errors.hpp"
#include "record.hpp"

namespace cdb {
/*
record.
*/
std::vector<unsigned char> record::to_binary() const
{
    std::vector<unsigned char> binary;

    /// NOTE: we're using a really ad-hoc format.
    encode_uint8_t(binary, status);
    encode_uint32_t(binary, id);
    encode_uint32_t(binary, next_id);
    return binary;
}

record record::parse(const std::vector<unsigned char> &binary)
{
    std::size_t idx = 0;

    std::uint8_t status = decode_uint8_t(binary, idx);
    std::uint32_t id = decode_uint32_t(binary, idx);
    std::uint32_t next_id = decode_uint32_t(binary, idx);

    return {status, id, next_id};
}

/// Helpers.
/// TODO: remove these once we have encode_bytes(size, void*);

void record::encode_uint32_t(std::vector<unsigned char> &binary, std::uint32_t val)
{
    /// Little-endian
    binary.push_back(val & 0xFF);
    binary.push_back((val >> 8) & 0xFF);
    binary.push_back((val >> 16) & 0xFF);
    binary.push_back((val >> 24) & 0xFF);
}

void record::encode_uint8_t(std::vector<unsigned char> &binary, std::uint8_t val)
{
    /// Little-endian
    binary.push_back(val & 0xFF);
}

std::uint8_t record::decode_uint8_t(const std::vector<unsigned char> &binary, std::size_t &idx)
{
    if (idx > binary.size())
        __RECORD_THROW("not enough bytes");

    std::uint8_t ret = 0;
    ret |= binary[idx];
    idx += 1;

    return ret;
}

std::uint32_t record::decode_uint32_t(const std::vector<unsigned char> &binary, std::size_t &idx)
{
    if (idx + 4 > binary.size())
        __RECORD_THROW("not enough bytes");

    std::uint32_t ret = 0;
    ret |= binary[idx];
    ret |= (binary[idx + 1] << 8);
    ret |= (binary[idx + 2] << 16);
    ret |= (binary[idx + 3] << 24);
    idx += 4;

    return ret;
}

/*
record_manager
*/

record_manager::record_manager(std::string const &file_name)
    : file_name_(file_name)
    , file_(file_name, std::fstream::app | std::fstream::binary | std::fstream::in | std::fstream::out)
    , cmd_file_("cmd_" + file_name, std::fstream::app | std::fstream::binary | std::fstream::in | std::fstream::out)
{
    if (!file_.is_open())
        __RECORD_THROW("cannot open log '" + file_name + "'");

    if (!cmd_file_.is_open())
        __RECORD_THROW("cannot open command log file");

    file_.seekg(std::ios::beg);
    cmd_file_.seekg(std::ios::beg);

    init_records();
}

void record_manager::log(const record &r)
{
    auto binary = r.to_binary();

    try 
    {
        /// Persist to disk.
        file_.write(reinterpret_cast<const char*>(binary.data()), binary.size());
        file_.flush();
        std::cout << "persist record " << (int)r.status << " " << r.id << " " << r.next_id << std::endl;

        if (r.status == RECORD_ABORT_DONE || r.status == RECORD_COMMIT_DONE)
        {
            records_.erase(r.id);

            /// Clear the contents of the log.
            if (records_.empty())
            {
                file_.close();
                cmd_file_.close();

                /// Haha! I don't know a more canonical way to delete the file_content.
                {
                    std::ofstream tmp{ file_name_, std::ios::out | std::ios::trunc };
                    std::ofstream tmp2{ "cmd_" + file_name_, std::ios::out | std::ios::trunc };
                }

                file_.open(file_name_, std::fstream::app | std::fstream::binary | std::fstream::in | std::fstream::out);
                cmd_file_.open("cmd_" + file_name_, std::fstream::app | std::fstream::binary | std::fstream::in | std::fstream::out);
                file_.seekg(std::ios::beg);
                cmd_file_.seekg(std::ios::beg);

                // Preserve at least one record to help finding next_id
                file_.write(reinterpret_cast<const char*>(binary.data()), binary.size());
                file_.flush();
            }
        }
        else
            records_[r.id] = r;
    }
    catch (std::exception &e)
    {
        __RECORD_THROW("failed logging record");
    }
}

/// Used by the 
void record_manager::log(const command *cmd)
{
    if (!cmd)
        __RECORD_THROW("cmd is nullptr");

    std::vector<unsigned char> binary;
    /// FIXME: yea, this encoding should be here.
    if (cmd->type == CMD_SET)
    {
        /// Type tag.
        const auto set_cmd = static_cast<const set_command*>(cmd);
        record::encode_uint8_t(binary, CMD_SET);

        /// Key.
        record::encode_uint32_t(binary, set_cmd->key().size());
        binary.insert(binary.end(), set_cmd->key().begin(), set_cmd->key().end());

        /// Value.
        record::encode_uint32_t(binary, set_cmd->value().size());
        binary.insert(binary.end(), set_cmd->value().begin(), set_cmd->value().end());
    }
    else if (cmd->type == CMD_DEL)
    {
        const auto del_cmd = static_cast<const del_command*>(cmd);
        record::encode_uint8_t(binary, CMD_DEL);

        for (const auto &arg : del_cmd->args())
        {
            record::encode_uint32_t(binary, arg.size());
            binary.insert(binary.end(), arg.begin(), arg.end());
        }
    }
    else
        __RECORD_THROW("encoding non SET/DEL command");

    try 
    {
        cmd_file_.write(reinterpret_cast<const char*>(binary.data()), binary.size());
        cmd_file_.flush();
        std::cout << "persist command " << cmd->id() << std::endl;

        /// No need to keep it in memory since the participant already has one.
    }
    catch (std::exception &e)
    {
        __RECORD_THROW("failed writing a command");
    }
}

void record_manager::init_records()
{
    /// Read all records in memory at once.
    std::vector<unsigned char> data(std::istreambuf_iterator<char>(file_), {});
    std::size_t start = 0;

    std::cout << "init_records with data.size() == " << data.size() << std::endl;

    while (start < data.size())
    {
        auto r = record::parse(std::vector<unsigned char>{ data.begin() + start, /* Begin */
                                                data.begin() + start + record::record_size} /* End */);
        std::cout << "init_records r " << (int)r.status << " " << r.id << " " << r.next_id << std::endl;
        next_id_ = r.next_id;
        /// Ignore DONE record.
        if (r.status == RECORD_ABORT_DONE || r.status == RECORD_COMMIT_DONE)
            records_.erase(r.id);
        else
            /// This will override the previous record.
            records_[r.id] = r;
        start += record::record_size;
    }

    /// NOTE: Always init records before commands!
    std::vector<unsigned char> cmd_data(std::istreambuf_iterator<char>(file_), {});
    start = 0;
    /// FIXME: Yea, pretty messy. We could've used the msgpack for this purpose.
    /// However, this does not seem to be an option.
    while (start < cmd_data.size())
    {
        auto type = record::decode_uint8_t(cmd_data, start);
        std::unique_ptr<command> cmd_obj = nullptr;

        /// Decode a set_command.
        if (type == CMD_SET)
        {
            auto id = record::decode_uint32_t(cmd_data, start);

            auto key_size = record::decode_uint32_t(cmd_data, start);
            std::string key{cmd_data.begin() + start, cmd_data.begin() + start + key_size};
            start += key_size;

            auto val_size = record::decode_uint32_t(cmd_data, start);
            std::string value{cmd_data.begin() + start, cmd_data.begin() + start + val_size};
            start += val_size;

            cmd_obj.reset(new set_command{std::move(key), std::move(value)});
            cmd_obj->set_id(id);
        }
        else if (type == CMD_DEL)
        {
            auto id = record::decode_uint32_t(cmd_data, start);
            auto num_args = record::decode_uint32_t(cmd_data, start);
            std::vector<std::string> args(num_args);

            while (num_args--)
            {
                auto arg_size = record::decode_uint32_t(cmd_data, start);
                std::string arg{ cmd_data.begin() + start, cmd_data.begin() + start + arg_size };
                start += arg_size;

                args.push_back(arg);
            }

            cmd_obj.reset(new del_command{std::move(args)});
            cmd_obj->set_id(id);
        }
        else
            __RECORD_THROW("unexpected command type");

        /// We're only interested in those that are not DONE.
        if (records_.count(cmd_obj->id()))
            cmds_[cmd_obj->id()] = std::move(cmd_obj);
    }
}

} // namespace cdb
