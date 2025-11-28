cmake_minimum_required(VERSION 4.0)
project(atmos_native LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /arch:AVX2 /fp:fast")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mavx2 -mfma -ffast-math -fno-exceptions -fno-rtti")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

set(SOURCES
        src/atmos_core.cpp
        src/linda.cpp
        src/monstermos.cpp
        src/hotspot.cpp
        src/superconductivity.cpp
        src/reactions.cpp
        src/gases.cpp
)

add_library(atmos_native SHARED ${SOURCES})

target_include_directories(atmos_native PUBLIC include)

set_target_properties(atmos_native PROPERTIES
        PREFIX ""
        OUTPUT_NAME "atmos_native"
)

option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(tests)
endif()