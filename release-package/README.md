# DSA Download Manager - Release Package

## How to Run

Simply execute:
```bash
cd ui
./launch_gui.sh
```

## What's Included

- **client_app** - Upload/download client for server communication
- **idm** - Multi-threaded downloader for direct internet downloads
- **ui/** - GUI application files
- **downloads/** - Downloaded files will be saved here

## Requirements

- Python 3 with tkinter
- Linux system

## Usage

1. Run `cd ui && ./launch_gui.sh`
2. Configure server settings in Settings tab
3. Upload files or download from URLs

## Features

- Multi-threaded downloads
- Content-based deduplication
- B-Tree file indexing
- Smart download mode (checks server first, then internet)
