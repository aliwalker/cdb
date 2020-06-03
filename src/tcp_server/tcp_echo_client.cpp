#include <atomic>
#include <functional>
#include <iostream>
#include <condition_variable>
#include <vector>
#include <string>
#include "tcp_client.hpp"

using tcp_server_lib::tcp_client;

std::atomic<std::size_t> count = ATOMIC_VAR_INIT(0);
std::condition_variable cond;
std::mutex mutex;

static void on_read_completion(tcp_client &, tcp_client::read_result &result)
{
    std::cout << "recv'ed from echo server" << std::endl;
    std::cout << std::string{result.data.begin(), result.data.end()} << std::endl << std::endl;

    // c.disconnect(true);
}

static void on_write_completion(tcp_client &client, tcp_client::write_result &result)
{
    std::cout << "sent " << result.size << " bytes to server" << std::endl;
    try {
        client.async_read({
            result.size,
            std::bind(&on_read_completion, std::ref(client), std::placeholders::_1)
        });
    } catch(std::runtime_error &e) { std::cout << "on_write_completion caught error: " << e.what() << std::endl; }
}

std::vector<std::shared_ptr<tcp_client>> batch_client()
{
    std::vector<std::shared_ptr<tcp_client>> clients;
    std::vector<std::string> lines{ 
        {"some random string 1."}, 
        {"complete garbled"},
        {"season system developer"}
    };

    clients.reserve(10000);
    for (int i = 0; i < 10000; i++)
    {
        try {
            std::shared_ptr<tcp_client> client{new tcp_client};
            client->connect("localhost", 3001);
            client->on_disconnection() = [&]() {
                std::cout << "client disconnected" << std::endl;
                count.fetch_add(1);
                std::cout << "count: " << count << std::endl;
                cond.notify_all();
            };

            for (auto &line : lines)
                client->async_write({
                    std::vector<char>{line.begin(), line.end()},    /* data */
                    std::bind(&on_write_completion, std::ref(*client), std::placeholders::_1)
                });

            clients.push_back(client);
        } catch(std::runtime_error &e) {
            count.fetch_add(1);
            std::cout << "Caught error: " << e.what() << std::endl;
        }
    }

    return clients;
}

int main()
{
    auto clients = batch_client();

    // Wait for ^C
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock, [&]() { return count == 10000; });

    exit(0);
}