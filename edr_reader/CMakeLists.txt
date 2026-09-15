cmake_minimum_required(VERSION 3.12)
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

add_executable(edr_agent main.cpp)
target_link_libraries(edr_agent PRIVATE picosha2 nlohmann_json::nlohmann_json)

add_executable(edr_unit_test unit_tests_main.cpp)
target_link_libraries(edr_unit_test PRIVATE  Catch2::Catch2WithMain)
