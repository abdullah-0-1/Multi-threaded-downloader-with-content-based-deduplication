#include "btree_index.h"
#include <fstream>

#define BLOCK_SIZE 100

ChunkMetadata* ChunkIndex::getChunk(int chunkID) 
{
    if (chunkID < 0 || chunkID >= totalChunks) 
    {
        return nullptr;
    }
    
    int blockID = chunkID / BLOCK_SIZE;
    ChunkBlock** result = blockIndex.find(blockID);
    
    if (result != nullptr) 
    {
        ChunkBlock* block = *result;
        int offsetInBlock = chunkID % BLOCK_SIZE;
        
        if (offsetInBlock < block->chunks.size()) 
        {
            return &(block->chunks[offsetInBlock]);
        }
    }
    
    return nullptr;
}

void ChunkIndex::addChunk(const ChunkMetadata& chunk) 
{
    int blockID = totalChunks / BLOCK_SIZE;
    
    if (blockIndex.find(blockID) == nullptr) 
    {
        blockIndex[blockID] = new ChunkBlock();
        blockIndex[blockID]->startChunkID = blockID * BLOCK_SIZE;
    }
    
    blockIndex[blockID]->chunks.push_back(chunk);
    blockIndex[blockID]->endChunkID = totalChunks;
    
    totalChunks++;
}

BTreeNode::BTreeNode(int degree, bool leaf) 
{
    t = degree;
    isLeaf = leaf;
    n = 0;
    
    keys = new string[2 * t - 1];
    values = new FileMetadata[2 * t - 1];
    children = new BTreeNode*[2 * t];
    
    for (int i = 0; i < 2 * t; i++) 
    {
        children[i] = nullptr;
    }
}

BTreeNode::~BTreeNode() 
{
    delete[] keys;
    delete[] values;
    delete[] children;
}

FileMetadata* BTreeNode::search(const string& key) 
{
    int i = 0;
    while (i < n && key > keys[i]) 
    {
        i++;
    }
    
    if (i < n && keys[i] == key) 
    {
        return &values[i];
    }
    
    if (isLeaf) 
    {
        return nullptr;
    }
    
    return children[i]->search(key);
}

void BTreeNode::insertNonFull(const string& key, const FileMetadata& value) 
{
    int i = n - 1;
    
    if (isLeaf) 
    {
        while (i >= 0 && keys[i] > key) 
        {
            keys[i + 1] = keys[i];
            values[i + 1] = values[i];
            i--;
        }
        
        keys[i + 1] = key;
        values[i + 1] = value;
        n++;
    }
    else 
    {
        while (i >= 0 && keys[i] > key) 
        {
            i--;
        }
        i++;
        
        if (children[i]->n == 2 * t - 1) 
        {
            splitChild(i, children[i]);
            
            if (keys[i] < key) 
            {
                i++;
            }
        }
        
        children[i]->insertNonFull(key, value);
    }
}

void BTreeNode::splitChild(int i, BTreeNode* child) 
{
    BTreeNode* newNode = new BTreeNode(child->t, child->isLeaf);
    newNode->n = t - 1;
    
    for (int j = 0; j < t - 1; j++) 
    {
        newNode->keys[j] = child->keys[j + t];
        newNode->values[j] = child->values[j + t];
    }
    
    if (!child->isLeaf) 
    {
        for (int j = 0; j < t; j++) 
        {
            newNode->children[j] = child->children[j + t];
        }
    }
    
    child->n = t - 1;
    
    for (int j = n; j >= i + 1; j--) 
    {
        children[j + 1] = children[j];
    }
    
    children[i + 1] = newNode;
    
    for (int j = n - 1; j >= i; j--) 
    {
        keys[j + 1] = keys[j];
        values[j + 1] = values[j];
    }
    
    keys[i] = child->keys[t - 1];
    values[i] = child->values[t - 1];
    n++;
}

void BTreeNode::traverse() 
{
    int i;
    for (i = 0; i < n; i++) 
    {
        if (!isLeaf) 
        {
            children[i]->traverse();
        }
        cout << "URL: " << keys[i] << " (Chunks: " << values[i].totalChunks << ")" << endl;
    }
    
    if (!isLeaf) 
    {
        children[i]->traverse();
    }
}

BTreeIndex::BTreeIndex(int degree) 
{
    root = nullptr;
    t = degree;
}

BTreeIndex::~BTreeIndex() 
{
    if (root != nullptr) 
    {
        deleteTree(root);
    }
}

