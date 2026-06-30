cmake_minimum_required(VERSION 3.20)
project(exchange-simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(exchange src/main.cpp src/order_simulator.cpp src/orderbook.cpp src/matching_engine.cpp src/binary_serializer.cpp src/exchange_server.cpp src/network_utils.cpp src/order_processor.cpp)
target_include_directories(exchange PRIVATE include)

add_executable(client src/client_main.cpp src/exchange_client.cpp src/order_simulator.cpp src/binary_serializer.cpp src/network_utils.cpp)
target_include_directories(client PRIVATE include)


#testing
enable_testing()
find_package(GTest REQUIRED)

add_executable(tests 
tests/test_order.cpp 
tests/test_orderbook.cpp 
tests/test_matching_engine.cpp 
tests/test_serializer.cpp 
tests/test_ring_buffer.cpp 

src/orderbook.cpp 
src/matching_engine.cpp 
src/binary_serializer.cpp 
src/order_simulator.cpp
src/network_utils.cpp
)
target_include_directories(tests PRIVATE include)
target_link_libraries(tests GTest::GTest GTest::Main)

add_test(Name exchange_tests COMMAND tests)