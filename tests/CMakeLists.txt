# ═══════════════════════════════════════════════════════════════════════════════════════════════════
# Test Sources
# ═══════════════════════════════════════════════════════════════════════════════════════════════════

set(TEST_SOURCES
        test_gases.cpp
        test_linda.cpp
        test_monstermos.cpp
        test_hotspot.cpp
        test_superconductivity.cpp
        test_reactions.cpp
        test_integration.cpp
        test_performance.cpp
        test_simd.cpp
)


# ═══════════════════════════════════════════════════════════════════════════════════════════════════
# Test Executable
# ═══════════════════════════════════════════════════════════════════════════════════════════════════

add_executable(atmos_tests ${TEST_SOURCES})

target_include_directories(atmos_tests PRIVATE ../include)

target_link_libraries(atmos_tests
        PRIVATE
        GTest::gtest_main
        atmos_native
)

if(WIN32)
    add_custom_command(TARGET atmos_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:atmos_native>
            $<TARGET_FILE_DIR:atmos_tests>
    )
endif()

# ═══════════════════════════════════════════════════════════════════════════════════════════════════
# GoogleTest Integration
# ═══════════════════════════════════════════════════════════════════════════════════════════════════

include(GoogleTest)
gtest_discover_tests(atmos_tests)