void BTreeIndex::deleteTree(BTreeNode* node) 
{
    if (node == nullptr) return;
    
    if (!node->isLeaf) 
    {
        for (int i = 0; i <= node->n; i++) 
        {
            deleteTree(node->children[i]);
        }
    }
    
    delete node;
}

void BTreeIndex::insert(const string& url, const FileMetadata& metadata) 
{
    if (root == nullptr) 
    {
        root = new BTreeNode(t, true);
        root->keys[0] = url;
        root->values[0] = metadata;
        root->n = 1;
    }
    else 
    {
        if (root->n == 2 * t - 1) 
        {
            BTreeNode* newRoot = new BTreeNode(t, false);
            newRoot->children[0] = root;
            newRoot->splitChild(0, root);
            
            int i = 0;
            if (newRoot->keys[0] < url) 
            {
                i++;
            }
            
            newRoot->children[i]->insertNonFull(url, metadata);
            root = newRoot;
        }
        else 
        {
            root->insertNonFull(url, metadata);
        }
    }
}

FileMetadata* BTreeIndex::search(const string& url) 
{
    if (root == nullptr) 
    {
        return nullptr;
    }
    
    return root->search(url);
}

bool BTreeIndex::exists(const string& url) 
{
    return search(url) != nullptr;
}

void BTreeIndex::display() 
{
    if (root != nullptr) 
    {
        root->traverse();
    }
    else 
    {
        cout << "No files in index" << endl;
    }
}

void BTreeNode::printNode(int indent) 
{
    cout << string(indent, ' ') << "Node (Keys: " << n << "):" << endl;
    
    for (int i = 0; i < n; i++) 
    {
        if (!isLeaf && children[i]) 
        {
            children[i]->printNode(indent + 4);
        }
        
        cout << string(indent + 2, ' ') << "┌─ URL: " << keys[i] << endl;
        cout << string(indent + 2, ' ') << "│  File Size: " << values[i].fileSize << " bytes" << endl;
        cout << string(indent + 2, ' ') << "│  Total Chunks: " << values[i].totalChunks << endl;
        
        if (values[i].chunkIndex != nullptr) 
        {
            cout << string(indent + 2, ' ') << "│  Index Type: BLOCK INDEX (O(1) access)" << endl;
            cout << string(indent + 2, ' ') << "│  Blocks: " << values[i].chunkIndex->blockIndex.size() << endl;
            cout << string(indent + 2, ' ') << "│  Block Size: " << BLOCK_SIZE << " chunks per block" << endl;
            
            cout << string(indent + 2, ' ') << "│  Chunk Details (showing first 3):" << endl;
            
            int maxChunks = (values[i].totalChunks > 3) ? 3 : values[i].totalChunks;
            for (int j = 0; j < maxChunks; j++) 
            {
                ChunkMetadata* chunk = values[i].chunkIndex->getChunk(j);
                if (chunk != nullptr) 
                {
                    cout << string(indent + 2, ' ') << "│    Chunk #" << chunk->chunkID 
                         << " - Offset: " << chunk->offset 
                         << ", Size: " << chunk->size 
                         << " bytes" << endl;
                    cout << string(indent + 2, ' ') << "│      Hash: " << chunk->hash.substr(0, 16) << "..." << endl;
                }
            }
            
            if (values[i].totalChunks > 3) 
            {
                cout << string(indent + 2, ' ') << "│    ... (" << (values[i].totalChunks - 3) << " more chunks)" << endl;
            }
        }
        
        cout << string(indent + 2, ' ') << "└─" << endl;
    }
    
    if (!isLeaf && children[n]) 
    {
        children[n]->printNode(indent + 4);
    }
}

void BTreeIndex::printTree() 
{
    cout << "\n═══════════════════════════════════════════════════════════" << endl;
    cout << "            B-TREE FILE INDEX STRUCTURE" << endl;
    cout << "═══════════════════════════════════════════════════════════" << endl;
    
    if (root == nullptr) 
    {
        cout << "  [EMPTY TREE - No files indexed]" << endl;
    }
    else 
    {
        root->printNode(2);
    }
    
    cout << "═══════════════════════════════════════════════════════════\n" << endl;
}

