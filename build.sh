#!/bin/bash

# Ensure we start from the project root directory
cd "$(dirname "$0")"

# Clean up existing build
rm -rf build
mkdir -p build
cd build

# Configure and build
if command -v cmake &> /dev/null; then
    echo "Using CMake to build"
    cmake .. || {
        echo "CMake configuration failed! Falling back to manual build"
        manual_build
        exit 1
    }
    make || {
        echo "Make failed! Falling back to manual build"
        cd ..
        manual_build
        exit 1
    }
else
    echo "CMake not found, using manual build"
    cd ..
    manual_build
fi

function manual_build() {
    echo "Attempting manual build with g++"
    mkdir -p bin
    
    # Try to build with g++ directly
    echo "Building main executable..."
    g++ -o bin/music-presence \
      src/main.cpp \
      src/application.cpp \
      src/discord/discord_rpc.cpp \
      src/media/mpris_detector.cpp \
      src/ui/systray.cpp \
      src/settings/settings_manager.cpp \
      -Isrc/ \
      -std=c++17 \
      -lQt5Core -lQt5Widgets -lQt5Network -lQt5DBus -ldiscord-rpc
}

# Provide usage instructions
echo ""
echo "Music Presence build complete. Run with: ./build/music-presence"
echo ""
echo "Make sure you:"
echo "1. Have Discord running"
echo "2. Are playing music in a supported player (Spotify, VLC, etc.)"
echo "3. Have discord-rpc library installed"
echo "" 
