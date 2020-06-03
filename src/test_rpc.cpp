#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <iostream>
#include "command_parser.hpp"
#include "rpc/client.h"
using namespace cdb;

// Single threaded. No need to synchronize.
std::vector<std::future<clmdep_msgpack::object_handle>> futures;

void make_set_rpc(set_command req, rpc::client &client)
{
    try {
        auto future = client.async_call("prepare_set", req);
        auto ok = future.get().as<bool>();
        // auto ok = client.call("prepare_set", req).as<bool>();
        if (ok)
        {
            auto future = client.async_call("commit", req.id());
            futures.emplace_back(std::move(future));
        }
        else
        {
            std::cout << "make_set_rpc - prepare failed" << std::endl;
        }
    } catch(std::exception &e) {
        std::cout << "make_set_rpc caught: " << e.what() << std::endl;
    }
}

void make_get_rpc(get_command req, rpc::client &client)
{
    try {
        std::string result = client.call("get", req).as<std::string>();
        std::cout << "> " << result << std::endl;
    } catch(std::exception &e) { std::cout << "make_get_rpc caught: " << e.what() << std::endl; }
}

void make_del_rpc(del_command req, rpc::client &client)
{
    try {
        auto future = client.async_call("prepare_del", req);
        auto ok = future.get().as<bool>();
        // auto ok = client.call("prepare_del", req).as<bool>();
        if (ok)
        {
            auto future = client.async_call("commit", req.id());
            futures.emplace_back(std::move(future));
        }
        else
        {
            std::cout << "make_del_rpc - prepare failed" << std::endl;
        }
    } catch(std::exception &e) { std::cout << "make_del_rpc caught: " << e.what() << std::endl; }
}

/// A script for testing RPC functionalities.
int main()
{
    /// WARN:
    /// We can have at most two concurrent/parallel update requests to be safe,
    /// since we're using 2 callback workers by default.
    /// Now we're sending 3 update requests concurrently. Note that the participant
    /// will probably crash.
    set_command set_cmd3{"CS06142", "Cloud Computing 20"};
    set_command set_cmd{"CS06142", "Cloud Computing"};
    del_command del_cmd{std::vector<std::string>{"CS06142", "non-existing"}};

    set_cmd3.set_id(2);
    set_cmd.set_id(0);
    del_cmd.set_id(1);
    
    get_command get_cmd{"CS06142"};
    get_command get_cmd2{"non_existing"};

    rpc::client client{"127.0.0.1", 8001};
    client.set_timeout(200);

    make_set_rpc(set_cmd, client);
    // make_set_rpc(set_req3, client);
    make_del_rpc(del_cmd, client);

    for (auto &future : futures)
    {
        try {
            auto ret = future.get().as<std::string>();
            std::cout << "> " << ret << std::endl;
        } catch(std::exception &e) {
            std::cout << "caught: " << e.what() << std::endl;
        }
    }

    // make_get_rpc(get_req, client);
    // make_get_rpc(get_req2, client);

    for (;;) {}
}