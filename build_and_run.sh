#!/bin/bash

set -e

# Function to check if a package is installed
is_pkg_installed() {
    dpkg -s "$1" >/dev/null 2>&1
}

# Install required dependencies
install_dependencies() {
    echo "Checking for required dependencies..."
    
    REQUIRED_PKGS=(
        "build-essential"
        "cmake"
        "qtbase5-dev"
        "qttools5-dev"
        "libqt5dbus5"
        "qtdeclarative5-dev"
        "libdbus-1-dev"
        "pkg-config"
    )
    
    PKGS_TO_INSTALL=()
    
    for pkg in "${REQUIRED_PKGS[@]}"; do
        if ! is_pkg_installed "$pkg"; then
            PKGS_TO_INSTALL+=("$pkg")
        fi
    done
    
    if [ ${#PKGS_TO_INSTALL[@]} -gt 0 ]; then
        echo "The following packages need to be installed: ${PKGS_TO_INSTALL[*]}"
        echo "Please run: sudo apt-get install ${PKGS_TO_INSTALL[*]}"
        echo "Then run this script again."
        exit 1
    fi
    
    echo "All dependencies are installed."
}

# Build the project
build_project() {
    echo "Building the project..."
    
    # Create build directory if it doesn't exist
    mkdir -p build
    cd build
    
    # Configure
    cmake ..
    
    # Build
    make -j$(nproc)
    
    echo "Build completed successfully."
}

# Run the application
run_application() {
    echo "Running Music Presence..."
    
    # Run from the build directory
    cd build
    ./music-presence
}

# Main
main() {
    # Check dependencies
    install_dependencies
    
    # Build project
    build_project
    
    # Ask if the user wants to run the application
    read -p "Do you want to run the application now? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        run_application
    fi
}

# Run main function
main 