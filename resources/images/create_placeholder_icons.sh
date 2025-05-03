#!/bin/bash

# Create a placeholder icon using convert (ImageMagick)
create_icon() {
    local name=$1
    local color=$2
    local output=$3
    
    convert -size 128x128 xc:$color -fill white -gravity center \
        -pointsize 18 -annotate 0 "$name" "$output"
}

# Create app icons
create_icon "Music Presence" "#5865F2" "app-icon.png"
create_icon "Active" "#57F287" "app-icon-active.png"
create_icon "Inactive" "#ED4245" "app-icon-inactive.png"

# Create player icons
mkdir -p player-icons
create_icon "Spotify" "#1DB954" "player-icons/spotify.png"
create_icon "VLC" "#FF8800" "player-icons/vlc.png"
create_icon "Rhythmbox" "#4A86CF" "player-icons/rhythmbox.png"
create_icon "Default" "#888888" "player-icons/default.png"

echo "Placeholder icons created" 