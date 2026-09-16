#pragma once

#include "nlohmann/json.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

using TTimePoint = std::chrono::time_point<std::chrono::system_clock>;

enum class ALERT_TYPE
{
    SuspicioutActivityAlert = 1,
    FileAdded = 3,
    FileChanged = 5,
    FileDeleted = 7
};

struct EDR_AlertBase
{
    ALERT_TYPE m_type;
    virtual std::string toString() = 0;
    virtual nlohmann::json toJSON() = 0;
    virtual ~EDR_AlertBase() = default;

protected:
    std::string getType();
};

struct ProcessAlert final : public EDR_AlertBase
{
    std::uint32_t m_pid;
    std::vector<std::uint32_t> m_pids;
    TTimePoint m_period_start;
    TTimePoint m_period_end;
    nlohmann::json toJSON() override;
    std::string toString() override;
};

struct FileAlert final : public EDR_AlertBase
{
    std::string m_file_name;
    nlohmann::json toJSON() override;
    std::string toString() override;
};
