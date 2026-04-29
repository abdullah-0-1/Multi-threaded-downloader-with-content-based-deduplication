#pragma once

#include <string>
#include "hashmap.h"
#include <mutex>

using namespace std;

class DeduplicationEngine 
{
private:
    HashMap<string, long> hashMap;
    mutable mutex mtx;
    
public:
    DeduplicationEngine();
    ~DeduplicationEngine();
    
    bool exists(const string& hash);
    long getOffset(const string& hash);
    void insert(const string& hash, long offset);
    int size();
    void clear();
    
    bool saveToDisk(const string& filename);
    bool loadFromDisk(const string& filename);
};
