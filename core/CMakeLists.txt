add_library(CoreLib STATIC
    frameProcessor.cpp
    frameProcessor.h
    iLink.h
)

file(GLOB_RECURSE CORE_UTILS_SOURCES "utils/*.cpp" "utils/*.h")
file(GLOB_RECURSE CORE_UI_SOURCES "ui/*.cpp" "ui/*.h")

target_sources(CoreLib PRIVATE
    ${CORE_UTILS_SOURCES}
    ${CORE_UI_SOURCES}
)

target_include_directories(CoreLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}  
    ${CMAKE_CURRENT_SOURCE_DIR}/ui     
    ${CMAKE_CURRENT_SOURCE_DIR}/utils    
)

target_include_directories(CoreLib PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(CoreLib PROPERTIES
    AUTOUIC ON
    AUTOMOC ON
)

target_link_libraries(CoreLib PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    ${OpenCV_LIBS}
)

