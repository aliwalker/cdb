#include <signal.h>
#include <condition_variable>
#include <mutex>
#include <string>
#include <iostream>
#include "tcp_server/tcp_client.hpp"
using tcp_server_lib::tcp_client;

std::mutex mutex;
std::condition_variable cond;

void read_reply(tcp_client &client, tcp_client::write_result &result);
void on_read_reply_done(tcp_client &client, tcp_client::read_result &result)
{
    if (!result.success)
    {
        std::cout << "connection closed" << std::endl;
        return;
    }

    std::string ret{result.data.begin(), result.data.end()};
    std::cout << "> " << ret << std::endl;

    tcp_client::write_result dummy { true, 0 };
    read_reply(client, dummy);
}

void read_reply(tcp_client &client, tcp_client::write_result &result)
{
    if (result.success)
    {
        try {
            client.async_read({
                1024,
                std::bind(&on_read_reply_done, std::ref(client), std::placeholders::_1)
            });
        } catch (std::runtime_error &e)
        {
            std::cout << "read_reply caught: " << e.what() << std::endl;
        }
    }
    else
        std::cout << "connection closed" << std::endl;
}

int main()
{
    std::string del_cmd{"*3\r\n$3\r\nDEL\r\n$7\r\nCS06142\r\n$5\r\nCS162\r\n"};
    std::string get_cmd{"*2\r\n$3\r\nGET\r\n$7\r\nCS06142\r\n"};
    std::string set_cmd{"*4\r\n$3\r\nSET\r\n$7\r\nCS06142\r\n$5\r\nCloud\r\n$9\r\nComputing\r\n"};

    tcp_client client;
    try 
    {
        client.connect("localhost", 8080);
        client.on_disconnection() = [&]() { exit(0); };

        std::vector<char> cmds_part_1{set_cmd.begin(), set_cmd.end()};
        cmds_part_1.insert(cmds_part_1.end(), get_cmd.begin(), get_cmd.begin() +20);
        std::vector<char> cmds_part_2{get_cmd.begin() + 20, get_cmd.end()};

        // Send a piece of incomplete cmd.
        client.async_write({
            cmds_part_1,
            nullptr
        });

        // Sleep for 400ms, then send the remaining cmd.
        std::this_thread::sleep_for(std::chrono::microseconds(400));
        client.async_write({
            cmds_part_2,
            std::bind(&read_reply, std::ref(client), std::placeholders::_1)
        });
    }
    catch (std::runtime_error &e)
    {
        std::cout << "connection closed" << std::endl;
    }

    for(;;) {}
}