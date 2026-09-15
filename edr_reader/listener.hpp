#pragma once

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

class TcpListener
{
public:
    explicit TcpListener(uint16_t port) : m_port(port) {}

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;
    TcpListener(TcpListener&&) = default;
    TcpListener& operator=(TcpListener&&) = default;

    void run()
    {
        try
        {
            asio::io_context io_ctx;
            tcp::acceptor acceptor(io_ctx, tcp::endpoint(tcp::v4(), m_port));
            std::cout << "Listening on 0.0.0.0:" << m_port << std::endl;

            while (true)
            {
                tcp::socket socket(io_ctx);
                acceptor.accept(socket);
                std::cout << "[+] connected from " << socket.remote_endpoint() << std::endl;
                handle_session(socket);
                std::cout << "[-] disconnected" << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "TcpListener: " << e.what() << std::endl;
        }
    }

private:
    void handle_session(tcp::socket& socket)
    {
        boost::system::error_code ec;
        asio::streambuf buf;
        while (true)
        {
            std::size_t n = asio::read_until(socket, buf, '\n', ec);
            if (ec) break;

            std::string line(
                asio::buffers_begin(buf.data()),
                asio::buffers_begin(buf.data()) + n);
            buf.consume(n);

            try
            {
                auto parsed = nlohmann::json::parse(line);
                if (!parsed.is_object() || !parsed.contains("Type"))
                {
                    std::cerr << "[!] missing required 'Type' field, raw: " << line;
                    continue;
                }
                std::cout << "[alert][" << parsed["Type"].get<std::string>() << "] "
                          << parsed.dump() << std::endl;
            }
            catch (const nlohmann::json::parse_error& e)
            {
                std::cerr << "[!] invalid JSON: " << e.what()
                          << " | raw: " << line;
            }
        }
        if (ec && ec != asio::error::eof && ec != asio::error::connection_reset)
        {
            std::cerr << "TcpListener read error: " << ec.message() << std::endl;
        }
    }

    uint16_t m_port;
};
