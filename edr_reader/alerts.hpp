#pragma once                            

#include "nlohmann/json.hpp"
#include <chrono>
#include <string>
#include <vector>

using TTimePoint = std::chrono::time_point<std::chrono::system_clock>;

enum class ALERT_TYPE
{
    SuspicioutActivityAlert=1,
    FileAdded=3,
    FileChanged=5,
    FileDeleted=7
};


struct EDR_AlertBase
{
    ALERT_TYPE m_type;
    virtual std::string toString()= 0;
    virtual nlohmann::json toJSON()= 0;
    virtual ~EDR_AlertBase() = default;
    protected:
    std::string getType()
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
};

struct ProcessAlert final: public EDR_AlertBase
{
    uint32_t m_pid;
    std::vector<uint32_t> m_pids;
    TTimePoint m_period_start;
    TTimePoint m_period_end;
    nlohmann::json toJSON()
    {
        nlohmann::json json_return;
        json_return["Type"] = getType();    
        json_return["pid"] = m_pid;  
        json_return["ppid"] = m_pids;
        json_return["period_start"] = std::chrono::duration_cast<std::chrono::seconds>(m_period_start.time_since_epoch()).count();        
        json_return["period_end"] = std::chrono::duration_cast<std::chrono::seconds>(m_period_end.time_since_epoch()).count();        
        return json_return;
    }
    std::string toString()
    {
        auto string_return = toJSON();
        return string_return.dump(4);
    }
};

struct FileAlert final: public EDR_AlertBase
{
    std::string m_file_name;
    nlohmann::json toJSON()
    {
        nlohmann::json json_return;
        json_return["Type"] = getType();
        json_return["File Name"] = m_file_name;
        return json_return;
    };
    std::string toString()
    {
        auto string_return = toJSON();
        return string_return.dump(4);
    }
};