cmake_minimum_required(VERSION 3.15)
project(net_calc C)

set(CMAKE_C_STANDARD 99)

add_executable(net_calc client.c server.c)