void BTreeIndex::saveNodeRecursive(BTreeNode* node, ofstream& file) 
{
    if (node == nullptr)
    {
        return;
    }
    
    for (int i = 0; i < node->n; i++) 
    {
        if (!node->isLeaf) 
        {
            saveNodeRecursive(node->children[i], file);
        }
        
        FileMetadata& meta = node->values[i];
        
        int urlLen = meta.url.length();
        file.write((char*)&urlLen, sizeof(urlLen));
        file.write(meta.url.c_str(), urlLen);
        
        file.write((char*)&meta.fileSize, sizeof(meta.fileSize));
        file.write((char*)&meta.totalChunks, sizeof(meta.totalChunks));
        
        if (meta.chunkIndex != nullptr) 
        {
            for (int j = 0; j < meta.totalChunks; j++) 
            {
                ChunkMetadata* chunk = meta.chunkIndex->getChunk(j);
                if (chunk != nullptr) 
                {
                    file.write((char*)&chunk->chunkID, sizeof(chunk->chunkID));
                    file.write((char*)&chunk->offset, sizeof(chunk->offset));
                    file.write((char*)&chunk->size, sizeof(chunk->size));
                    
                    int hashLen = chunk->hash.length();
                    file.write((char*)&hashLen, sizeof(hashLen));
                    file.write(chunk->hash.c_str(), hashLen);
                }
            }
        }
    }
    
    if (!node->isLeaf) 
    {
        saveNodeRecursive(node->children[node->n], file);
    }
}
bool BTreeIndex::saveToDisk(const string& filename) 
{
    ofstream file(filename, ios::binary);
    if (!file.is_open()) 
    {
        cerr << "Error: Cannot open " << filename << " for writing" << endl;
        return false;
    }
    
    file.write((char*)&t, sizeof(t));
    
    int fileCount = getFileCount();
    file.write((char*)&fileCount, sizeof(fileCount));
    
    if (fileCount == 0) 
    {
        file.close();
        return true;
    }
    
    saveNodeRecursive(root, file);
    
    file.close();
    return true;
}

bool BTreeIndex::loadFromDisk(const string& filename) 
{
    ifstream file(filename, ios::binary);
    if (!file.is_open()) 
    {
        return false;
    }
    
    int savedDegree;
    file.read((char*)&savedDegree, sizeof(savedDegree));
    
    if (savedDegree != t) 
    {
        cerr << "Warning: Saved degree (" << savedDegree << ") != current degree (" << t << ")" << endl;
        file.close();
        return false;
    }
    
    int fileCount;
    file.read((char*)&fileCount, sizeof(fileCount));
    
    if (fileCount == 0) 
    {
        file.close();
        return true;
    }
    
    cout << "  Loading " << fileCount << " files from disk..." << endl;
    
    for (int f = 0; f < fileCount; f++) 
    {
        FileMetadata meta;
        

        int urlLen;
        file.read((char*)&urlLen, sizeof(urlLen));
        char* urlBuf = new char[urlLen + 1];
        file.read(urlBuf, urlLen);
        urlBuf[urlLen] = '\0';
        meta.url = string(urlBuf);
        delete[] urlBuf;
        
        file.read((char*)&meta.fileSize, sizeof(meta.fileSize));
        file.read((char*)&meta.totalChunks, sizeof(meta.totalChunks));
        
        meta.chunkIndex = new ChunkIndex();
        
        for (int i = 0; i < meta.totalChunks; i++) 
        {
            ChunkMetadata chunk;
            
            file.read((char*)&chunk.chunkID, sizeof(chunk.chunkID));
            file.read((char*)&chunk.offset, sizeof(chunk.offset));
            file.read((char*)&chunk.size, sizeof(chunk.size));
            
            int hashLen;
            file.read((char*)&hashLen, sizeof(hashLen));
            char* hashBuf = new char[hashLen + 1];
            file.read(hashBuf, hashLen);
            hashBuf[hashLen] = '\0';
            chunk.hash = string(hashBuf);
            delete[] hashBuf;
            
            meta.chunkIndex->addChunk(chunk);
        }
        
        insert(meta.url, meta);
        
        if ((f + 1) % 10 == 0 || f == fileCount - 1) 
        {
            cout << "\r  Loaded: " << (f + 1) << "/" << fileCount << " files" << flush;
        }
    }
    
    cout << endl;
    file.close();
    
    return true;
}

void BTreeIndex::countNodeRecursive(BTreeNode* node, int& count) const
{
    if (node == nullptr) return;
    
    for (int i = 0; i < node->n; i++) 
    {
        if (!node->isLeaf) 
        {
            countNodeRecursive(node->children[i], count);
        }
        count++;
    }
    
    if (!node->isLeaf) 
    {
        countNodeRecursive(node->children[node->n], count);
    }
}

int BTreeIndex::getFileCount() const 
{
    if (root == nullptr) return 0;
    
    int count = 0;
    countNodeRecursive(root, count);
    return count;
}
