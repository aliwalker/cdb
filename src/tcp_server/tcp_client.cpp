#include "exceptions.hpp"
#include "tcp_client.hpp"

namespace tcp_server_lib {

tcp_client::tcp_client()
    : reactor_(get_default_reactor())
    , on_disconnection_(nullptr) {}

tcp_client::~tcp_client()
{
    disconnect(false);
}

tcp_client::tcp_client(tcp_socket&& socket)
    : reactor_(get_default_reactor())
    , socket_(std::move(socket))
    , is_connected_(true)
    , on_disconnection_(nullptr) {}

void tcp_client::connect(const std::string &host, std::uint32_t port)
{
    if (is_connected_)
        __TCP_THROW("tcp_client is already connected");

    try {
        socket_.connect(host, port);
        reactor_->register_fd(socket_.fd());
    } catch (const std::runtime_error &e) {
        socket_.close();
        throw e;
    }

    is_connected_ = true;
}

void tcp_client::disconnect(bool wait)
{
    if (!is_connected_)
        return;

    is_connected_ = false;
    clear_read_reqs();
    clear_write_reqs();

    // Unregister from reactor_.
    reactor_->unregister(socket_.fd());
    socket_.close();
    if (wait)
        reactor_->wait_on_removal_cond(socket_.fd());

    // User-provided cb.
    if (on_disconnection_) {
        on_disconnection_();
    }
}

void tcp_client::clear_read_reqs()
{
    std::lock_guard<std::mutex> lock(read_request_mutex_);

    std::queue<read_request> empty;
    std::swap(read_requests_, empty);
}

void tcp_client::clear_write_reqs()
{
    std::lock_guard<std::mutex> lock(write_request_mutex_);

    std::queue<write_request> empty;
    std::swap(write_requests_, empty);
}

void tcp_client::async_read(const read_request &req)
{
    std::lock_guard<std::mutex> lock(read_request_mutex_);

    if (is_connected_)
    {
        // Always re-register socket_ to reactor_.
        reactor_->set_rd_callback(
            socket_.fd(), 
            std::bind(&tcp_client::on_read_available, this, std::placeholders::_1/* The fd */));

        // Append the info.
        read_requests_.push(req);
    }
    else
    {
        __TCP_THROW("tcp_client is not connected");
    }
}

/// Same as async_read.
void tcp_client::async_write(const write_request &req)
{
    std::lock_guard<std::mutex> lock(write_request_mutex_);

    if (is_connected_)
    {
        reactor_->set_wr_callback(
            socket_.fd(),
            std::bind(&tcp_client::on_write_available, this, std::placeholders::_1)
        );

        write_requests_.push(req);
    }
    else
    {
        __TCP_THROW("tcp_client is not connected");
    }
}

/// [fd] is always socket_.fd().
void tcp_client::on_read_available(int)
{
    read_result result;
    auto user_cb = do_read(result);

    // Disconnect if once operation failed.
    if (!result.success)
    {
        // TODO: add log
        disconnect(false);
    }

    if (user_cb)
        user_cb(result);
}

/// Same as above.
void tcp_client::on_write_available(int)
{
    write_result result;
    auto user_cb = do_write(result);

    // Disconnect if once operation failed.
    if (!result.success)
    {
        // TODO: add log.
        disconnect(false);
    }

    if (user_cb)
        user_cb(result);
}

tcp_client::read_callback_t tcp_client::do_read(tcp_client::read_result &result)
{
    std::lock_guard<std::mutex> lock(read_request_mutex_);

    // Return if no request is pending.
    if (read_requests_.empty()) return nullptr;

    // Serve request.
    auto &req = read_requests_.front();
    auto cb = req.cb;

    try {
        result.data = socket_.recv(req.size);
        result.success = true;
    } catch (const std::runtime_error&) {
        result.success = false;
    }

    read_requests_.pop();
    
    // Basically, when no request is pending, we don't want
    // reactor_ to poll socket_
    if (read_requests_.empty())
        reactor_->set_rd_callback(socket_.fd(), nullptr);

    return cb;
}

tcp_client::write_callback_t tcp_client::do_write(tcp_client::write_result &result)
{
    std::lock_guard<std::mutex> lock(write_request_mutex_);

    if (write_requests_.empty())    return nullptr;

    // Serve request.
    auto &req = write_requests_.front();
    auto cb = req.cb;

    try {
        socket_.send(req.data);
        result.size = req.data.size();
        result.success = true;
    } catch (const std::runtime_error&) {
        result.success = false;
    }

    write_requests_.pop();
    // Basically, when no request is pending, we don't want
    // reactor_ to poll socket_
    if (write_requests_.empty())
        reactor_->set_wr_callback(socket_.fd(), nullptr);

    return cb;
}

bool tcp_client::operator==(const tcp_client &rhs) const
{
    return socket_ == rhs.socket_;
}

bool tcp_client::operator!=(const tcp_client &rhs) const
{
    return socket_ != rhs.socket_;
}

}   // namespace tcp_server_lib