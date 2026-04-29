#!/bin/bash

echo "╔═══════════════════════════════════════════════════════════╗"
echo "║     Building All Components - DSA Final Project          ║"
echo "╔═══════════════════════════════════════════════════════════╗"
echo ""

# Track build status
SUCCESS=0
FAILED=0

# ═══════════════════════════════════════════════════════════
# 1. Build Phase 1: IDM Downloader
# ═══════════════════════════════════════════════════════════
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Building Phase 1: Multi-Threaded IDM Downloader"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

mkdir -p build downloads

echo "  Compiling main.cpp..."
g++ -std=c++17 -Wall -Wextra -pthread -Iinclude -c src/main.cpp -o build/main.o

echo "  Compiling minheap.cpp..."
g++ -std=c++17 -Wall -Wextra -pthread -Iinclude -c src/minheap.cpp -o build/minheap.o

echo "  Compiling downloader.cpp..."
g++ -std=c++17 -Wall -Wextra -pthread -Iinclude -c src/downloader.cpp -o build/downloader.o

echo "  Linking..."
g++ -std=c++17 -Wall -Wextra -pthread build/main.o build/minheap.o build/downloader.o -o idm -lcurl

if [ $? -eq 0 ]; then
    echo "  ✓ IDM Downloader built successfully: ./idm"
    SUCCESS=$((SUCCESS + 1))
else
    echo "  ✗ IDM Downloader build failed!"
    FAILED=$((FAILED + 1))
fi

echo ""

# ═══════════════════════════════════════════════════════════
# 2. Build Phase 2: Deduplication Server
# ═══════════════════════════════════════════════════════════
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Building Phase 2: Deduplication Storage Server"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

mkdir -p server_build server-storage

echo "  Compiling deduplication_engine.cpp..."
g++ -std=c++17 -pthread -c server/deduplication_engine.cpp -o server_build/deduplication_engine.o

echo "  Compiling btree_index.cpp..."
g++ -std=c++17 -pthread -c server/btree_index.cpp -o server_build/btree_index.o

echo "  Compiling storage_manager.cpp..."
g++ -std=c++17 -pthread -c server/storage_manager.cpp -o server_build/storage_manager.o

echo "  Compiling server_main.cpp..."
g++ -std=c++17 -pthread -c server/server_main.cpp -o server_build/server_main.o

echo "  Linking..."
g++ -std=c++17 -pthread server_build/deduplication_engine.o server_build/btree_index.o server_build/storage_manager.o server_build/server_main.o -o server_app -lssl -lcrypto

if [ $? -eq 0 ]; then
    echo "  ✓ Server built successfully: ./server_app"
    SUCCESS=$((SUCCESS + 1))
else
    echo "  ✗ Server build failed!"
    FAILED=$((FAILED + 1))
fi

echo ""

# ═══════════════════════════════════════════════════════════
# 3. Build Phase 2: Client Application
# ═══════════════════════════════════════════════════════════
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Building Phase 2: Client Application"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

mkdir -p build downloads

echo "  Compiling downloader.cpp..."
g++ -std=c++17 -Wall -Wextra -pthread -Iinclude -c src/downloader.cpp -o build/downloader.o

echo "  Compiling minheap.cpp..."
g++ -std=c++17 -Wall -Wextra -pthread -Iinclude -c src/minheap.cpp -o build/minheap.o

echo "  Compiling client_main.cpp..."
g++ -std=c++17 -Wall -Wextra -pthread -Iinclude -c client/client_main.cpp -o build/client_main.o

echo "  Linking..."
g++ -std=c++17 -Wall -Wextra -pthread build/client_main.o build/downloader.o build/minheap.o -o client_app -lcurl -lssl -lcrypto

if [ $? -eq 0 ]; then
    echo "  ✓ Client built successfully: ./client_app"
    SUCCESS=$((SUCCESS + 1))
else
    echo "  ✗ Client build failed!"
    FAILED=$((FAILED + 1))
fi

echo ""

# ═══════════════════════════════════════════════════════════
# Summary
# ═══════════════════════════════════════════════════════════
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║                     BUILD SUMMARY                         ║"
echo "╠═══════════════════════════════════════════════════════════╣"
echo "║  ✓ Successful builds: $SUCCESS                                  ║"
echo "║  ✗ Failed builds: $FAILED                                      ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "🎉 All components built successfully!"
    echo ""
    echo "Usage:"
    echo "  Phase 1 (Downloader):  ./idm <URL>"
    echo "  Phase 2 (Server):      ./server_app"
    echo "  Phase 2 (Client):      ./client_app"
    echo ""
    exit 0
else
    echo "⚠️  Some builds failed. Please check errors above."
    exit 1
fi
