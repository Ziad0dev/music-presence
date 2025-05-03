#!/bin/bash

# Ensure we start from the project root directory
cd "$(dirname "$0")"

echo "Cleaning up repository for git..."

# Remove markdown files except README.md
echo "Removing markdown files (except README.md)..."
find . -name "*.md" -not -name "README.md" -type f -delete

# Remove build artifacts
echo "Removing build artifacts..."
rm -rf build/
rm -rf bin/
find . -name "*.o" -type f -delete
find . -name "*.a" -type f -delete
find . -name "*.so" -type f -delete
find . -name "*.lo" -type f -delete
find . -name "*.la" -type f -delete
find . -name "*.dylib" -type f -delete
find . -name "*.exe" -type f -delete
find . -name "*.out" -type f -delete
find . -name "*.app" -type f -delete
find . -name "*.dll" -type f -delete
find . -name "*.lib" -type f -delete
find . -name "*.exp" -type f -delete
find . -name "*.pdb" -type f -delete
find . -name "*.ilk" -type f -delete

# Remove editor/IDE files
echo "Removing editor/IDE files..."
find . -name "*~" -type f -delete
find . -name "*.swp" -type f -delete
find . -name "*.swo" -type f -delete
find . -name ".DS_Store" -type f -delete
find . -name "Thumbs.db" -type f -delete
find . -name ".vscode" -type d -exec rm -rf {} +
find . -name ".idea" -type d -exec rm -rf {} +
find . -name ".vs" -type d -exec rm -rf {} +
find . -name "__pycache__" -type d -exec rm -rf {} +
find . -name "*.pyc" -type f -delete

# Remove any test/example files
echo "Removing test files..."
find . -name "*test*" -type f -delete

# Clean up CMake cache files
echo "Removing CMake cache files..."
find . -name "CMakeCache.txt" -type f -delete
find . -name "CMakeFiles" -type d -exec rm -rf {} +
find . -name "cmake_install.cmake" -type f -delete
find . -name "Makefile.cmake" -type f -delete
find . -name "compile_commands.json" -type f -delete

# Remove any potential sensitive info
echo "Removing potential sensitive info..."
find . -name "*.key" -type f -delete
find . -name "*.pem" -type f -delete
find . -name "*.env" -type f -delete

echo "Cleanup complete!"

# Create .gitignore file
echo "Creating .gitignore file..."
cat > .gitignore << EOF
# Build artifacts
build/
bin/
*.o
*.a
*.so
*.lo
*.la
*.dylib
*.exe
*.out
*.app
*.dll
*.lib
*.exp
*.pdb
*.ilk

# CMake files
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
Makefile
compile_commands.json

# Editor/IDE files
*~
*.swp
*.swo
.DS_Store
Thumbs.db
.vscode/
.idea/
.vs/
__pycache__/
*.pyc

# Project specific
EOF

echo ".gitignore file created!"
echo "Repository is now ready for git!" 