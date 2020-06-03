#ifndef TCP_SERVER_PIPE_HPP
#define TCP_SERVER_PIPE_HPP

#include <unistd.h>

namespace tcp_server_lib {

/// Wrapper around *nix pipe.
class pipe {
public:
    pipe();
    ~pipe();

    pipe(const pipe &) = delete;
    pipe& operator=(const pipe&) = delete;

    int get_read_fd() const;
    int get_write_fd() const;

    void notify();
    void clear_pipe();

private:
    int fds[2];
};
}   // namespace tcp_server_lib

#endif