#ifndef TCP_SERVER_REACTOR_HPP
#define TCP_SERVER_REACTOR_HPP

#include <atomic>
#include <functional>
#include <unordered_map>
#include <vector>
#include "thread_pool.hpp"
#include "pipe.hpp"

namespace tcp_server_lib {

/// Forward declaration.
class reactor;

/// Singleton.
reactor *get_default_reactor();

/// Reactor class deviates a bit from the original
/// paper's description. The reactor consists of a
/// single polling thread that calls multiplexing I/O
/// syscall and a thread pool for dispatching handlers.
class reactor
{
public:
    /// Constructors.
    reactor(std::size_t thread_num);
    ~reactor();

    reactor(const reactor &) = delete;
    reactor& operator=(const reactor&) = delete;

public:
    /*
    PUBLIC interface.
    */

    /// Event handler func type. The parameter is an fd that can be
    /// read/write without blocking the thread.
    typedef std::function<void(int)> event_handler_t;

    /// Change the number of underlying thread workers.
    void set_thread_num(std::size_t thread_num);

    /// Register [fd] to the reactor. 
    void register_fd(int fd,
                     const event_handler_t &rd_callback = nullptr,
                     const event_handler_t &wr_callback = nullptr);

    /// Return the number of register fds.
    std::size_t register_num();

    /// Update callback on read available.
    void set_rd_callback(int fd, const event_handler_t &cb);

    /// Update callback on write available.
    void set_wr_callback(int fd, const event_handler_t &cb);

    /// Unregister an [fd].
    void unregister(int fd);

    /// Wait until [fd] is removed from [tracked_fds_].
    void wait_on_removal_cond(int fd);

    /// Shutdown the whole reactor.
    void stop();

    /*
    PRIVATE implementation is below.
    */
private:
    /// All info needed to track a registered fd.
    struct fd_info {
        std::atomic<bool> marked_untrack = ATOMIC_VAR_INIT(false);
        std::atomic<bool> is_executing_rd_cb = ATOMIC_VAR_INIT(false);
        std::atomic<bool> is_executing_wr_cb = ATOMIC_VAR_INIT(false);
        event_handler_t rd_callback;
        event_handler_t wr_callback;

        fd_info()
            : rd_callback(nullptr)
            , wr_callback(nullptr) {}
    };

    /// Uniform multiplexing I/O syscall.
    void poll();

    /// Initialize fds used by [select]. Return the correct
    /// nfds.
    int init_select_fds();

    /// Dispatch handlers.
    void dispatch();
    void dispatch_select();
    void dispatch_read(int fd);
    void dispatch_write(int fd);

private:
    /// fd and callback info map.
    std::unordered_map<int, fd_info> tracked_fds_;
    std::mutex tracked_fds_mutex_;

    /// Workers.
    std::thread poll_worker_;
    thread_pool callback_workers_;

    /// fds that are polled.
    std::vector<int> polled_fds_;
    
    /// Read/write fd sets used by [select].
    fd_set rd_set_;
    fd_set wr_set_;

    /// Flag to force instructs [poll_worker_] to stop.
    std::atomic<bool> poll_stop_;

    /// Used to wake up [poll_worker_].
    pipe notifier_;

    /// Sleep on this cond var until waken up by
    /// the poll_worker_.
    std::condition_variable removal_cond;
};

} // namespace tcp_server_lib


#endif