#include <algorithm>
#include "exceptions.hpp"
#include "tcp_server.hpp"
#include "common.hpp"

namespace tcp_server_lib
{

tcp_server::tcp_server()
    : reactor_(get_default_reactor())
    , on_new_connection_cb_(nullptr) {}

tcp_server::~tcp_server()
{
    stop();
}

void tcp_server::start(const std::string &host, std::uint32_t port, const on_new_connection_cb_t &cb)
{
    if (is_running_)
        __TCP_THROW("tcp_server is already running");

    socket_.bind(host, port);
    socket_.listen(TCP_SERVER_BACK_LOG);

    // Register the listening fd.
    reactor_->register_fd(socket_.fd(), std::bind(&tcp_server::on_read_available, this, std::placeholders::_1));
    on_new_connection_cb_ = cb;

    is_running_ = true;
}

// The fd will always be the listening fd.
void tcp_server::on_read_available(int)
{
    try
    {
        // NOTE: we're limited to C++11 here, so make_shared is not available.
        std::shared_ptr<tcp_client> client{new tcp_client(socket_.accept())};

        if (on_new_connection_cb_)
            on_new_connection_cb_(client);

        // If client::on_disconnection_ is set by on_new_connection_cb_,
        // make sure we'll call it too.
        if (client->on_disconnection())
            client->on_disconnection() = std::bind(
                &tcp_server::on_client_disconnect, 
                this, 
                client,
                client->on_disconnection());
        else
            client->on_disconnection() = std::bind(
                &tcp_server::on_client_disconnect, 
                this, 
                client, 
                nullptr);

        // Append the client to internal client list.
        clients_.emplace_back(client);
    }
    catch(const std::exception& e)
    {
        // TODO: add log.
        stop();
    }
}

void tcp_server::stop()
{
    if (!is_running_)
        return;

    is_running_ = false;
    reactor_->unregister(socket_.fd());
    socket_.close();

    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto &c : clients_)
        c->disconnect(true);

    clients_.clear();
}

/// Called by tcp_client::diconnect(). This method simply
/// remove [client] from the underlying bookkeeping container.
void 
tcp_server::on_client_disconnect(std::shared_ptr<tcp_client> client, 
                                 const tcp_client::on_disconnection_t &user_cb)
{
    if (!is_running_)   return;

    // Call user-provided callback before actually remove it.
    if (user_cb)
        user_cb();

    /* Erase from the list. */
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = std::find(clients_.begin(), clients_.end(), client);

    if (it != clients_.end())
        clients_.erase(it);
}

} // namespace tcp_server_lib
