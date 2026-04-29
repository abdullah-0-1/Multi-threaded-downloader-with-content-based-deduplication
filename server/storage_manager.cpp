#include "storage_manager.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

using namespace std;

StorageManager::StorageManager(const string& filename) 
{
    storageFile = filename;
    
    fd = open(storageFile.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) 
    {
        cerr << "Error: Failed to open storage file: " << storageFile << endl;
        return;
    }
    
    struct stat st;
    if (fstat(fd, &st) == 0) 
    {
        lock_guard<mutex> lock(mtx);
        currentOffset = st.st_size;
    }
    else 
    {
        lock_guard<mutex> lock(mtx);
        currentOffset = 0;
    }
    
    cout << "StorageManager initialized: " << storageFile 
         << " (size: " << currentOffset << " bytes)" << endl;
}

StorageManager::~StorageManager() 
{
    if (fd >= 0) 
    {
        close(fd);
    }
}

long StorageManager::saveChunk(const char* data, int size) 
{
    if (fd < 0 || data == nullptr || size <= 0) 
    {
        return -1;
    }
    
    long chunkOffset;
    {
        lock_guard<mutex> lock(mtx);
        chunkOffset = currentOffset;
        currentOffset += size;
    }
    
    ssize_t written = pwrite(fd, data, size, chunkOffset);
    if (written != size) 
    {
        cerr << "Error: Failed to write chunk" << endl;
        return -1;
    }
    
    return chunkOffset;
}

void StorageManager::readChunk(long offset, int size, char* buffer) 
{
    if (fd < 0 || buffer == nullptr || size <= 0) 
    {
        return;
    }
    
    ssize_t bytesRead = pread(fd, buffer, size, offset);
    if (bytesRead != size) 
    {
        cerr << "Error: Failed to read chunk at offset " << offset << endl;
    }
}

long StorageManager::getCurrentSize() 
{
    lock_guard<mutex> lock(mtx);
    return currentOffset;
}
