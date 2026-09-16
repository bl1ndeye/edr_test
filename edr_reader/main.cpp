#include <iostream>
#include <string>
#include <vector>
#include <ranges>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <random>
#include <stop_token>

#include "event_enumerator.hpp"
#include "file_detector.hpp"
#include "file_estimator.hpp"
#include "alert_collector.hpp"
#include <boost/program_options.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#endif

namespace po = boost::program_options;

namespace
{
    std::stop_source g_app_stop_source;

#ifdef _WIN32
    BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
    {
        if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT || ctrl_type == CTRL_CLOSE_EVENT)
        {
            g_app_stop_source.request_stop();
            return TRUE;
        }
        return FALSE;
    }
#else
    void signal_handler(int)
    {
        g_app_stop_source.request_stop();
    }
#endif
}

int main(int argc, char** argv) {
    po::options_description desc("EDR APP options");
    desc.add_options()
        ("help", "produce help message")
        ("host,-h", po::value<std::string>(), "set host to send report")
        ("port,-p", po::value<std::string>(), "set port for host to send report")
        ("events,-e", po::value<std::string>(), "path to events log file")
        ("manifest,-m", po::value<std::string>(), "path to baseline manifest")
        ("dir,-d", po::value<std::string>(), "path to directory to scan");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }
    if (vm["host"].empty() || vm["port"].empty())
    {
        std::cout << "Both host and port should be specified" << std::endl;
        return EXIT_FAILURE;
    }

    const std::string host = vm["host"].as<std::string>();
    const std::string port = vm["port"].as<std::string>();
    const std::string events_path = vm["events"].empty()
        ? std::string("process_events.txt")
        : vm["events"].as<std::string>();
    const std::string manifest_path = vm["manifest"].empty()
        ? std::string("manifest_test.json")
        : vm["manifest"].as<std::string>();
    const std::string directory_path = vm["dir"].empty()
        ? std::string("test_dir")
        : vm["dir"].as<std::string>();

    EventEnumerator event_enumerator {2, events_path};
    std::shared_ptr<BufferRingThreadSafe<std::string>> event_buffer = std::make_shared<BufferRingThreadSafe<std::string>>(20);
    std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> alert_buffer = std::make_shared<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> (80);

    event_enumerator.setBuffer(event_buffer);
    EventDetector event_detector;
    event_detector.setBuffer(event_buffer);
    event_detector.setBufferAleft(alert_buffer);
    FileEstimator file_estimator{ manifest_path, directory_path };
    file_estimator.setBufferAleft(alert_buffer);
    file_estimator.parseManifestFile();

    AlertCollector alert_collector{host, port};
    alert_collector.setBuffer(alert_buffer);

#ifdef _WIN32
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#else
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
#endif
    const std::stop_token app_stop_token = g_app_stop_source.get_token();
    event_enumerator.set_stop_token(app_stop_token);
    file_estimator.set_stop_token(app_stop_token);

    {
        std::jthread thread_enum {[&]()
        {
            event_enumerator.startEnumeration();
        }};
        std::jthread thread_file_estimator{ [&]()
        {
            file_estimator.estimateFilesWithManifest();
        } };
        std::jthread thread_detector {[&]()
        {
            event_detector.start();
        }};
        std::jthread thread_collector{[&]()
        {
            alert_collector.start();
        }};

        // Producers of event_buffer finish -> close it so the detector drains and exits.
        thread_enum.join();
        event_buffer->close();

        // Both detector and estimator produce alerts; wait for them, then close alert_buffer.
        thread_detector.join();
        thread_file_estimator.join();
        alert_buffer->close();

        // Collector drains remaining alerts and exits.
        thread_collector.join();
    }
    return 0;
}
