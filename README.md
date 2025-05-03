# Discord Music Presence

Display your currently playing music in Discord Rich Presence.

## Key Features

- Detects music playing from any MPRIS-compatible media player (Spotify, VLC, Rhythmbox, etc.)
- Shows artist, title, and album in your Discord status
- Robust handling of player changes and track transitions
- Smart debouncing to prevent rapid updates
- Delayed presence clearing (waits 30 seconds after pausing before clearing)

## Requirements

- Linux system with D-Bus
- Qt5 (Core, Widgets, Network, DBus)
- Discord desktop client
- Discord Rich Presence library (discord-rpc)

## Building

### Method 1: Using build script

```bash
# Make the script executable
chmod +x build.sh

# Run the build script
./build.sh
```

### Method 2: Manual build with CMake

```bash
# Create build directory
mkdir -p build
cd build

# Configure and build
cmake ..
make
```

### Method 3: Direct compilation with g++

```bash
mkdir -p bin
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
```

## Running

1. Make sure Discord is running
2. Start a music player (Spotify, VLC, etc.)
3. Run the application:
   ```bash
   ./build/music-presence
   ```
   or
   ```bash
   ./bin/music-presence
   ```

## Troubleshooting

### CMake errors

If you see errors like:
```
CMake Error: Could not find CMAKE_ROOT !!!
```

Try the following:
```bash
cd ~
rm -rf ~/.local/share/cmake-3.28
sudo apt-get remove --purge cmake
sudo apt-get autoremove
sudo apt-get clean
sudo apt-get install cmake
```

### Discord connection issues

- Ensure Discord is running
- Check that the App ID is correct in `src/discord/discord_rpc.cpp`
- Verify discord-rpc library is installed

### QDBusRawType errors or crashes

The application includes robust error handling for DBus communication. If you still experience crashes, please file an issue with details about your system and music player. 