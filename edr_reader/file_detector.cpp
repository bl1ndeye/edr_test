#include "file_detector.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

void EventDetector::setBuffer(std::shared_ptr<BufferRingThreadSafe<std::string>> buf)
{
    m_buffer_ptr = std::move(buf);
}

void EventDetector::setBufferAleft(std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> buf)
{
    m_buffer_alert = std::move(buf);
}

void EventDetector::addProcessEvent(std::uint32_t ppid, std::uint32_t pid, std::int64_t ts_seconds)
{
    TTimePoint t { std::chrono::duration_cast<TTimePoint::duration>(std::chrono::seconds(ts_seconds)) };
    m_detector_event_map[ppid].push_back(std::pair{pid, t});
}

void EventDetector::detectSuspuciousActivity()
{
    std::erase_if(m_detector_event_map, [](const auto& pair)
    {
        return pair.second.size() < 5;
    });
    for (auto& [ppid, pid_ts_pairs] : m_detector_event_map)
    {
        std::sort(pid_ts_pairs.begin(), pid_ts_pairs.end(),
            [](const auto& a, const auto& b)
            {
                return a.second < b.second;
            });
        size_t left = 0;
        for (size_t right = 0; right < pid_ts_pairs.size(); ++right)
        {
            while ((left < right) && (pid_ts_pairs[right].second - pid_ts_pairs[left].second > std::chrono::seconds(10)))
            {
                ++left;
            }
            if (right - left + 1 >= 5)
            {
                auto alert = std::make_unique<ProcessAlert>();
                alert->m_type = ALERT_TYPE::SuspicioutActivityAlert;
                alert->m_pid = ppid;
                alert->m_period_start = pid_ts_pairs[left].second;
                alert->m_period_end = pid_ts_pairs[right].second;
                alert->m_pids.reserve(right - left + 1);
                for (size_t i = left; i <= right; ++i)
                {
                    alert->m_pids.push_back(pid_ts_pairs[i].first);
                }
                m_buffer_alert->push(std::move(alert));
                break;
            }
        }
    }
}

void EventDetector::start()
{
    if (m_buffer_ptr)
    {
        m_inner_parsed_buffer.resize(20);
        auto parseItemsFromStringBuffer = [&] ()
        {
            while (true)
            {
                auto item = m_buffer_ptr->pop();
                if (item.empty())
                {
                    break;
                }
                nlohmann::json json_item;
                try
                {
                    json_item = nlohmann::json::parse(item);
                    if (json_item["type"] == "ProcessStarted")
                    {
                        m_inner_parsed_buffer.push(json_item);
                    }
                }
                catch (const nlohmann::json::parse_error& e)
                {
                    std::cerr << "EventDetector JSON parse error: " << e.what() << '\n';
                }
            }
            m_inner_parsed_buffer.close();
        };
        auto parseAndValidateFormat = [&] ()
        {
            while (true)
            {
                auto current_element = m_inner_parsed_buffer.pop();
                if (current_element.is_null())
                {
                    break;
                }
                std::uint32_t current_ppid = current_element["ppid"];
                std::uint32_t current_pid = current_element["pid"];
                auto raw_timestamp = current_element["ts"];
                TTimePoint current_time { std::chrono::duration_cast<TTimePoint::duration>(std::chrono::seconds(raw_timestamp)) };
                m_detector_event_map[current_ppid].push_back(std::pair{current_pid, current_time});
            }
        };

        {
            std::jthread thread_parse_string {parseItemsFromStringBuffer};
            std::jthread thread_validate_json {parseAndValidateFormat};
        }
        if (m_buffer_alert)
        {
            detectSuspuciousActivity();
        }
        else
        {
            // TODO ACHTUNG something
        }
    }
    else
    {
        // TODO alert error or whatever
    }
}
