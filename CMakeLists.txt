cmake_minimum_required(VERSION 3.20)
project(exchange-simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

add_executable(exchange src/main.cpp src/order_simulator.cpp src/orderbook.cpp)
target_include_directories(exchange PRIVATE include)

