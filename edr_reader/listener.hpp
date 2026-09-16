#pragma once

#include <cstdint>

#include <boost/asio.hpp>

// просто слушалка по тисипи, благо есть опыт с буст асио
// принимает сессии , печатает полученное в стд соут
class TcpListener
{
public:
    explicit TcpListener(std::uint16_t port);

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;
    TcpListener(TcpListener&&) = default;
    TcpListener& operator=(TcpListener&&) = default;

    void run();

private:
    void handle_session(boost::asio::ip::tcp::socket& socket);

    std::uint16_t m_port;
};
