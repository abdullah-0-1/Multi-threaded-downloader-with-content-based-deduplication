#!/bin/bash

# Distributed Download Manager - GUI Launcher
# Quick launch script for the Python GUI

echo "=========================================="
echo " Distributed Download Manager - GUI     "
echo "=========================================="
echo ""

# Check if Python3 is installed
if ! command -v python3 &> /dev/null; then
    echo "❌ Error: Python 3 is not installed"
    echo "   Install it with: sudo apt-get install python3"
    exit 1
fi

# Check if tkinter is installed
if ! python3 -c "import tkinter" &> /dev/null; then
    echo "❌ Error: tkinter is not installed"
    echo "   Install it with: sudo apt-get install python3-tk"
    exit 1
fi

# Check if binaries are built
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

if [ ! -f "$PROJECT_ROOT/client_app" ]; then
    echo "⚠️  Warning: client_app not found"
    echo "   Build it first: ./build_all.sh"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Launch GUI
echo "✓ Launching GUI..."
echo ""

cd "$PROJECT_ROOT"
python3 ui/main.py

echo ""
echo "GUI closed."
