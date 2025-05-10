cmake_minimum_required(VERSION 3.22)
project(redis-clone)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# server subdirectory
add_subdirectory(server)

# client subdirectory
add_subdirectory(client)
