#pragma once

#include <coroutine>
#include <utility>

namespace libcoro {

template <class T>
class Generator
{
public:
    struct promise_type;

private:
    struct Iterator
    {
        auto operator++() { generator.handle_.resume(); }
        auto operator*() { return std::move(generator.handle_.promise().result); }
        auto operator!=(auto const&) { return !generator.handle_.done(); }
        Generator& generator;
    };

public:
    Generator() = default;

    Generator(promise_type* promise)
        : handle_(std::coroutine_handle<promise_type>::from_promise(*promise))
    {}

    ~Generator()
    {
        handle_.destroy();
    }

    T&& next() { handle_.resume(); return std::move(handle_.promise().result); }

    Iterator begin() { return {*this}; }
    Iterator end() { return {*this}; }

private:
    std::coroutine_handle<promise_type> handle_;
};

template <class T>
struct Generator<T>::promise_type
{
    T result;

    auto get_return_object() { return Generator(this); }

    auto unhandled_exception() noexcept {}

    auto return_void() {}

    auto yield_value(auto&& value)
    {
        result = std::forward<decltype(value)>(value);
        return std::suspend_always {};
    }

    auto initial_suspend() noexcept { return std::suspend_never {}; }
    auto final_suspend() noexcept { return std::suspend_always {}; }
};

}
