cmake_minimum_required(VERSION 3.20)

project(binance_cpp_trading_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(binance_ws
    src/main.cpp
    src/market_data/MarketDataParser.cpp
    src/storage/JsonlWriter.cpp
)

target_compile_definitions(binance_ws
    PRIVATE
        _WIN32_WINNT=0x0A00
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

target_link_libraries(binance_ws
    PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        Boost::system
        nlohmann_json::nlohmann_json
)

if (MSVC)
    target_compile_options(binance_ws PRIVATE /W4 /O2)
else()
    target_compile_options(binance_ws PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()
