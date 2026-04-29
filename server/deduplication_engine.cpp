#include "deduplication_engine.h"
#include <iostream>
#include <fstream>

DeduplicationEngine::DeduplicationEngine() 
{
}

DeduplicationEngine::~DeduplicationEngine() 
{
    lock_guard<mutex> lock(mtx);
    hashMap.clear();
}

bool DeduplicationEngine::exists(const string& hash) 
{
    lock_guard<mutex> lock(mtx);
    return hashMap.contains(hash);
}

long DeduplicationEngine::getOffset(const string& hash) 
{
    lock_guard<mutex> lock(mtx);
    long* result = hashMap.find(hash);
    if (result != nullptr) 
    {
        return *result;
    }
    return -1;
}

void DeduplicationEngine::insert(const string& hash, long offset) 
{
    lock_guard<mutex> lock(mtx);
    hashMap[hash] = offset;
}

int DeduplicationEngine::size() 
{
    lock_guard<mutex> lock(mtx);
    return hashMap.size();
}

void DeduplicationEngine::clear() 
{
    lock_guard<mutex> lock(mtx);
    hashMap.clear();
}

bool DeduplicationEngine::saveToDisk(const string& filename) 
{
    lock_guard<mutex> lock(mtx);
    
    ofstream file(filename, ios::binary);
    if (!file.is_open()) 
    {
        cerr << "Error: Cannot open " << filename << " for writing" << endl;
        return false;
    }
    
    int count = hashMap.size();
    file.write((char*)&count, sizeof(count));
    
    for (const auto& pair : hashMap) 
    {
        int hashLen = pair.first.length();
        file.write((char*)&hashLen, sizeof(hashLen));
        file.write(pair.first.c_str(), hashLen);
        file.write((char*)&pair.second, sizeof(pair.second));
    }
    
    file.close();
    return true;
}

bool DeduplicationEngine::loadFromDisk(const string& filename) 
{
    lock_guard<mutex> lock(mtx);
    
    ifstream file(filename, ios::binary);
    if (!file.is_open()) 
    {
        return false;
    }
    
    int count;
    file.read((char*)&count, sizeof(count));
    
    cout << "  Loading " << count << " chunk hashes from disk..." << endl;
    
    for (int i = 0; i < count; i++) 
    {
        int hashLen;
        file.read((char*)&hashLen, sizeof(hashLen));
        
        char* hashBuf = new char[hashLen + 1];
        file.read(hashBuf, hashLen);
        hashBuf[hashLen] = '\0';
        string hash(hashBuf);
        delete[] hashBuf;
        
        long offset;
        file.read((char*)&offset, sizeof(offset));
        
        hashMap[hash] = offset;
        
        if ((i + 1) % 1000 == 0 || i == count - 1) 
        {
            cout << "\r  Loaded: " << (i + 1) << "/" << count << " hashes" << flush;
        }
    }
    
    cout << endl;
    file.close();
    return true;
}
