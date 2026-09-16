#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "alerts.hpp"
#include "nlohmann/json.hpp"
#include "ring_buffer.hpp"

// анализирует полученный список строк событий
// парсит в контейнер и детектирует по правилу
class EventDetector
{
public:
    EventDetector() = default;

    EventDetector(const EventDetector&) = delete;
    EventDetector& operator=(const EventDetector&) = delete;
    EventDetector(EventDetector&&) = default;
    EventDetector& operator=(EventDetector&&) = default;

    void setBuffer(std::shared_ptr<BufferRingThreadSafe<std::string>> buf);
    void setBufferAleft(std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> buf);
    // Feed a ProcessStarted event directly into the detector map (test/utility hook).
    void addProcessEvent(std::uint32_t ppid, std::uint32_t pid, std::int64_t ts_seconds);

    void detectSuspuciousActivity();
    void start();

private:
    std::shared_ptr<BufferRingThreadSafe<std::string>> m_buffer_ptr;
    std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> m_buffer_alert;
    BufferRingThreadSafe<nlohmann::json> m_inner_parsed_buffer;
    std::unordered_map<std::uint32_t, std::vector<std::pair<std::uint32_t, TTimePoint>>> m_detector_event_map;
};
