#include "Task.hpp"

#include <algorithm>
#include <vector>

void libcoro::Scheduler::start()
{
    while (true)
    {
        std::unique_lock lock(mutex_);
        condition_.wait(lock, [&] () {
            return state_.with([] (auto state) { return state == State::RUNNING || state == State::STOPPED; });
        });

        if (state_.with([] (auto state) { return state == State::STOPPED; }))
        {
            break;
        }

        auto tasks = tasks_.with([] (std::vector<std::shared_ptr<Task>>& tasks) {
            std::vector<std::shared_ptr<Task>> result {};

            std::erase_if(tasks, [] (auto& task) {
                return task->state() == libcoro::Task<>::State::FINISHED;
            });

            std::copy_if(tasks.begin(), tasks.end(), std::back_inserter(result), [] (auto& task) {
                return task->state() == libcoro::Task<>::State::STARTED ||
                       task->state() == libcoro::Task<>::State::RESUMED;
            });

            return result;
        });

        for (auto& task : tasks)
        {
            task->set_state(libcoro::Task<>::State::RUNNING);
            std::ignore = pool_.submit_task(task->handle());
        }

        state_.with([] (auto& state) { state = State::WAITING; });
    }
}

void libcoro::Scheduler::stop()
{
    state_.with([] (auto& state) { state = State::STOPPED; });
    notify();
}

template <>
libcoro::Task<void> libcoro::Task<>::delay(std::chrono::milliseconds delay)
{
    std::this_thread::sleep_for(delay);
    co_return {};
}
