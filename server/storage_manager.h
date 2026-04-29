#pragma once

#include <string>
#include <mutex>

using namespace std;

class StorageManager 
{
private:
    string storageFile;
    int fd;
    long currentOffset;
    mutable mutex mtx;
    
public:
    StorageManager(const string& filename);
    ~StorageManager();
    
    long saveChunk(const char* data, int size);
    void readChunk(long offset, int size, char* buffer);
    long getCurrentSize();
};
