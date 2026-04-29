#!/bin/bash

echo "Building Multi-Threaded IDM..."

# Create directories
mkdir -p build downloads

# Compile source files
echo "Compiling main.cpp..."
g++ -std=c++17 -Wall -Wextra -pthread -Iinclude -c src/main.cpp -o build/main.o

echo "Compiling minheap.cpp..."
g++ -std=c++17 -Wall -Wextra -pthread -Iinclude -c src/minheap.cpp -o build/minheap.o

echo "Compiling downloader.cpp..."
g++ -std=c++17 -Wall -Wextra -pthread -Iinclude -c src/downloader.cpp -o build/downloader.o

# Link
echo "Linking..."
g++ -std=c++17 -Wall -Wextra -pthread build/main.o build/minheap.o build/downloader.o -o idm -lcurl

if [ $? -eq 0 ]; then
    echo "✓ Build successful! Executable: ./idm"
else
    echo "✗ Build failed!"
    exit 1
fi
