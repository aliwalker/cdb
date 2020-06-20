## C++11 TCP Server Library

NOTE: I've basically plagiarized the whole library from [tacopie](https://github.com/Cylix/tacopie)(under MIT License) by [Cylix](https://github.com/Cylix). I've got rid of all the Windows related code and simplified a few places. Also some of the interfaces were tweaked according to my own preferences.

The reason for this library is fairly simple. Once we can get a working TCP server and client interfaces, we can focus on the upper layer protocol.

The TCP server utilizes a thread-pool for performing the actual socket operations and a polling thread for multiplexing socket operations. This design implements the reactor pattern described by Douglas C. Schmidt. The original pattern describes a reactor pattern that exploits the system provided interface for examining the status of file descriptors(e.g. select, poll, epoll, etc.), to figure out when to perform operations on which file descriptor such that the operations won't block the current thread.

The end result is that the library provides us asynchronous proactor style interfaces for reading/writing from/to a TCP client.

### Example

See [tcp_echo_client.cpp](./tcp_echo_client.cpp) and [tcp_echo_server.cpp](./tcp_echo_server.cpp).

### Reference

- [Proactor - An Object Behavioral Pattern for Demultiplexing and Dispatching Handlers for Asynchronous Events](https://www.dre.vanderbilt.edu/~schmidt/PDF/Proactor.pdf), Irfan Pyarali, Tim Harrison, Douglas C. Schmidt, Thomas D. Jordan, 1997 (pdf 143 kB)
- [An Object Behavioral Pattern for Demultiplexing and Dispatching Handles for Synchronous Events](http://www.dre.vanderbilt.edu/~schmidt/PDF/reactor-siemens.pdf) by Douglas C. Schmidt
- [tacopie](https://github.com/Cylix/tacopie) - a multi-platform TCP Client & Server C++11 library.