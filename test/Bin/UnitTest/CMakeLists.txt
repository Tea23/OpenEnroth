cmake_minimum_required(VERSION 3.20.4 FATAL_ERROR)

if(ENABLE_TESTS)
    set(UNIT_TEST_MAIN_SOURCES UnitTestMain.cpp)

    add_executable(OpenEnroth_UnitTest ${UNIT_TEST_MAIN_SOURCES})
    target_fix_libcxx_assertions(OpenEnroth_UnitTest)
    target_link_libraries(OpenEnroth_UnitTest unit_test)

    add_custom_target(UnitTest OpenEnroth_UnitTest
        DEPENDS OpenEnroth_UnitTest
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    target_check_style(OpenEnroth_UnitTest)
endif()
