#include <cstdlib>
#include <ctime>
#include <iostream>
#include <chrono>
#include "errors.hpp"
#include "logger.hpp"
#include "common.hpp"
#include "command_parser.hpp"
#include "coordinator.hpp"
#include "participant.hpp"

namespace cdb {

coordinator::coordinator(coordinator_configuration &&conf)
    : conf_(std::move(conf))
    , svr_()
    , r_manager_("coordinator.log")
    , participants_() {}

void coordinator::start()
{
    if (is_started_)
        __SERVER_THROW("coordinator has already started");

    is_started_ = true;

    svr_.start(conf_.addr, 
               conf_.port,
               std::bind(&coordinator::handle_new_client, this, std::placeholders::_1));

    recovery();
    heartbeat_participants();
}

/// Starts the heartbeat mechanism in a separate worker thread.
void coordinator::async_start()
{
    if (is_started_)
        __SERVER_THROW("coordinator has already started");

    is_started_ = true;

    svr_.start(conf_.addr,
               conf_.port,
               std::bind(&coordinator::handle_new_client, this, std::placeholders::_1));
    
    recovery();
    async_heartbeat_ = std::thread(std::bind(&coordinator::heartbeat_participants, this));
}

void coordinator::recovery()
{
    /// next_id_ initialization.
    next_id_ = r_manager_.next_id();
    if (next_id_ == 0)
    {
        std::srand(std::time(nullptr));
        next_id_.store(std::rand());
        is_recovered = false;
        __CDB_LOG(debug, "random next_id");
    }
    __CDB_LOG(debug, "recovery next_id_: " + std::to_string(next_id_));

    /// Initialize participants.
    init_participants();
    handle_unfinished_records();
}

void coordinator::handle_unfinished_records()
{
    __CDB_LOG(debug, "handle_unfinished_records with size == " + std::to_string(r_manager_.records().size()));
    bool participant_dead = false;
    /// Make a copy!
    std::map<std::uint32_t, record> records{ r_manager_.records().begin(), r_manager_.records().end() };

    /// Handle all unfinished records.
    for (auto &pair : records)
    {
        __CDB_LOG(debug, "handle_unfinished_records r id: " + std::to_string(pair.second.id));

        switch (pair.second.status)
        {
        /// Abort unresolved request.
        case RECORD_UNRESOLVED:
            abort_db_request(nullptr, pair.second.id, participant_dead);
            break;
        case RECORD_ABORT:
            abort_db_request(nullptr, pair.second.id, participant_dead);
            break;
        case RECORD_COMMIT:
            commit_db_request(nullptr, pair.second.id, participant_dead);
            break;
        default:
            assert(0);
            break;
        }
    }
    __CDB_LOG(debug, "handle_unfinished_records returned");
}

void coordinator::init_participants()
{
    std::unique_lock<std::mutex> lock(participants_mutex_);

    /// Create participant clients.
    for (std::size_t i = 0; i < conf_.participant_addrs.size(); i++)
    {
        auto &ip = conf_.participant_addrs[i];
        auto port = conf_.participant_ports[i];
        init_participant(ip, port);
    }
}

/// NOTE: The caller must acquired the lock.
void coordinator::init_participant(std::string const &ip, uint16_t port)
{
    auto addr = ip + ":" + std::to_string(port);
    try
    {
        participants_[addr] = std::unique_ptr<rpc::client>{ new rpc::client{ip, port} };
        participants_[addr]->set_timeout(RPC_TIMEOUT);

        /// Examine the the P's next_id first. If its next_id is not as new as the coordinator,
        /// then the P needs a recovery.
        auto p_next_id = participants_[addr]->call("NEXT_ID").as<std::uint32_t>();

        if (!is_recovered)
        {
            participants_[addr]->call("SET_NEXT_ID", next_id_.fetch_add(0));
            goto PARTICIPANT_UP_TO_DATE;
        }

        /// The same next_id means the participant is defintely up-to-date
        if (is_recovered && p_next_id == next_id_)
            goto PARTICIPANT_UP_TO_DATE;

        auto &records = r_manager_.records();
        if (is_recovered && p_next_id + 1 == next_id_ && records.count(next_id_) && records[next_id_].status == RECORD_ABORT)
            goto PARTICIPANT_UP_TO_DATE;

        /// Needs a recovery.
        throw std::exception();
    }
    catch (std::exception &err)
    {
        __CDB_LOG(warn, "remove participant: " + std::string{err.what()});
        participants_.erase(addr);
    }

PARTICIPANT_UP_TO_DATE:
    return;
}

void coordinator::heartbeat_participants()
{
    for (;;)
    {
        const auto &addrs = conf_.participant_addrs;
        const auto &ports = conf_.participant_ports;

        for (std::size_t i = 0; i < addrs.size(); i++)
        {
            try
            {
                rpc::client client{addrs[i], ports[i]};
                client.set_timeout(RPC_TIMEOUT);
                client.call("HEARTBEAT");

                std::unique_lock<std::mutex> lock(participants_mutex_);

                std::string addr = addrs[i] + ":" + std::to_string(ports[i]);
                if (!participants_.count(addr))
                {
                    /// Add it back either because we've started the coordinator before the participants
                    /// or participant failure occured.
                    if (recover_participant(client))
                    {
                        participants_[addr] = std::unique_ptr<rpc::client>{ new rpc::client{addrs[i], ports[i]} };
                        if (participants_.size() == conf_.participant_addrs.size())
                            del_keys_.clear();

                        handle_unfinished_records();
                    }
                }
                __CDB_LOG(debug, "heartbeat: participants_.size() == " + std::to_string(participants_.size()));
            }
            catch (std::exception &e) {
                __CDB_LOG(warn, "heartbeat failed");

                std::unique_lock<std::mutex> lock(participants_mutex_);
                std::string addr = addrs[i] + ":" + std::to_string(ports[i]);
                if (participants_.count(addr))
                    participants_.erase(addr);
            }
        }

        {
            std::unique_lock<std::mutex> lock(participants_mutex_);
            participants_cond_.wait_for(lock, std::chrono::seconds(1));
        }
    }
}

/// NOTE: lock is acquired before entering this function.
bool coordinator::recover_participant(rpc::client &client)
{
    __CDB_LOG(debug, "recover_participant");

    /// If [is_recovered] is marked as false, this means the
    /// coordinator is started with no prior requests before(or started freshly).
    if (!is_recovered)
    {
        client.set_timeout(RPC_TIMEOUT);
        client.call("SET_NEXT_ID", next_id_.fetch_add(0));
        is_recovered = true;
        return true;
    }

    /// If this participant is up-to-date as the coordinator, no recovery is needed.
    auto p_next_id = client.call("NEXT_ID").as<std::uint32_t>();
    if (p_next_id == next_id_)
        return true;

    auto &records = r_manager_.records();
    if (p_next_id + 1 == next_id_ && records.count(p_next_id) && records[p_next_id].status == RECORD_UNRESOLVED)
        return true;

    // Lock is already acquired.
    while (!participants_.empty())
    {
        std::vector<char> snapshot;
        bool snapshot_ok = false;
        try
        {
            participants_.begin()->second->set_timeout(RPC_TIMEOUT);
            snapshot = std::move(participants_.begin()->second->call("GET_SNAPSHOT").as<std::vector<char>>());
            snapshot_ok = true;
        }
        catch(const std::exception& e)
        {
            participants_.erase(participants_.begin());
            snapshot_ok = false;
        }

        if (snapshot_ok)
        {
            __CDB_LOG(info, "snapshot OK");
            try
            {
                std::uint32_t id = next_id_;
                client.set_timeout(RPC_TIMEOUT);
                client.call("RECOVER", snapshot, del_keys_);
                client.call("SET_NEXT_ID", id);
                __CDB_LOG(info, "recover done");
                return true;
            }
            catch (std::exception &e)
            {
                __CDB_LOG(warn, "recover failed: " + std::string{e.what()});
                return false;
            }
        }
    }

    __CDB_LOG(warn, "recovery failed because all participants were dead");
    return false;
}

void coordinator::handle_new_client(std::shared_ptr<tcp_client> client)
{
    __CDB_LOG(info, "handle_new_client");
    try {
        /// Asynchronously read a db request from client.
        client->async_read({
            1024,   /* 1024 is big enough for most db request. */
            std::bind(&coordinator::handle_db_requests, this, client, nullptr, std::placeholders::_1)
        });
    } catch(std::runtime_error &e) {
        /// Client disconnected.
        __CDB_LOG(error, "client disconnected");
    }
}

void coordinator::handle_db_requests(std::shared_ptr<tcp_client> client, 
                                     std::shared_ptr<std::vector<char>> prev_data,
                                     tcp_client::read_result &req)
{
    if (!req.success)
    {
        /// Does not throw.
        client->disconnect(false);
        return;
    }

    __CDB_LOG(debug, "handle_db_requests with " + std::to_string(req.data.size()) + std::string{" bytes"});
    std::vector<char> data;
    if (prev_data)
        data = std::move(*prev_data);
    data.insert(data.end(), req.data.begin(), req.data.end());

    std::vector<std::unique_ptr<command> > cmds;
    bool parse_error = false;
    bool is_incomplete = false;
    std::size_t bytes_parsed = 0;

    /// Try parsing a client command.
    try
    {
        std::vector<char> data_copy{data};
        parse_db_requests(data_copy, cmds, bytes_parsed);
    }
    catch (_parse_incomplete_error &e)
    {
        is_incomplete = true;
        parse_error = true;
        __CDB_LOG(warn, "parse error: " + std::string{e.what()});
    }
    catch (std::runtime_error &e)
    {
        parse_error = true;
        __CDB_LOG(warn, "parse error: " + std::string{e.what()});
    }

    /// Dispatch.
    for (const auto &cmd : cmds)
    {
        switch (cmd->type)
        {
        case CMD_GET: {
            /// copy constructed
            get_command get_cmd{ *static_cast<get_command*>(cmd.get()) };
            handle_db_get_request(client, get_cmd);
            break;
        }

        case CMD_SET: {
            /// copy constructed
            set_command set_cmd{ *static_cast<set_command*>(cmd.get()) };
            handle_db_set_request(client, set_cmd);
            break;
        }

        case CMD_DEL: {
            /// copy constructed
            del_command del_cmd{ *static_cast<del_command*>(cmd.get()) };
            handle_db_del_request(client, del_cmd);
            break;
        }

        default:
            send_error(client);
            break;
        }
    }

    if (parse_error)
    {
        /// If this is an parse_incomplete_error, handle_command_error
        /// will re-call handle_db_requests with the previous left bytes.
        handle_command_error(client, 
                             /* Left bytes. */
                             std::shared_ptr<std::vector<char>>{ new std::vector<char>{data.begin() + bytes_parsed, data.end()} },
                             is_incomplete);
        return;
    }

    /// Recurse on itself.
    try
    {
        client->async_read({
            1024,
            std::bind(&coordinator::handle_db_requests, this, client, nullptr, std::placeholders::_1)
        });
    } 
    catch (std::runtime_error &e)
    {
        __CDB_LOG(error, "client disconnected");
    }
}

void coordinator::handle_db_get_request(std::shared_ptr<tcp_client> client, 
                                        get_command cmd)
{
    bool participant_dead = false;

    {
        /// Acquire lock.
        std::unique_lock<std::mutex> lock(participants_mutex_);

        if (participants_.empty())
        {
            /// The system cannot function.
            send_error(client);
            return;
        }

        /// GET will be served directly.
        while (!participants_.empty())
        {
            try {
                auto &p = participants_.begin()->second;
                p->set_timeout(RPC_TIMEOUT);

                auto value = p->call("GET", cmd).as<std::string>();
                send_result(client, value, nullptr);
                goto PARTICIPANT_CHECK;
            } catch (std::exception &) {
                // Remove this client.
                participants_.erase(participants_.begin());
                participant_dead = true;
                __CDB_LOG(warn, "handle_db_get_request remove participant");
            }
        }
    }

    /// If we've exhausted all dbs. The system is down.
    send_error(client);

PARTICIPANT_CHECK:
    if (participant_dead)
        participants_cond_.notify_all();
}

void coordinator::handle_db_set_request(std::shared_ptr<tcp_client> client, 
                                        set_command cmd)
{
    /// Acquire lock.
    std::unique_lock<std::mutex> lock(participants_mutex_);
    if (participants_.empty())
    {
        __CDB_LOG(warn, "participant empty");
        /// The system cannot function.
        send_error(client);
        return;
    }

    /// NOTE: If current participants_ is empty, do not increment next_id_.
    cmd.set_id(next_id_.fetch_add(1));

    /// Persist the request info.
    r_manager_.log({ RECORD_UNRESOLVED, cmd.id(), next_id_ });

    /// PREPARE
    bool prepare_ok = true;
    bool participant_dead = false;
    __CDB_LOG(info, "participants_.size() == " + std::to_string(participants_.size()));
    for (auto iter = participants_.begin(); iter != participants_.end(); )
    {
        try
        {
            __CDB_LOG(info, "prepare_set " + std::to_string(cmd.id()));
            iter->second->set_timeout(RPC_TIMEOUT);
            prepare_ok = iter->second->call("PREPARE_SET", cmd).as<bool>();
            if (!prepare_ok)
                break;
            __CDB_LOG(info, "prepare_set ok");
        } 
        catch (std::exception &e)
        {
            /// Unreachable db
            iter = participants_.erase(iter);
            participant_dead = true;
            __CDB_LOG(warn, "handle_db_set_request remove participant");
            continue;
        }
        iter++;
    }

    if (participants_.empty())
    {
        /// Since all participant is dead, no abort rpc is needed to make.
        send_error(client);
        return;
    }

    /// COMMIT
    std::string ret;
    if (prepare_ok)
        commit_db_request(client, cmd.id(), participant_dead);

    /// ABORT
    else
        abort_db_request(client, cmd.id(), participant_dead);

    if (participant_dead)
        participants_cond_.notify_all();
}

void coordinator::handle_db_del_request(std::shared_ptr<tcp_client> client, 
                                        del_command cmd)
{
    /// Acquire lock.
    std::unique_lock<std::mutex> lock(participants_mutex_);
    if (participants_.empty())
    {
        __CDB_LOG(warn, "participant empty");
        /// The system cannot function.
        send_error(client);
        return;
    }

    cmd.set_id(next_id_.fetch_add(1));
    r_manager_.log({ RECORD_UNRESOLVED, cmd.id(), next_id_ });

    /// PREPARE
    bool prepare_ok = true;
    bool participant_dead = false;
    for (auto iter = participants_.begin(); iter != participants_.end(); )
    {
        try
        {
            iter->second->set_timeout(RPC_TIMEOUT);
            prepare_ok = iter->second->call("PREPARE_DEL", cmd).as<bool>();
            if (!prepare_ok)
                break;
        } 
        catch (std::exception &e)
        {
            /// Unreachable db
            iter = participants_.erase(iter);
            participant_dead = true;
            __CDB_LOG(warn, "handle_db_del_request removed participant");
            continue;
        }
        iter++;
    }

    if (participants_.empty())
    {
        send_error(client);
        return;
    }

    /// COMMIT
    if (prepare_ok)
    {
        commit_db_request(client, cmd.id(), participant_dead);
        
        /// Record DEL cmd that'll be used to recover dead participants.
        if (participants_.size() < conf_.participant_addrs.size())
        {
            auto keys = cmd.args();
            del_keys_.insert(keys.begin(), keys.end());
        }
    }
    /// ABORT
    else
        abort_db_request(client, cmd.id(), participant_dead);
    
    if (participant_dead)
        participants_cond_.notify_all();
}

/// NOTE: lock is acquired before entering this function.
void coordinator::commit_db_request(std::shared_ptr<tcp_client> client, 
                                    std::uint32_t id, 
                                    bool &participant_dead) {
    std::string ret;

    /// Log first.
    r_manager_.log({ RECORD_COMMIT, id, next_id_ });

    /// Lock has required by caller.
    for (auto iter = participants_.begin(); iter != participants_.end(); )
    {
        try
        {
            __CDB_LOG(info, "before commit");
            iter->second->set_timeout(RPC_TIMEOUT);
            if (ret.empty())
                ret = iter->second->call("COMMIT", id).as<std::string>();
            else
                iter->second->call("COMMIT", id).as<std::string>();

            __CDB_LOG(info, "commit result: " + ret);
        }
        catch (std::exception &e)
        {
            /// Unreachable db.
            iter = participants_.erase(iter);
            participant_dead = true;
            __CDB_LOG(warn, "commit_db_request remove participants");
            continue;
        }
        iter++;
    }

    /// Log done info only if at least one participant has it committed.
    if (!participants_.empty())
    {
        r_manager_.log({ RECORD_COMMIT_DONE, id, next_id_ });
    }
    
    /// Client is nullptr when it is called from recovery().
    if (client != nullptr)
        send_result(client, ret, nullptr);
}

/// NOTE: lock is acquired before entering this function.
void coordinator::abort_db_request(std::shared_ptr<tcp_client> client, 
                                   std::uint32_t id, 
                                   bool &participant_dead) 
{
    r_manager_.log({ RECORD_ABORT, id, next_id_ });

    /// Lock has required by caller.
    for (auto iter = participants_.begin(); iter != participants_.end(); )
    {
        try
        {
            iter->second->set_timeout(RPC_TIMEOUT);
            /// If abort rpc returns false, basically it's malfunctioning.
            if (!iter->second->call("ABORT", id).as<bool>())
                throw std::exception();
        }
        catch (std::exception &e)
        {
            /// Unreachable db.
            iter = participants_.erase(iter);
            participant_dead = true;
            __CDB_LOG(warn, "abort_db_request removed participant");
            continue;
        }
        iter++;
    }

    /// Log this info only if at least one participant has this message.
    if (!participants_.empty())
    {
        r_manager_.log({ RECORD_ABORT_DONE, id, next_id_ });
    }

    /// client might be null when it comes from recovery.
    if (client != nullptr)
        send_error(client);
}

void
coordinator::parse_db_requests(std::vector<char> &data, 
                               std::vector<std::unique_ptr<command> > &ret, 
                               std::size_t &bytes_parsed)
{
    command_parser parser{std::move(data)};

    while (!parser.is_done())
    {
        auto cmd = parser.read_command();
        bytes_parsed = parser.bytes_parsed();
        ret.push_back(std::move(cmd));
    }
}

void coordinator::handle_command_error(std::shared_ptr<tcp_client> client,
                                       std::shared_ptr<std::vector<char>> data,
                                       bool is_incomplete)
{
    if (!is_incomplete)
    {
        send_error(client);
        return;
    }
    __CDB_LOG(info, "handle incomplete error");

    /// Error caused by an incomplete command.
    try
    {
        /// Try again.
        client->async_read({
            1024,
            std::bind(&coordinator::handle_db_requests, this, client, data, std::placeholders::_1)
        });
    } 
    catch(std::runtime_error &e)
    {
        __CDB_LOG(error, "client disconnected");
    }
}

void coordinator::send_error(std::shared_ptr<tcp_client> client)
{
    __CDB_LOG(info, "send_error");
    send_result(client, participant::error_string, [=](tcp_client::write_result &) {
        client->disconnect(false);
    });
}

void coordinator::send_result(std::shared_ptr<tcp_client> client, std::string const &msg, tcp_client::write_callback_t cb)
{
    try {
        client->async_write({
            std::vector<char>{msg.begin(), msg.end()},
            cb
        });
    } catch(std::runtime_error &e) {
        __CDB_LOG(error, "client disconnected");
        client->disconnect(false);
    }
}

} // namespace cdb
