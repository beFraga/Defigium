cmake_minimum_required(VERSION 3.24)
project(master CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =====================================================================
# 1. DYNAMIC CONFIGURATION PARSING
# =====================================================================
# Allow command line overrides (e.g., cmake -DTYPE=redis ..)
if(NOT DEFINED TYPE)
    if(EXISTS "${CMAKE_SOURCE_DIR}/../../../config.yaml")
        # Read the file natively in CMake
        file(READ "${CMAKE_SOURCE_DIR}/../../../config.yaml" CONFIG_CONTENT)
        
        # Use Regex to extract the string after 'type:'
		string(REGEX MATCH "type:[ \t\r\n]*\"?([a-zA-Z0-9_]+)\"?" _MATCH "${CONFIG_CONTENT}")
        set(TYPE ${CMAKE_MATCH_1})
        
        message(STATUS "[Master Build] Detected TYPE from config.yaml: '${TYPE}'")
    else()
        message(WARNING "config.yaml not found. Master will build with no strategies.")
    endif()
endif()


if (DEFINED TYPE AND NOT TYPE STREQUAL "")
	string(TOUPPER "${TYPE}" TYPE_UPPER)
	add_compile_definitions(BUILD_${TYPE_UPPER}_STRATEGY)
endif()


# =====================================================================
# 2. FETCHCONTENT SETUP
# =====================================================================
include(FetchContent)

# Disable sub-module tests to speed up your JIT compilation
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
)
FetchContent_MakeAvailable(yaml-cpp)
target_compile_options(yaml-cpp PRIVATE "-include" "cstdint")

# =====================================================================
# 3. JIT STRATEGY RESOLUTION
# =====================================================================
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
list(FILTER SOURCES EXCLUDE REGEX "${CMAKE_BINARY_DIR}/*")

add_executable(master ${SOURCES})

# Link standard libraries and yaml-cpp
find_package(Threads REQUIRED)
target_link_libraries(master PRIVATE Threads::Threads yaml-cpp)

# Conditionally link Redis libraries
if(TYPE STREQUAL "redis")
    target_link_libraries(master PRIVATE redis++_static hiredis)
elseif(TYPE STREQUAL "http")
	target_link_libraries(master PRIVATE CURL::libcurl)
endif()
