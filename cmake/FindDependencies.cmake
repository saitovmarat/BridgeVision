if(DEFINED ENV{QT_DIR})
    set(Qt6_DIR "$ENV{QT_DIR}/lib/cmake/Qt6")
else()
    message(FATAL_ERROR "QT_DIR environment variable is not set.")
endif()

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    MultimediaWidgets
    Network
)

find_package(OpenCV REQUIRED COMPONENTS
    core
    imgproc
    highgui
    videoio
    imgcodecs
)

message(STATUS "Qt6::Core include dirs: ${Qt6Core_INCLUDE_DIRS}")

if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found.")
endif()

add_library(Project::QtCore INTERFACE IMPORTED)
set_target_properties(Project::QtCore PROPERTIES
    INTERFACE_LINK_LIBRARIES Qt6::Core
)

add_library(Project::QtNetwork INTERFACE IMPORTED)
set_target_properties(Project::QtNetwork PROPERTIES
    INTERFACE_LINK_LIBRARIES Qt6::Network
)


add_library(Project::Dependencies INTERFACE IMPORTED)
target_link_libraries(Project::Dependencies INTERFACE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Network
    ${OpenCV_LIBS}  
)
