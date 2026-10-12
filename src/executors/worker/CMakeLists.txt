cmake_minimum_required(VERSION 3.24)
project(worker CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

if (DEFINED TYPE AND NOT TYPE STREQUAL "")
	string(TOUPPER "${TYPE}" TYPE_UPPER)
	add_compile_definitions(BUILD_${TYPE_UPPER}_STRATEGY)
endif()


if(TYPE STREQUAL "redis")
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	set(DISABLE_TESTS ON CACHE BOOL "" FORCE)

    # 1. Fetch Hiredis
    FetchContent_Declare(
        hiredis
        GIT_REPOSITORY https://github.com/redis/hiredis.git
        GIT_TAG v1.2.0
		GIT_SHALLOW TRUE
		SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/hiredis_root/hiredis
    )
	FetchContent_MakeAvailable(hiredis)

    # 2. Fetch Redis-Plus-Plus
	set(HIREDIS_HEADER "${CMAKE_BINARY_DIR}/_deps/hiredis_root" CACHE PATH "" FORCE)
	set(HIREDIS_LIB "hiredis" CACHE STRING "" FORCE)

	set(REDIS_PLUS_PLUS_BUILD_TEST OFF CACHE BOOL "" FORCE)
	set(REDIS_PLUS_PLUS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(REDIS_PLUS_PLUS_BUILD_STATIC ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        redis_plus_plus
        GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
        GIT_TAG 1.3.10
		GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(redis_plus_plus)

elseif(TYPE STREQUAL "http")
	find_package(CURL REQUIRED)
endif()

# =====================================================================
# 4. COMPILATION RULES
# =====================================================================
# Recursively find all C++ files (replaces the shell `find` command)
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_BINARY_DIR}/.*")

add_executable(worker ${SOURCES})
target_include_directories(worker PRIVATE ${CMAKE_BINARY_DIR})
target_compile_options(worker PRIVATE -Wall)

find_package(Threads REQUIRED)
target_link_libraries(worker PRIVATE Threads::Threads)

# Conditionally link Redis libraries
if(TYPE STREQUAL "redis")
    target_link_libraries(worker PRIVATE redis++_static hiredis)
elseif(TYPE STREQUAL "http")
	target_link_libraries(worker PRIVATE CURL::libcurl)
endif()
