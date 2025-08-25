#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <stack>
#include <thread>

namespace core {

template <size_t N>
class ThreadPool
{
public:
    ThreadPool()
    {
        start();
    }

    ~ThreadPool()
    {
        {
            std::unique_lock lock(mutex_);
            stop_ = true;
        }

        condition_.notify_all();

        std::ranges::for_each(pool_, &std::jthread::join);
    }

public:
    void submit_task(std::function<void()>&& task);

private:
    void start();

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    std::stack<std::function<void()>> tasks_;
    std::array<std::jthread, N> pool_;
    std::atomic<bool> stop_ = false;
};

template <size_t N>
void ThreadPool<N>::submit_task(std::function<void()>&& task)
{
    {
        std::scoped_lock lock(mutex_);
        tasks_.push(std::move(task));
    }
    condition_.notify_one();
}

template <size_t N>
void ThreadPool<N>::start()
{
    for (std::jthread& worker : pool_)
    {
        worker = std::jthread([this] {
            while (true)
            {
                std::function<void()> task;

                {
                    std::unique_lock lock(mutex_);
                    condition_.wait(lock, [this] {
                        return !tasks_.empty() || stop_;
                    });

                    if (stop_ && tasks_.empty()) break;

                    task = tasks_.top();
                    tasks_.pop();
                }

                task();
            }
        });
    }
}

}
