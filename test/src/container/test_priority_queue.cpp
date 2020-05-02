#include "test/src/prt_test.h"
#include <catch2/catch.hpp>
#include "src/container/priority_queue.h"

#include <cstdlib>

TEST_CASE( "Test priority queue", "[priotity_queue]") {
    prt::priority_queue<float> q;
    REQUIRE(q.empty());

    for (unsigned int i = 0; i < 1000; ++i) {
        float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        q.push(r);
        REQUIRE(q.size() == i + 1);
    }
    float last = q.top();
    size_t expectedSize = 1000;
    while (!q.empty()) {
        REQUIRE(q.size() == expectedSize);
        REQUIRE(q.top() >= last);
        q.pop();
        --expectedSize;
    }
    REQUIRE(q.empty());
}