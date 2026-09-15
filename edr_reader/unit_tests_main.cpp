#define CATCH_CONFIG_MAIN 
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <memory>
#include <thread>
#include "ring_buffer.hpp"
#include "file_detector.hpp"

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

TEST_CASE("Detection: >=5 processes from same ppid within 10s triggers alert", "[detector]") {
    EventDetector detector;
    auto alerts = std::make_shared<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>>(80);
    detector.setBufferAleft(alerts);

    detector.addProcessEvent(100, 1, 1000);
    detector.addProcessEvent(100, 2, 1001);
    detector.addProcessEvent(100, 3, 1002);
    detector.addProcessEvent(100, 4, 1003);
    detector.addProcessEvent(100, 5, 1004);

    detector.detectSuspuciousActivity();

    REQUIRE(alerts->size() == 1);
    auto alert = alerts->pop();
    auto* process_alert = dynamic_cast<ProcessAlert*>(alert.get());
    REQUIRE(process_alert != nullptr);
    REQUIRE(process_alert->m_pid == 100);
    REQUIRE(process_alert->m_pids == std::vector<std::uint32_t>{1, 2, 3, 4, 5});
}

TEST_CASE("Detection: fewer than 5 processes -> no alert", "[detector]") {
    EventDetector detector;
    auto alerts = std::make_shared<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>>(80);
    detector.setBufferAleft(alerts);

    detector.addProcessEvent(100, 1, 1000);
    detector.addProcessEvent(100, 2, 1001);
    detector.addProcessEvent(100, 3, 1002);
    detector.addProcessEvent(100, 4, 1003);

    detector.detectSuspuciousActivity();

    REQUIRE(alerts->size() == 0);
}

TEST_CASE("Detection: 5 processes spread over >10s -> no alert", "[detector]") {
    EventDetector detector;
    auto alerts = std::make_shared<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>>(80);
    detector.setBufferAleft(alerts);

    detector.addProcessEvent(100, 1, 0);
    detector.addProcessEvent(100, 2, 60);
    detector.addProcessEvent(100, 3, 120);
    detector.addProcessEvent(100, 4, 180);
    detector.addProcessEvent(100, 5, 240);

    detector.detectSuspuciousActivity();

    REQUIRE(alerts->size() == 0);
}

TEST_CASE("Detection: only the offending ppid triggers among several", "[detector]") {
    EventDetector detector;
    auto alerts = std::make_shared<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>>(80);
    detector.setBufferAleft(alerts);

    for (std::int64_t i = 0; i < 5; ++i)
    {
        detector.addProcessEvent(200, static_cast<std::uint32_t>(1000 + i), 1000 + i);
    }
    for (std::int64_t i = 0; i < 3; ++i)
    {
        detector.addProcessEvent(300, static_cast<std::uint32_t>(2000 + i), 2000 + i);
    }

    detector.detectSuspuciousActivity();

    REQUIRE(alerts->size() == 1);
    auto alert = alerts->pop();
    auto* process_alert = dynamic_cast<ProcessAlert*>(alert.get());
    REQUIRE(process_alert != nullptr);
    REQUIRE(process_alert->m_pid == 200);
}





