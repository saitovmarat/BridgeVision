add_library(NetLib STATIC
    udpLink.cpp
    udpLink.h
)

target_include_directories(NetLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(NetLib PROPERTIES
    AUTOMOC ON
)

target_link_libraries(NetLib PRIVATE
    Qt6::Network
    Qt6::Core
    Qt6::Gui
    ${OpenCV_LIBS}
    CoreLib
)
