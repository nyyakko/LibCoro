#pragma once

#include "core/MutexProtected.hpp"
#include "core/ThreadPool.hpp"

#include <coroutine>
#include <memory>
#include <type_traits>
#include <variant>

namespace libcoro {

template <class T = void>
class Task
{
public:
    enum class State;

    using value_t = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

    struct promise_type;
    struct awaitable_t;

public:
    Task(promise_type* promise)
        : handle_(
            std::make_shared<std::coroutine_handle<promise_type>>(std::coroutine_handle<promise_type>::from_promise(*promise))
        )
    {}

    Task(Task const& that)
        : handle_(that.handle_)
    {}

    Task(Task&& that)
        : handle_(std::move(that.handle_))
    {}

public:
    auto handle(this auto& self) { return *self.handle_; }

    auto&& result(this auto& self) { return std::move(self.handle_->promise().result); }

    auto done(this auto& self) { return self.handle_->done(); }

    auto continuation(this auto& self) { return self.handle_->promise().continuation; }
    auto set_continuation(this auto& self, auto const& continuation) { self.handle_->promise().continuation = continuation; }

    auto state(this auto& self) { return self.handle_->promise().state; }
    auto set_state(this auto& self, auto state) { self.handle_->promise().state = state; }

    static Task<void> delay(std::chrono::milliseconds delay);

public:
    awaitable_t operator co_await();

private:
    std::shared_ptr<std::coroutine_handle<promise_type>> handle_;
};

template <>
enum class Task<>::State { STARTED, RUNNING, AWAITING, RESUMED, FINISHED };

class Scheduler
{
    enum class State { RUNNING, WAITING, STOPPED };

public:
    class Task
    {
    public:
        virtual ~Task() = default;

        virtual std::coroutine_handle<> handle() = 0;
        virtual std::shared_ptr<Task> continuation() = 0;

        virtual void set_state(libcoro::Task<>::State state) = 0;
        virtual libcoro::Task<>::State state() = 0;
    };

    template <class T>
    class TaskImpl : public Task
    {
    public:
        TaskImpl(libcoro::Task<T>&& task)
            : task_(std::move(task))
        {}

        TaskImpl(std::coroutine_handle<typename libcoro::Task<T>::promise_type> handle)
            : task_(handle)
        {}

    public:
        virtual std::coroutine_handle<> handle() override { return task_.handle(); }
        virtual std::shared_ptr<Task> continuation() override { return task_.continuation(); };

        virtual libcoro::Task<>::State state() override { return task_.state(); }
        virtual void set_state(libcoro::Task<>::State state) override { task_.set_state(state); }

        typename libcoro::Task<T>::value_t&& result() { return task_.result(); }

    private:
        libcoro::Task<T> task_;
    };

public:
    Scheduler()
        : pool_(8)
    {}

public:
    static Scheduler& the()
    {
        static Scheduler the {};
        return the;
    }

    void start();
    void stop();

    void notify()
    {
        std::scoped_lock lock(mutex_);
        state_ = State::RUNNING;
        condition_.notify_one();
    }

    template <class T>
    void schedule(this auto& self, libcoro::Task<T> task)
    {
        self.tasks_.with([&] (auto& tasks) {
            tasks.push_back(std::shared_ptr<Task>(new TaskImpl(std::move(task))));
        });

        self.notify();
    }

    void schedule(this auto& self, auto x, auto ... xs)
    {
        std::apply([&] (auto&& ... xs) {
            self.tasks_.with([&] (auto& tasks) {
                (tasks.push_back(std::shared_ptr<Task>(new TaskImpl(std::move(xs)))), ...);
            });
        }, std::make_tuple(x, xs...));

        self.notify();
    }

    auto find_task(std::coroutine_handle<> task)
    {
        return tasks_.with([&] (auto const& tasks) {
            return std::find_if(tasks.begin(), tasks.end(), [&] (auto const& current) {
                return current->handle() == task;
            });
        });
    }

    template <class T>
    bool is_scheduled(libcoro::Task<T> const& task)
    {
        return tasks_.with([&] (auto const& tasks) {
            return std::find_if(tasks.begin(), tasks.end(), [&] (auto const& current) {
                return current->handle() == task.handle();
            }) != tasks.end();
        });
    }

private:
    core::BS::thread_pool<> pool_;
    core::MutexProtected<std::vector<std::shared_ptr<Task>>> tasks_;
    core::MutexProtected<State> state_;
    std::mutex mutex_;
    std::condition_variable condition_;
};

template <class T>
struct Task<T>::promise_type
{
    Task<>::State state;
    std::shared_ptr<Scheduler::Task> continuation;
    value_t result;

    auto get_return_object() { return Task(this); }
    auto unhandled_exception() noexcept {}

    template <class U>
    auto return_value(U&& value)
        requires std::constructible_from<value_t, U>
    {
        result = std::forward<U>(value);
    }

    auto return_value(value_t&& value)
    {
        result = std::move(value);
    }

    auto initial_suspend() noexcept
    {
        state = Task<>::State::STARTED;
        return std::suspend_always {};
    }

    auto final_suspend() noexcept
    {
        state = Task<>::State::FINISHED;

        if (continuation)
        {
            continuation->set_state(libcoro::Task<>::State::RESUMED);
        }

        Scheduler::the().notify();

        return std::suspend_always {};
    }
};

template <class T>
struct Task<T>::awaitable_t
{
    Task& self;

    auto await_ready() const noexcept { return self.done(); }

    auto await_suspend(std::coroutine_handle<> handle)
    {
        auto continuation = *Scheduler::the().find_task(handle);
        continuation->set_state(libcoro::Task<>::State::AWAITING);

        self.set_continuation(continuation);

        if (!Scheduler::the().is_scheduled(self))
        {
            Scheduler::the().schedule(self);
        }
    }

    Task<T>::value_t&& await_resume() { return self.result(); }
};

template <class T>
Task<T>::awaitable_t Task<T>::operator co_await()
{
    return awaitable_t(*this);
}

}
