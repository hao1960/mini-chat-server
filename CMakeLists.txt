cmake_minimum_required(VERSION 3.10)
project(webServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(echo_server src/echo_server.cpp)
add_executable(echo_client src/echo_client.cpp)
