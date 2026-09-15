#include <iostream>
#include <string>
#include <vector>
#include <ranges>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <random>

#include "event_enumerator.hpp"
#include "file_detector.hpp"
#include "file_estimator.hpp"
#include "alerts.hpp"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char** argv) {
    po::options_description desc("EDR APP options");
    desc.add_options()
        ("help", "produce help message")
        ("host,-h", po::value<std::string>(), "set host to send report")
        ("port,-p", po::value<std::string>(), "set port for host to send report");

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

    EventEnumerator event_enumerator {2 ,"d:/1eye/NCOT/edr_test/process_events.txt"};
    std::shared_ptr<BufferRingThreadSafe<std::string>> event_buffer = std::make_shared<BufferRingThreadSafe<std::string>>(20);
    std::shared_ptr<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> alert_buffer = std::make_shared<BufferRingThreadSafe<std::unique_ptr<EDR_AlertBase>>> (80);

    event_enumerator.setBuffer(event_buffer);
    EventDetector event_detector;
    event_detector.setBuffer(event_buffer);
    event_detector.setBufferAleft(alert_buffer);
    FileEstimator file_estimator{ "d:/1eye/NCOT/edr_test/manifest_test.json", "d:/1eye/NCOT/edr_test/test_dir" };
    file_estimator.setBufferAleft(alert_buffer);
    file_estimator.parseManifestFile();
    {
        std::jthread thread_enum {[&]()
        {
            event_enumerator.startEnumeration();
        }};
        std::jthread thread_detector {[&]()
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            event_detector.start();
        }};
        std::jthread thread_file_estimator{ [&]()
        {
            file_estimator.estimateFilesWithManifest();
        } };

    }
    // event_enumerator.setBuffer(event_buffer);
    // event_enumerator.startEnumeration();
    // while (event_buffer->hasElements())
    // {
    //     std::cout<<event_buffer->pop()<<std::endl;
    // }


    
     //file_estimator.createNewManifest("d:/1eye/NCOT/edr_test/test_dir",
     //    "d:/1eye/NCOT/edr_test/manifest_test.json");

    // ProcessAlert p_alert;
    // p_alert.m_type = ALERT_TYPE::SuspicioutActivityAlert;
    // p_alert.m_pid = "777";
    // p_alert.m_pids.push_back("778");
    // p_alert.m_pids.push_back("779");
    // p_alert.m_pids.push_back("780");
    // p_alert.m_pids.push_back("781");
    // p_alert.m_pids.push_back("782");
    // p_alert.m_period_start = std::chrono::system_clock::now()-std::chrono::seconds(10);
    // p_alert.m_period_end = std::chrono::system_clock::now();
    // std::cout<< p_alert.toJSON()<<'\n';
    // std::cout<< p_alert.toString()<<'\n';

    // FileAlert f_alert;
    // f_alert.m_type = ALERT_TYPE::FileAdded;
    // f_alert.m_file_name= "some_file_name.txt";
    // std::cout<< f_alert.toJSON()<<'\n';
    // std::cout<< f_alert.toString()<<'\n';
    std::cout<<alert_buffer->pop()->toJSON()<<'\n';
    std::cout<<alert_buffer->size()<<'\n';
    return 0;
}
