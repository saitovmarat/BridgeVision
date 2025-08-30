cmake_minimum_required(VERSION 3.24)

project(drone-navigation-client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_CLANG_TIDY)
    message(STATUS "Clang-Tidy enabled: ${CMAKE_CXX_CLANG_TIDY}")
    list(APPEND CMAKE_CXX_CLANG_TIDY "--warnings-as-errors=*")
    set(CMAKE_CXX_CLANG_TIDY ${CMAKE_CXX_CLANG_TIDY})
endif()

if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
    message(FATAL_ERROR "Only Windows is supported.")
endif()

if(NOT (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64"))
    message(FATAL_ERROR "Only x64 is supported.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FindDependencies)

add_subdirectory(core)
target_compile_definitions(CoreLib PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
add_subdirectory(net)

set(APPLICATION_SOURCES
    application/main.cpp
    resources/resources.qrc
)

add_executable(${PROJECT_NAME} WIN32 ${APPLICATION_SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE UDP)

set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Project::Dependencies
    CoreLib     
    NetLib
)