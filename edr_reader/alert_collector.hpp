#pragma once

#include <memory>
#include <string>

#include "alerts.hpp"
#include "ring_buffer.hpp"

// сборщик аллертов, отправляет все по тисипи в нужном формате
class AlertCollector
{
public:
    AlertCollector() = default;
    AlertCollector(std::string host, std::string port);

    AlertCollector(const AlertCollector&) = delete;
    AlertCollector& operator=(const AlertCollector&) = delete;
    AlertCollector(AlertCollector&&) = default;
    AlertCollector& operator=(AlertCollector&&) = default;

    void setBuffer(std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> buf);
    void start();

private:
    std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> m_buffer_ptr;
    std::string m_host;
    std::string m_port;
};
