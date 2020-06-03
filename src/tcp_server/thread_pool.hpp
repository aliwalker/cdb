#ifndef TCP_SERVER_THREAD_POOL_HPP
#define TCP_SERVER_THREAD_POOL_HPP

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>

namespace tcp_server_lib
{

/// Simple thread pool.
class thread_pool {
public:
    thread_pool(std::size_t thread_num);
    ~thread_pool();

    thread_pool(const thread_pool&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;

public:
    /// Only allow void(*)() task type.
    typedef std::function<void()> task_t;

    /// Task to be performed by workers.
    void add_task(const task_t &task);

    /// Number of underlying workers.
    void set_thread_num(std::size_t num);
    std::size_t get_thread_num() const;

    /// Stop the thread pool.
    void stop();

private:
    /*
    Worker operations.
    */

    /// Worker initialization.
    void init_worker();

    /// Fetch a task from tasks_.
    task_t fetch_task();

    /// Return true to terminate a worker.
    bool should_stop();

private:
    std::vector<std::thread> workers_;

    /// Flag indicates whether the pool should stop.
    std::atomic<bool> stop_;

    /// Number of thread workers.
    std::atomic<std::size_t> thread_num_;

    /// Underlying tasks to run by [workers_]
    std::queue<task_t> tasks_;

    /// Thread safety.
    std::mutex tasks_mutex;
    std::condition_variable tasks_cond;
};

} // namespace tcp_server_lib


#endif