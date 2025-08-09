#pragma once

#include <concepts>
#include <coroutine>
#include <utility>
#include <variant>

namespace libcoro {

template <class T>
class Generator
{
public:
    using value_t = std::conditional_t<std::is_void_v<T>, std::monostate, T>;

    struct promise_type;

private:
    struct Iterator
    {
        auto operator++() { if (!generator.handle_.done()) generator.handle_.resume(); }
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

    auto&& next() { return handle_.resume(), std::move(handle_.promise().result); }

    Iterator begin() { return next(), Iterator(*this); }
    Iterator end() { return Iterator(*this); }

private:
    std::coroutine_handle<promise_type> handle_;
};

template <class T>
struct Generator<T>::promise_type
{
    value_t result;

    auto get_return_object() { return Generator(this); }
    auto unhandled_exception() noexcept {}

    auto return_void() {}

    template <class U>
    auto yield_value(U&& value)
        requires std::constructible_from<value_t, U>
    {
        result = std::forward<U>(value);
        return std::suspend_always {};
    }

    auto yield_value(value_t&& value)
    {
        result = std::move(value);
        return std::suspend_always {};
    }

    auto initial_suspend() noexcept { return std::suspend_always {}; }
    auto final_suspend() noexcept { return std::suspend_always {}; }
};

}
