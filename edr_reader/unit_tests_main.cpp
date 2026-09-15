#define CATCH_CONFIG_MAIN 
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <thread>
#include "ring_buffer.hpp"

TEST_CASE("Items are added", "[buffer]") {
    BufferRingThreadSafe<std::string> buffer;
   for (int i=0;i<5;++i)
    {
        buffer.push("test string");
    }
    REQUIRE(buffer.size()==5);
}

TEST_CASE("Items are added and popped", "[buffer]") {
    BufferRingThreadSafe<std::string> buffer;
   for (int i=0;i<5;++i)
    {
        buffer.push("test string");
    }
   for (int i=0;i<5;++i)
    {
        buffer.pop();    
    } 
    REQUIRE(buffer.size()==0);
}

TEST_CASE("Push blocks when buffer is full (backpressure, no drop)", "[buffer]") {
    BufferRingThreadSafe<std::string> buffer(2);
    buffer.push("one");
    buffer.push("two");

    std::atomic<bool> pushed{false};
    std::jthread producer([&]() {
        buffer.push("three");   // blocks until a slot is freed
        pushed = true;
    });

    // Give the producer a moment to block on the full buffer.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(pushed == false);   // still blocked -> nothing was dropped

    buffer.pop();               // free one slot
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(pushed == true);    // producer unblocked and delivered the item

    producer.join();
    REQUIRE(buffer.size() == 2);
}

TEST_CASE("Pop returns sentinel when buffer is closed and drained", "[buffer]") {
    BufferRingThreadSafe<std::string> buffer;
    buffer.push("only item");
    REQUIRE(buffer.pop() == "only item");

    buffer.close();
    REQUIRE(buffer.pop().empty());  // closed + empty -> sentinel
}





