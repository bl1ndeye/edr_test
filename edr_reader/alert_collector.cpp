#include "alert_collector.hpp"

#include <iostream>
#include <utility>

#include <boost/asio.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

AlertCollector::AlertCollector(std::string host, std::string port)
    : m_host(std::move(host)), m_port(std::move(port))
{
}

void AlertCollector::setBuffer(std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> buf)
{
    m_buffer_ptr = std::move(buf);
}

void AlertCollector::start()
{
    if (!m_buffer_ptr)
    {
        std::cerr << "AlertCollector::start: alert buffer not set" << std::endl;
        return;
    }
    try
    {
        asio::io_context io_ctx;
        tcp::resolver resolver(io_ctx);
        boost::system::error_code ec;
        auto endpoints = resolver.resolve(m_host, m_port, ec);
        if (ec)
        {
            std::cerr << "AlertCollector: resolve '" << m_host << ":" << m_port
                      << "' failed: " << ec.message() << std::endl;
            return;
        }

        tcp::socket socket(io_ctx);

        auto send_alert = [&socket, &endpoints, &ec](std::unique_ptr<EDR_AlertBase>&& alert_ptr)
        {
            if (!socket.is_open())
            {
                asio::connect(socket, endpoints, ec);
                if (ec)
                {
                    std::cerr << "AlertCollector: connect failed: " << ec.message() << std::endl;
                    return;
                }
            }

            std::string payload = alert_ptr->toJSON().dump() + "\n";
            asio::write(socket, asio::buffer(payload), ec);
            if (ec)
            {
                std::cerr << "AlertCollector: send failed: " << ec.message() << std::endl;
                socket.close();
            }
        };

        while (true)
        {
            auto alert = m_buffer_ptr->pop();
            if (!alert) break;
            send_alert(std::move(alert));
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "AlertCollector: exception: " << e.what() << std::endl;
    }
}
