#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "hashmap.h"
using namespace std;

struct ChunkMetadata 
{
    int chunkID;
    long offset;
    int size;
    string hash;
    
    ChunkMetadata() : chunkID(0), offset(0), size(0) {}
    ChunkMetadata(int id, long off, int sz, const string& h) 
        : chunkID(id), offset(off), size(sz), hash(h) {}
};

struct ChunkBlock 
{
    vector<ChunkMetadata> chunks; 
    int startChunkID;      
    int endChunkID;
    
    ChunkBlock() : startChunkID(0), endChunkID(0) {}
};
struct ChunkIndex 
{
    HashMap<int, ChunkBlock*> blockIndex;
    
    int totalChunks;
    
    ChunkIndex() : totalChunks(0) {}
    
    ChunkMetadata* getChunk(int chunkID);
    
    void addChunk(const ChunkMetadata& chunk);
    
    ~ChunkIndex() {
        for (auto it = blockIndex.begin(); it != blockIndex.end(); ++it) {
            auto pair = *it;
            delete pair.second;
        }
    }
};

struct FileMetadata 
{
    string url;
    long fileSize;
    int totalChunks;
    ChunkIndex* chunkIndex;
    
    FileMetadata() : fileSize(0), totalChunks(0), chunkIndex(nullptr) {}
    
    FileMetadata(const FileMetadata& other) 
        : url(other.url), fileSize(other.fileSize), totalChunks(other.totalChunks) 
    {
        if (other.chunkIndex != nullptr) 
        {
            chunkIndex = new ChunkIndex();
            for (int i = 0; i < other.totalChunks; i++) 
            {
                ChunkMetadata* chunk = other.chunkIndex->getChunk(i);
                if (chunk != nullptr) 
                {
                    chunkIndex->addChunk(*chunk);
                }
            }
        }
        else 
        {
            chunkIndex = nullptr;
        }
    }
    
    FileMetadata& operator=(const FileMetadata& other) 
    {
        if (this != &other) 
        {
            url = other.url;
            fileSize = other.fileSize;
            totalChunks = other.totalChunks;
            
            if (chunkIndex != nullptr) 
            {
                delete chunkIndex;
            }
            
            if (other.chunkIndex != nullptr) 
            {
                chunkIndex = new ChunkIndex();
                for (int i = 0; i < other.totalChunks; i++) 
                {
                    ChunkMetadata* chunk = other.chunkIndex->getChunk(i);
                    if (chunk != nullptr) 
                    {
                        chunkIndex->addChunk(*chunk);
                    }
                }
            }
            else 
            {
                chunkIndex = nullptr;
            }
        }
        return *this;
    }
    
    ~FileMetadata() {
        if (chunkIndex != nullptr) {
            delete chunkIndex;
        }
    }
};

struct BTreeNode 
{
    string* keys;
    FileMetadata* values;
    int t;
    BTreeNode** children;
    int n;
    bool isLeaf;
    
    BTreeNode(int degree, bool leaf);
    ~BTreeNode();
    
    void insertNonFull(const string& key, const FileMetadata& value);
    void splitChild(int i, BTreeNode* child);
    FileMetadata* search(const string& key);
    void traverse();
    void printNode(int indent);
};

class BTreeIndex 
{
private:
    BTreeNode* root;
    int t;
    
    void deleteTree(BTreeNode* node);
    
    void saveNodeRecursive(BTreeNode* node, ofstream& file);
    void countNodeRecursive(BTreeNode* node, int& count) const;
    
public:
    BTreeIndex(int degree);
    ~BTreeIndex();
    
    void insert(const string& url, const FileMetadata& metadata);
    FileMetadata* search(const string& url);
    bool exists(const string& url);
    void display();
    void printTree();
    
    bool saveToDisk(const string& filename);
    bool loadFromDisk(const string& filename);
    int getFileCount() const;
};
