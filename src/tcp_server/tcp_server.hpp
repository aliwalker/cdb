#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include <atomic>
#include <list>
#include <string>
#include <functional>
#include "tcp_socket.hpp"
#include "tcp_client.hpp"
#include "reactor.hpp"

namespace tcp_server_lib
{

class tcp_server
{
public:
    tcp_server();
    /// Rule of 5.
    ~tcp_server();
    tcp_server(const tcp_server&) = delete;
    tcp_server &operator=(const tcp_server&) = delete;
    tcp_server(tcp_server&&);
    tcp_server &operator=(tcp_server&&);

public:
    typedef std::function<void(std::shared_ptr<tcp_client>)> on_new_connection_cb_t;
    /// Start TCP server.
    void start(const std::string &host, std::uint32_t port, const on_new_connection_cb_t &cb);

    /// Stop TCP server.
    void stop();

    /* GETTER. */
    tcp_socket &socket() { return socket_; }
    const tcp_socket &socket() const { return socket_; }

private:
    /// Callback when we can call socket_.accept() without blocking.
    void on_read_available(int fd);

    /// Callback when tcp_client::disconnect() is called.
    void on_client_disconnect(std::shared_ptr<tcp_client>, const tcp_client::on_disconnection_t &user_cb = nullptr);

private:
    reactor *reactor_;

    /// Indicator.
    std::atomic<bool> is_running_ = ATOMIC_VAR_INIT(false);

    /// Underlying listening socket.
    tcp_socket socket_;

    /// Underlying bookkeeping container.
    /// NOTE: We've chose to use shared_ptr because the [on_new_connection_cb_]
    /// can obtain the pointer to a client and use it later. By using unique_ptr,
    /// there'd be a possibility that the user of tcp_server accidentally access
    /// a dangling ptr.
    /// TODO: Change this to std::set
    std::list<std::shared_ptr<tcp_client>> clients_;

    /// Thread safety.
    std::mutex clients_mutex_;

    /// Callback when a new TCP connection is established.
    on_new_connection_cb_t on_new_connection_cb_;
};

} // namespace tcp_server_lib

#endif