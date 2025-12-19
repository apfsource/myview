cmake_minimum_required(VERSION 3.16)

project(myview LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(myview
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/imagetab.cpp
    src/imagetab.h
    src/myview.qrc
)

target_link_libraries(myview PRIVATE Qt6::Widgets Qt6::Network)

# Install Rules
install(TARGETS myview DESTINATION bin)
install(FILES linux/myview.desktop DESTINATION share/applications)
install(FILES src/logo.png DESTINATION share/icons/hicolor/128x128/apps RENAME myview.png)

# CPack - Packaging Support
set(CPACK_PACKAGE_NAME "myview")
set(CPACK_PACKAGE_VENDOR "MyView Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A modern and fast image viewer")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "user@example.com")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6widgets6, libqt6network6, libqt6core6, libqt6gui6")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/linux/myview.desktop") # Placeholder or real license

include(CPack)
