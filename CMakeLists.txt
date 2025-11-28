cmake_minimum_required(VERSION 4.0)

# ═══════════════════════════════════════════════════════════════════════════════════════════════════
# Project
# ═══════════════════════════════════════════════════════════════════════════════════════════════════

project(atmos_native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# ═══════════════════════════════════════════════════════════════════════════════════════════════════
# Compiler Options
# ═══════════════════════════════════════════════════════════════════════════════════════════════════

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /arch:AVX2 /fp:fast")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mavx2 -mfma -ffast-math -fno-exceptions -fno-rtti")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()


# ═══════════════════════════════════════════════════════════════════════════════════════════════════
# Sources
# ═══════════════════════════════════════════════════════════════════════════════════════════════════

set(SOURCES
        src/atmos_core.cpp
        src/linda.cpp
        src/monstermos.cpp
        src/hotspot.cpp
        src/superconductivity.cpp
        src/reactions.cpp
        src/gases.cpp
)


# ═══════════════════════════════════════════════════════════════════════════════════════════════════
# Library Target
# ═══════════════════════════════════════════════════════════════════════════════════════════════════

add_library(atmos_native SHARED ${SOURCES})

target_include_directories(atmos_native PUBLIC include)

set_target_properties(atmos_native PROPERTIES
        PREFIX ""
        OUTPUT_NAME "atmos_native"
)


# ═══════════════════════════════════════════════════════════════════════════════════════════════════
# Output Directories
# ═══════════════════════════════════════════════════════════════════════════════════════════════════

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(CMAKE_CONFIGURATION_TYPES)
    foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${cfg} CFGU)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFGU} ${CMAKE_BINARY_DIR}/bin)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFGU} ${CMAKE_BINARY_DIR}/lib)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFGU} ${CMAKE_BINARY_DIR}/lib)
    endforeach()
endif()

# ═══════════════════════════════════════════════════════════════════════════════════════════════════
# Tests
# ═══════════════════════════════════════════════════════════════════════════════════════════════════

option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()