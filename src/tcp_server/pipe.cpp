#include "exceptions.hpp"
#include "pipe.hpp"

namespace tcp_server_lib {

pipe::pipe()
    : fds{-1, -1}
{
    if (::pipe(fds) == -1)
        __TCP_THROW("error pipe()");
}

pipe::~pipe()
{
    if (fds[0] != -1)
        close(fds[0]);
    if (fds[1] != -1)
        close(fds[1]);
}

int pipe::get_read_fd() const
{
    return fds[0];
}

int pipe::get_write_fd() const
{
    return fds[1];
}

void pipe::notify()
{
    (void)::write(fds[1], " ", 1);
}

void pipe::clear_pipe()
{
    char buffer[256];
    (void)::read(fds[0], buffer, sizeof(buffer));
}

} // namespace tcp_server_lib
