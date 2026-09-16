cmake_minimum_required(VERSION 3.15)
project(edr_agent VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(FetchContent)

FetchContent_Declare(
    picosha2
    GIT_REPOSITORY https://github.com/okdshin/PicoSHA2
)
FetchContent_MakeAvailable(picosha2)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
     GIT_TAG        v3.8.1
)
FetchContent_MakeAvailable(Catch2)

find_package(Boost REQUIRED CONFIG COMPONENTS program_options)

add_library(edr_core STATIC
    alerts.cpp
    event_enumerator.cpp
    file_detector.cpp
    file_estimator.cpp
    alert_collector.cpp
    listener.cpp
)
target_link_libraries(edr_core PUBLIC
    picosha2
    nlohmann_json::nlohmann_json
    Boost::headers
)

add_executable(edr_agent main.cpp)
target_link_libraries(edr_agent PRIVATE
    edr_core
    Boost::program_options
)

add_executable(edr_unit_test unit_tests_main.cpp)
target_link_libraries(edr_unit_test PRIVATE
    edr_core
    Catch2::Catch2WithMain
)

add_executable(edr_listener listener_main.cpp)
target_link_libraries(edr_listener PRIVATE
    edr_core
    Boost::program_options
)
