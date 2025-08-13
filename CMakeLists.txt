cmake_minimum_required(VERSION 3.24)

project(BridgeVision VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(DEPLOY_DIR "${CMAKE_BINARY_DIR}/Deploy")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${DEPLOY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources ${DEPLOY_DIR}/resources
        COMMENT "📦 Копируем приложение и ресурсы"
    )

    if(TARGET Qt6::Core)
        get_target_property(QT_BIN_DIR Qt6::Core IMPORTED_LOCATION)
        get_filename_component(QT_BIN_DIR "${QT_BIN_DIR}" DIRECTORY)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${QT_BIN_DIR}" ${DEPLOY_DIR}
            COMMENT "🟢 Копируем Qt DLL из ${QT_BIN_DIR}"
        )
    endif()

    if(OpenCV_LIB_DIR)
        get_filename_component(OPENCV_BIN_DIR "${OpenCV_LIB_DIR}/../bin" ABSOLUTE)
        if(EXISTS "${OPENCV_BIN_DIR}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${OPENCV_BIN_DIR}" ${DEPLOY_DIR}
                COMMENT "🔵 Копируем OpenCV DLL из ${OPENCV_BIN_DIR}"
            )
        endif()
    endif()
endif()
