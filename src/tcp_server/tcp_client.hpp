#ifndef TCP_SERVER_TCP_CLIENT_HPP
#define TCP_SERVER_TCP_CLIENT_HPP

#include <atomic>
#include <functional>
#include <queue>
#include <vector>
#include "reactor.hpp"
#include "tcp_socket.hpp"

namespace tcp_server_lib {

/// Proactor TCP client class that utilizes reactor.
class tcp_client {
public:
    tcp_client();
    /// Rule of 5.
    ~tcp_client();
    /// Explicitly disallow move/copy.
    tcp_client(const tcp_client&) = delete;
    tcp_client(tcp_socket&& socket);
    tcp_client(tcp_client&&) = delete;
    tcp_client &operator=(const tcp_client&) = delete;
    tcp_client &operator=(tcp_client&&) = delete;

public:

    /*
    PUBLIC interfaces.
    */

    /// Info struct passed to async_read callback.
    struct read_result {
        /// Indicator.
        bool success;

        /// Actual data.
        std::vector<char> data;
    };
    /// Info struct passed to async_write callback.
    struct write_result {
        /// Indicator.
        bool success;

        /// Actual number of bytes written.
        std::size_t size;
    };

    typedef std::function<void(read_result&)> read_callback_t;
    typedef std::function<void(write_result&)> write_callback_t;

    /// Bookkeeping info for read request.
    struct read_request
    {
        /// Size in bytes.
        std::size_t size;

        /// Callback when read is done.
        read_callback_t cb;
    };
    /// Bookkeeping info for write request.
    struct write_request
    {
        /// Data to write.
        std::vector<char> data;

        /// Callback when write is done.
        write_callback_t cb;
    };

    const std::string &host() const { return socket_.host(); }
    std::uint32_t port() const { return socket_.port(); }

    void connect(const std::string &host, std::uint32_t port);
    bool is_connected() const { return is_connected_; }

    /// Asynchronously read without blocking the current thread.
    void async_read(const read_request&);
    /// Asynchronously write without blocking the current thread.
    void async_write(const write_request&);

    /// Shut down the connection actively. If [wait] is true, the call
    /// blocks until disconnection completes.
    void disconnect(bool wait);

    /// GETTER.
    tcp_socket &socket() { return socket_; }
    const tcp_socket &socket() const { return socket_; }

    typedef std::function<void()> on_disconnection_t;

    on_disconnection_t &on_disconnection() { return on_disconnection_; }
    const on_disconnection_t &on_disconnection() const { return on_disconnection_; }

public:
    /*
    Needed cuz we need to find within a container.
    */
    bool operator==(const tcp_client &rhs) const;
    bool operator!=(const tcp_client &rhs) const;

private:
    /*
    Callbacks used by reactor, telling us we can read/write [socket_] without blocking.
    NOTE: [fd] param is always socket_.fd().
    */
    void on_read_available(int);
    void on_write_available(int);

    /*
    Read/write the from/to underlying [socket_] used by the above two callbacks.
    */
    read_callback_t do_read(read_result& result);
    write_callback_t do_write(write_result &result);

private:
    reactor *reactor_;

    /// Underlying socket.
    tcp_socket socket_;

    /// Indicator.
    std::atomic<bool> is_connected_ = ATOMIC_VAR_INIT(false);

    /// Pending requests to the underlying [socket_].
    std::queue<read_request> read_requests_;
    std::queue<write_request> write_requests_;

    void clear_read_reqs();
    void clear_write_reqs();

    /// Thread safety.
    std::mutex read_request_mutex_;
    std::mutex write_request_mutex_;

    /// Called when this client is disconnected.
    on_disconnection_t on_disconnection_;
};

}
#endif