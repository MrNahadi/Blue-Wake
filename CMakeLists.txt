cmake_minimum_required(VERSION 3.15)

project(eexi_cii_pi VERSION 0.1.0 LANGUAGES CXX)

option(EEXI_CII_BUILD_TESTS "Build EEXI-CII unit tests" ON)
option(EEXI_CII_BUILD_OPENCPN_PLUGIN "Build the OpenCPN plugin wrapper" OFF)

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
    src/data/persistence.cpp
)

target_include_directories(eexi_cii_data
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(eexi_cii_data PUBLIC cxx_std_17)
target_link_libraries(eexi_cii_data PUBLIC eexi_cii_domain)
target_include_directories(eexi_cii_data
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/opencpn-libs/json-schema-validator
)

if(MSVC)
    target_compile_options(eexi_cii_data PRIVATE /W4)
else()
    target_compile_options(eexi_cii_data PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(eexi_cii_config STATIC
    src/config/profile_settings.cpp
)

target_include_directories(eexi_cii_config
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(eexi_cii_config PUBLIC cxx_std_17)
target_link_libraries(eexi_cii_config PUBLIC eexi_cii_domain)

if(MSVC)
    target_compile_options(eexi_cii_config PRIVATE /W4)
else()
    target_compile_options(eexi_cii_config PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(eexi_cii_export STATIC
    src/export/csv_exporter.cpp
)

target_include_directories(eexi_cii_export
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(eexi_cii_export PUBLIC cxx_std_17)
target_link_libraries(eexi_cii_export PUBLIC eexi_cii_data)

if(MSVC)
    target_compile_options(eexi_cii_export PRIVATE /W4)
else()
    target_compile_options(eexi_cii_export PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(eexi_cii_plugin_core STATIC
    src/plugin/plugin_core.cpp
)

target_include_directories(eexi_cii_plugin_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(eexi_cii_plugin_core PUBLIC cxx_std_17)
target_link_libraries(eexi_cii_plugin_core PUBLIC eexi_cii_export eexi_cii_config)

if(MSVC)
    target_compile_options(eexi_cii_plugin_core PRIVATE /W4)
else()
    target_compile_options(eexi_cii_plugin_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EEXI_CII_BUILD_OPENCPN_PLUGIN)
    find_package(wxWidgets REQUIRED COMPONENTS base core)
    include(${wxWidgets_USE_FILE})

    add_subdirectory(opencpn-libs/api-21)

    set_property(TARGET
        eexi_cii_domain
        eexi_cii_data
        eexi_cii_config
        eexi_cii_export
        eexi_cii_plugin_core
        PROPERTY POSITION_INDEPENDENT_CODE ON
    )

    add_library(eexi_cii_pi MODULE
        src/opencpn/eexi_cii_pi.cpp
    )

    target_include_directories(eexi_cii_pi
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_compile_features(eexi_cii_pi PRIVATE cxx_std_17)
    target_compile_definitions(eexi_cii_pi PRIVATE MAKING_PLUGIN)
    target_link_libraries(eexi_cii_pi
        PRIVATE
            eexi_cii_plugin_core
            ocpn::api
            ${wxWidgets_LIBRARIES}
    )

    set_target_properties(eexi_cii_pi PROPERTIES
        PREFIX ""
    )

    if(MSVC)
        target_compile_options(eexi_cii_pi PRIVATE /W4)
    else()
        target_compile_options(eexi_cii_pi PRIVATE -Wall -Wextra -Wpedantic)
    endif()
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

    target_link_libraries(eexi_cii_data_tests PRIVATE eexi_cii_export eexi_cii_config)
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

    target_link_libraries(eexi_cii_accumulator_tests PRIVATE eexi_cii_plugin_core)
    target_compile_features(eexi_cii_accumulator_tests PRIVATE cxx_std_17)

    if(MSVC)
        target_compile_options(eexi_cii_accumulator_tests PRIVATE /W4)
    else()
        target_compile_options(eexi_cii_accumulator_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME eexi_cii_accumulator_tests COMMAND eexi_cii_accumulator_tests)

endif()
