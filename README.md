# coroutine library.

asynchronous tasks just like in C# ... kinda.

# installation

you may install it with [CPM](https://github.com/cpm-cmake/CPM.cmake) or install directly into your system with the following:

* ``py install.py``

and then include it with cmake into your project

```cmake
cmake_minimum_required_version(VERSION 3.25)

project(CoolProject LANGUAGES CXX)

find_package(LibCoro CONFIG REQUIRED)
add_executable(CoolProject source.cpp)
target_link_libraries(CoolProject PRIVATE LibCoro::LibCoro)
```

# examples

## Task<T>

```c++
#include <libcoro/Task.hpp>

#include <thread>
#include <iostream>

using namespace std::literals;

libcoro::Task<void> prepare_eggs_async()
{
    std::cout << "preparing eggs...\n";
    co_await libcoro::Task<>::delay(2s);
    co_return {};
}

libcoro::Task<void> prepare_bacon_async()
{
    std::cout << "preparing bacon...\n";
    co_await libcoro::Task<>::delay(1s);
    co_return {};
}

libcoro::Task<void> prepare_toast_async()
{
    std::cout << "preparing toast...\n";
    co_await libcoro::Task<>::delay(2s);
    co_return {};
}

libcoro::Task<void> prepare_breakfast_async()
{
    auto prepareEggsTask = prepare_eggs_async();
    auto prepareBaconTask = prepare_bacon_async();
    auto prepareToastTask = prepare_toast_async();

    Scheduler::the().schedule(prepareEggsTask, prepareBaconTask, prepareToastTask);

    co_await prepareBaconTask;
    std::cout << "adding sauce to bacon\n";
    std::cout << "bacon is ready!\n\n";

    co_await prepareEggsTask;
    std::cout << "applying seasoning to eggs\n";
    std::cout << "eggs are ready!\n\n";

    co_await prepareToastTask;
    std::cout << "applying butter to toast\n";
    std::cout << "applying jam to toast\n";
    std::cout << "toast is ready!\n\n";

    std::cout << "breakfast is ready!\n";

    co_return {};
}

int main()
{
    std::thread schedulerThread {
        [] {
            libcoro::Scheduler::the().start();
        }
    };

    libcoro::Scheduler::the().schedule(prepare_breakfast_async());

    schedulerThread.join();
}

```

## Generator<T>

```c++
#include <libcoro/Generator.hpp>

libcoro::Generator<std::filesystem::path> walk_files_async(std::filesystem::path path)
{
    for (auto const& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_directory() || entry.is_regular_file())
        {
            co_yield entry;
        }
    }

    co_return;
}

libcoro::Generator<std::filesystem::path> walk_files_async(std::filesystem::path path, std::vector<std::filesystem::path> const& blacklistedPaths, std::vector<std::filesystem::path> const& blacklistedExtensions)
{
    for (auto const& current : walk_files_async(path))
    {
        if (std::filesystem::is_directory(current))
        {
            auto maybePath = std::find(blacklistedPaths.begin(), blacklistedPaths.end(), current);

            if (maybePath != blacklistedPaths.end())
            {
                std::cout << "Skipping blacklisted path: " << current << '\n';
                continue;
            }

            for (auto const& file : walk_files_async(current, blacklistedPaths, blacklistedExtensions))
            {
                co_yield file;
            }
        }
        else
        {
            auto maybeExtension = std::find(blacklistedExtensions.begin(), blacklistedExtensions.end(), current.extension());

            if (maybeExtension != blacklistedExtensions.end())
            {
                std::cout << "Skipping blacklisted file: " << current << '\n';
                continue;
            }

            co_yield current;
        }
    }

    co_return;
}

int main()
{
    std::vector<std::filesystem::path> blacklistedPaths { "./build", "./.cache" };
    std::vector<std::filesystem::path> blacklistedExtensions { ".txt" };

    for (auto const& path : walk_files_async(".", blacklistedPaths, blacklistedExtensions))
    {
        std::cout << "Visiting path: " << path.string() << '\n';
    }
}
```

i recommend you to simply explore the code and see what you can do with it. seriously. do it.
