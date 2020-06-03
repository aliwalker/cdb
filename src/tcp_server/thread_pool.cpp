#include "thread_pool.hpp"

namespace tcp_server_lib
{

thread_pool::thread_pool(std::size_t thread_num)
    : stop_(false), thread_num_(thread_num)
{
    workers_.reserve(thread_num);
    for (std::size_t i = 0; i < thread_num; i++)
        workers_.emplace_back(
            std::thread(std::bind(&thread_pool::init_worker, this)));
}

thread_pool::~thread_pool()
{
    stop();
}

void thread_pool::init_worker()
{
    while (!should_stop())
    {
        task_t task = fetch_task();

        if (task)
        {
            try {
                task();
            } catch (const std::exception& e) {
                // TODO: add log.
            }
        }
    }
}

thread_pool::task_t thread_pool::fetch_task()
{
    std::unique_lock<std::mutex> lock(tasks_mutex);

    tasks_cond.wait(lock, [&] { return this->should_stop() || !tasks_.empty(); });
    if (should_stop() || tasks_.empty())
        return nullptr;

    task_t task = std::move(tasks_.front());
    tasks_.pop();
    return task;
}

void thread_pool::add_task(const task_t &task)
{
    std::lock_guard<std::mutex> lock(tasks_mutex);

    tasks_.push(task);
    tasks_cond.notify_all();
}

void thread_pool::stop()
{
    stop_ = true;
    tasks_cond.notify_all();

    for (auto &worker : workers_)
        worker.join();

    // Prevent join from being called multiple times on the same
    // worker.
    workers_.clear();
}

bool thread_pool::should_stop()
{
    return stop_ || (workers_.size() > thread_num_);
}

void thread_pool::set_thread_num(std::size_t num)
{
    thread_num_.store(num);

    if (workers_.size() < thread_num_)
        while (workers_.size() < thread_num_)
            workers_.push_back(std::thread(std::bind(&thread_pool::init_worker, this)));
    else
        // Since [thread_num_] is updated, some fast workers
        // will exit their loops.
        tasks_cond.notify_all();
}

std::size_t thread_pool::get_thread_num() const
{
    return thread_num_;
}

} // namespace tcp_server_lib
