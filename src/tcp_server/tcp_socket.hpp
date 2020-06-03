#ifndef TCP_SERVER_TCP_SOCKET_HPP
#define TCP_SERVER_TCP_SOCKET_HPP

#include <string>
#include <vector>

namespace tcp_server_lib
{

/// A simple wrapper around socket interface provided by *nix.
class tcp_socket {
public:
    /// Type of the socket.
    enum class type
    {
        /// Uninitialized yet.
        UNKNOWN,
        /// Used for communication.
        CLIENT,
        /// Listening socket.
        SERVER,
    };

    /// ctor.
    tcp_socket();
    tcp_socket(int fd, const std::string &host, std::uint32_t port, type t);
    ~tcp_socket() = default;

    /// Move ctor.
    tcp_socket(tcp_socket&&);
    tcp_socket& operator=(tcp_socket&&);

    /// Disallow copy.
    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&) = delete;

    /// Close connection.
    void close(void);

public:
    /*
    CLIENT operations.
    */

    /// Read in at most [size] bytes.
    std::vector<char> recv(std::size_t size);

    /// Write at most [size] bytes.
    void send(const std::vector<char> &data);

    /// Connect to remote server.
    void connect(const std::string &host, std::uint32_t port);

public:
    /*
    SERVER operations.
    */

    /// Bind to address [host:port].
    void bind(const std::string &host, uint32_t port);
    void listen(std::size_t backlog);
    tcp_socket accept(void);

public:
    /*
    Helpers.
    */

    const std::string &host() const { return host_; }
    std::string &host() { return host_; }
    std::uint32_t port() const { return port_; }
    std::uint32_t &port() { return port_; }

    type get_type() const { return type_; }
    void set_type(type t) { type_ = t; }

    /// Underlying socket.
    int fd() const { return fd_; }
    int &fd() { return fd_; }

    bool operator==(const tcp_socket &rhs) const;
    bool operator!=(const tcp_socket &rhs) const;

private:
    void ensure_fd();
    void ensure_type(type t);

private:
    /// Socket.
    int fd_;

    std::string host_;
    std::uint32_t port_;
    type type_;
};

} // namespace tcp_server_lib


#endif