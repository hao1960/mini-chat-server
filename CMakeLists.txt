cmake_minimum_required(VERSION 3.10)
project(webServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找到 winsock 库
set(WS2_LIB ws2_32)

add_executable(echo_server src/echo_server.cpp)
target_link_libraries(echo_server PRIVATE ${WS2_LIB})

add_executable(echo_client src/echo_client.cpp)
target_link_libraries(echo_client PRIVATE ${WS2_LIB})
