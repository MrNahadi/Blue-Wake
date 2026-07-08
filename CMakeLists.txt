cmake_minimum_required(VERSION 3.15)

project(eexi_cii_pi VERSION 0.1.0 LANGUAGES CXX)

option(EEXI_CII_BUILD_TESTS "Build EEXI-CII unit tests" ON)

add_library(eexi_cii_domain STATIC
    src/domain/aer_engine.cpp
    src/domain/cii_reference.cpp
    src/domain/fuel_estimator.cpp
    src/domain/vessel_profile.cpp
)

target_include_directories(eexi_cii_domain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(eexi_cii_domain PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(eexi_cii_domain PRIVATE /W4)
else()
    target_compile_options(eexi_cii_domain PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(eexi_cii_data STATIC
    src/data/accumulator.cpp
    src/data/haversine.cpp
    src/data/nmea_parser.cpp
)

target_include_directories(eexi_cii_data
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(eexi_cii_data PUBLIC cxx_std_17)
target_link_libraries(eexi_cii_data PUBLIC eexi_cii_domain)

if(MSVC)
    target_compile_options(eexi_cii_data PRIVATE /W4)
else()
    target_compile_options(eexi_cii_data PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if(EEXI_CII_BUILD_TESTS)
    add_executable(eexi_cii_domain_tests
        tests/domain_tests.cpp
    )

    target_link_libraries(eexi_cii_domain_tests PRIVATE eexi_cii_domain)
    target_compile_features(eexi_cii_domain_tests PRIVATE cxx_std_17)

    if(MSVC)
        target_compile_options(eexi_cii_domain_tests PRIVATE /W4)
    else()
        target_compile_options(eexi_cii_domain_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME eexi_cii_domain_tests COMMAND eexi_cii_domain_tests)

    add_executable(eexi_cii_aer_engine_tests
        tests/aer_engine_tests.cpp
    )

    target_link_libraries(eexi_cii_aer_engine_tests PRIVATE eexi_cii_domain)
    target_compile_features(eexi_cii_aer_engine_tests PRIVATE cxx_std_17)

    if(MSVC)
        target_compile_options(eexi_cii_aer_engine_tests PRIVATE /W4)
    else()
        target_compile_options(eexi_cii_aer_engine_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME eexi_cii_aer_engine_tests COMMAND eexi_cii_aer_engine_tests)

    add_executable(eexi_cii_data_tests
        tests/data_tests.cpp
    )

    target_link_libraries(eexi_cii_data_tests PRIVATE eexi_cii_data)
    target_compile_features(eexi_cii_data_tests PRIVATE cxx_std_17)

    if(MSVC)
        target_compile_options(eexi_cii_data_tests PRIVATE /W4)
    else()
        target_compile_options(eexi_cii_data_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME eexi_cii_data_tests COMMAND eexi_cii_data_tests)

    add_executable(eexi_cii_accumulator_tests
        tests/accumulator_tests.cpp
    )

    target_link_libraries(eexi_cii_accumulator_tests PRIVATE eexi_cii_data)
    target_compile_features(eexi_cii_accumulator_tests PRIVATE cxx_std_17)

    if(MSVC)
        target_compile_options(eexi_cii_accumulator_tests PRIVATE /W4)
    else()
        target_compile_options(eexi_cii_accumulator_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME eexi_cii_accumulator_tests COMMAND eexi_cii_accumulator_tests)

endif()
