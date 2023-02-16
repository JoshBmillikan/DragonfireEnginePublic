//
// Created by josh on 2/15/23.
//
#include <catch2/catch_test_macros.hpp>
#include <thread_pool.h>

using namespace df;
TEST_CASE("Thread pool")
{
    ThreadPool pool;
    auto task = pool.spawn<int>([] {
        sleep(1);
        return 1 + 1;
    });
    auto task2 = pool.spawn<int>([] {
        sleep(2);
        return 2 * 2;
    });

    task.wait();
    task2.wait();
    REQUIRE(task.get() == 2);
    REQUIRE(task2.get() == 4);
}