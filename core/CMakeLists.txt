add_library(CoreLib STATIC)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "frameProcessor.cpp"
    "iLink.cpp"
    "iLink.h"
    "utils/*.cpp"
    "utils/*.h"
    "ui/*.cpp"
    "ui/*.h"
)

target_sources(CoreLib PRIVATE ${CORE_SOURCES})

target_include_directories(CoreLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui     
    ${CMAKE_CURRENT_SOURCE_DIR}/utils    
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

