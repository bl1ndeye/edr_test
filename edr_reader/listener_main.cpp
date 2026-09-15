#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>
#include "listener.hpp"

namespace po = boost::program_options;

int main(int argc, char** argv)
{
    po::options_description desc("EDR listener options");
    desc.add_options()
        ("help", "produce help message")
        ("port,-p", po::value<std::string>(), "TCP port to listen on");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 0;
    }

    if (vm["port"].empty())
    {
        std::cout << "Port should be specified" << std::endl;
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }

    int parsed = std::atoi(vm["port"].as<std::string>().c_str());
    if (parsed <= 0 || parsed > 65535)
    {
        std::cerr << "Invalid port: " << vm["port"].as<std::string>() << std::endl;
        return EXIT_FAILURE;
    }

    TcpListener listener(static_cast<uint16_t>(parsed));
    listener.run();
    return 0;
}
