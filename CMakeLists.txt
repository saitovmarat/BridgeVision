cmake_minimum_required(VERSION 3.24)

project(BridgeVision VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM "win")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR "macOS is not supported for this project.")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    set(ARCHITECTURE "x64")
else()
    message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(DEFINED ENV{QT_DIR})
    set(Qt6_DIR "$ENV{QT_DIR}/lib/cmake/Qt6")
else()
    message(FATAL_ERROR "No Qt env variable!")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Multimedia
    MultimediaWidgets
)

find_package(OpenCV REQUIRED COMPONENTS
    core
    imgproc
    highgui
    videoio
    imgcodecs
)

message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")
message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIB_DIR: ${OpenCV_LIB_DIR}")
message(STATUS "OpenCV_VERSION: ${OpenCV_VERSION}")

set(OpenCV_LIB_DIR "${OpenCV_LIB_DIR}/lib")
message(STATUS "OpenCV_LIB_DIR: ${OpenCV_LIB_DIR}")

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV не найден")
endif()

add_definitions(-DPROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

set(PROJECT_SOURCES
    src/ui/mainWindow.ui
    src/ui/mainWindow.cpp
    src/ui/mainWindow.h

    src/core/videoPlayer.cpp
    src/core/videoPlayer.h
    src/core/frameProcessor.h
    src/core/frameProcessor.cpp

    src/utils/imageConverter.h
    src/utils/imageConverter.cpp
    src/utils/detectionsDrawer.h
    src/utils/detectionsDrawer.cpp

    main.cpp
)

set(RESOURCES resources/resources.qrc)

add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES} ${RESOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    src/ui
    src/core
    src/utils
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ${OpenCV_LIBS}
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

# ${QT_VERSION_MAJOR}
