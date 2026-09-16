#include "alerts.hpp"

#include <chrono>

std::string EDR_AlertBase::getType()
{
    switch (m_type)
    {
    case ALERT_TYPE::SuspicioutActivityAlert:
        return "SuspiciousActivityAlert";
    case ALERT_TYPE::FileAdded:
        return "File Added";
    case ALERT_TYPE::FileChanged:
        return "File Changed";
    case ALERT_TYPE::FileDeleted:
        return "File Deleted";
    }
    return "Undefined";
}

nlohmann::json ProcessAlert::toJSON()
{
    nlohmann::json json_return;
    json_return["Type"] = getType();
    json_return["pid"] = m_pid;
    json_return["ppid"] = m_pids;
    json_return["period_start"] = std::chrono::duration_cast<std::chrono::seconds>(m_period_start.time_since_epoch()).count();
    json_return["period_end"] = std::chrono::duration_cast<std::chrono::seconds>(m_period_end.time_since_epoch()).count();
    return json_return;
}

std::string ProcessAlert::toString()
{
    return toJSON().dump(4);
}

nlohmann::json FileAlert::toJSON()
{
    nlohmann::json json_return;
    json_return["Type"] = getType();
    json_return["File Name"] = m_file_name;
    return json_return;
}

std::string FileAlert::toString()
{
    return toJSON().dump(4);
}
