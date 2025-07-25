#pragma once

#include <mutex>

template <class T>
class MutexProtected
{
public:
    MutexProtected()
        : data_ {}
        , mutex_ {}
    {}

    MutexProtected(T&& data)
        : data_(std::move(data))
        , mutex_ {}
    {}

    auto with(auto&& functor)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return functor(data_);
    }

private:
    T data_;
    std::mutex mutex_;
};

