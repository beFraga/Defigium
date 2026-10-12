cmake_minimum_required(VERSION 3.10)
project(daemon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(daemon main.cpp Daemon.cpp)

target_compile_options(daemon PRIVATE -Wall -Wextra)
find_package(Threads REQUIRED)
target_link_libraries(daemon PRIVATE Threads::Threads)

