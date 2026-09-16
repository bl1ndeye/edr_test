#include "alert_collector.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

#include <boost/asio.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace
{
    constexpr int kMaxConnectAttempts = 5;
    constexpr int kMaxSendAttempts = 3;
    constexpr auto kConnectTimeout = std::chrono::seconds(5);
    constexpr auto kConnectRetryDelay = std::chrono::milliseconds(500);

    // Synchronous connect with a deadline (async_connect + steady_timer on a
    // single io_context). Returns a connected socket, or a closed one on
    // failure/timeout. Never throws.
    tcp::socket connect_once(asio::io_context& io,
                             const tcp::resolver::results_type& endpoints,
                             std::chrono::milliseconds timeout,
                             boost::system::error_code& out_ec)
    {
        tcp::socket socket(io);
        asio::steady_timer timer(io);
        timer.expires_after(timeout);

        bool timed_out = false;
        bool completed = false;

        timer.async_wait([&](boost::system::error_code ec)
        {
            if (ec != asio::error::operation_aborted && !completed)
            {
                timed_out = true;
                boost::system::error_code ignored;
                socket.close(ignored);
            }
        });

        asio::async_connect(socket, endpoints, [&](boost::system::error_code ec, const tcp::endpoint&)
        {
            completed = true;
            out_ec = ec;
            timer.cancel();
        });

        io.restart();
        io.run();

        if (timed_out)
        {
            out_ec = asio::error::timed_out;
            boost::system::error_code ignored;
            socket.close(ignored);
        }
        return socket;
    }

    tcp::socket connect_with_retry(asio::io_context& io,
                                   const tcp::resolver::results_type& endpoints,
                                   const std::string& host, const std::string& port)
    {
        for (int attempt = 0; attempt < kMaxConnectAttempts; ++attempt)
        {
            boost::system::error_code ec;
            tcp::socket socket = connect_once(io, endpoints, kConnectTimeout, ec);
            if (!ec)
            {
                return socket;
            }
            std::cerr << "AlertCollector: connect attempt " << (attempt + 1) << "/"
                      << kMaxConnectAttempts << " to " << host << ":" << port
                      << " failed: " << ec.message() << std::endl;
            socket.close();
            std::this_thread::sleep_for(kConnectRetryDelay);
        }
        return tcp::socket(io);
    }
}

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

    asio::io_context io;
    tcp::resolver resolver(io);
    boost::system::error_code ec;
    auto endpoints = resolver.resolve(m_host, m_port, ec);
    if (ec)
    {
        std::cerr << "AlertCollector: resolve '" << m_host << ":" << m_port
                  << "' failed: " << ec.message() << std::endl;
        return;
    }

    tcp::socket socket(io);

    while (true)
    {
        auto alert = m_buffer_ptr->pop();
        if (!alert) break;

        std::string payload = alert->toJSON().dump() + "\n";

        if (!socket.is_open())
        {
            socket = connect_with_retry(io, endpoints, m_host, m_port);
            if (!socket.is_open())
            {
                std::cerr << "AlertCollector: cannot reach " << m_host << ":" << m_port
                          << ", dropping alert" << std::endl;
                continue;
            }
        }

        bool sent = false;
        for (int attempt = 0; attempt < kMaxSendAttempts && !sent; ++attempt)
        {
            asio::write(socket, asio::buffer(payload), ec);
            if (!ec)
            {
                sent = true;
            }
            else
            {
                std::cerr << "AlertCollector: send failed (attempt " << (attempt + 1)
                          << "/" << kMaxSendAttempts << "): " << ec.message() << std::endl;
                socket.close(ec);
                socket = connect_with_retry(io, endpoints, m_host, m_port);
                if (!socket.is_open())
                {
                    break;
                }
            }
        }
        if (!sent)
        {
            std::cerr << "AlertCollector: dropped alert after retries" << std::endl;
        }
    }
}
