cmake_minimum_required(VERSION 3.12)
project(MusicPresence VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(Qt5 COMPONENTS Core Widgets Network DBus REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

# Find Discord RPC
option(BUILD_DISCORD_RPC "Build with Discord RPC support" ON)

if(BUILD_DISCORD_RPC)
    # Try to find the library using pkg-config
    pkg_check_modules(DISCORD_RPC discord-rpc)
    
    # If not found, we'll use the bundled version
    if(NOT DISCORD_RPC_FOUND)
        message(STATUS "Discord RPC not found via pkg-config, will download it")
        include(FetchContent)
        FetchContent_Declare(
            discord-rpc
            GIT_REPOSITORY https://github.com/discord/discord-rpc.git
            GIT_TAG v3.4.0
        )
        FetchContent_MakeAvailable(discord-rpc)
        set(DISCORD_RPC_INCLUDE_DIRS ${discord-rpc_SOURCE_DIR}/include)
        set(DISCORD_RPC_LIBRARIES discord_rpc_static)
    endif()
    
    add_definitions(-DUSE_DISCORD_RPC)
endif()

# Set source files
set(SOURCES
    src/main.cpp
    src/application.cpp
    src/discord/discord_rpc.cpp
    src/media/mpris_detector.cpp
    src/ui/systray.cpp
    src/settings/settings_manager.cpp
)

# Set header files
set(HEADERS
    src/application.h
    src/discord/discord_rpc.h
    src/media/mpris_detector.h
    src/ui/systray.h
    src/settings/settings_manager.h
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${DBUS_INCLUDE_DIRS}
)

if(BUILD_DISCORD_RPC)
    include_directories(${DISCORD_RPC_INCLUDE_DIRS})
endif()

# Automatically run Qt's moc, uic, and rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add resources
set(RESOURCES
    resources/resources.qrc
)

# Create executable
add_executable(music-presence ${SOURCES} ${HEADERS} ${RESOURCES})

# Link libraries
target_link_libraries(music-presence PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::DBus
    ${DBUS_LIBRARIES}
)

if(BUILD_DISCORD_RPC)
    target_link_libraries(music-presence PRIVATE ${DISCORD_RPC_LIBRARIES})
endif()

# Install targets
install(TARGETS music-presence DESTINATION bin)
install(FILES resources/music-presence.desktop DESTINATION share/applications)
install(FILES resources/images/app-icon.png DESTINATION share/icons/hicolor/128x128/apps RENAME music-presence.png)

# Create Debian package
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "music-presence")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Discord Music Presence for Linux")
set(CPACK_PACKAGE_CONTACT "your-email@example.com")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5widgets5, libqt5network5, libqt5dbus5, libdbus-1-3")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
include(CPack) 