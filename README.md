# DSA Download Manager with Deduplication Server

Multi-threaded download manager with content-based deduplication server and GUI client.

## 🚀 Quick Start

### Build & Run

```bash
# Build all components
./build_all.sh

# Terminal 1 - Start server
./server_app

# Terminal 2 - Launch GUI
cd ui && ./launch_gui.sh
```

## ✨ Features

- **Multi-threaded Downloads** (8 threads, min-heap scheduling)
- **Content Deduplication** (SHA-256 chunking, saves storage)
- **Custom Data Structures**: HashMap (DJB2), B-Tree Index, Min-Heap, Block Index
- **GUI Client** (tkinter - Download/Upload/Settings tabs)
- **TCP Socket Communication** (Client-Server architecture)

## � Structure

- `idm` - Standalone multi-threaded downloader
- `server_app` - Deduplication server (stores in `server-storage/`)
- `client_app` - CLI client for server operations
- `ui/` - Python GUI client
- `downloads/` - Downloaded files

## 🧪 Test URLs

**Multi-threaded (range support):**
```
http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4  (this link not available anymore)

https://www.w3.org/WAI/ER/tests/xhtml/testfiles/resources/pdf/dummy.pdf
```

**Single-threaded (no range support):**
```
https://files.catbox.moe/wt6b42.mp4
```

## 🔧 Requirements

```bash
# Ubuntu/Debian
sudo apt-get install build-essential libcurl4-openssl-dev libssl-dev python3-tk
```

## � Notes

- First download: Fetches from internet → uploads to server
- Subsequent downloads: Instant retrieval from server (deduplication)
- Duplicate uploads: Server detects and rejects (saves space)


TO run on any other pc it requires the following:
=> python 3 with tinker
=> libcurl
=> OpenSSL

run the following on linux:
sudo apt-get update
sudo apt-get install python3 python3-tk libcurl4 libssl3