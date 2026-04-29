#include "storage_manager.h"
#include "deduplication_engine.h"
#include "btree_index.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <thread>
#include <mutex>
#include <csignal>

using namespace std;

StorageManager* storage;
DeduplicationEngine* dedup;
BTreeIndex* btree;
mutex serverMutex;

const string BTREE_INDEX_FILE = "server-storage/btree.index";
const string DEDUP_INDEX_FILE = "server-storage/dedup.index";

void saveIndexes() 
{
    cout << "\n💾 Saving indexes to disk..." << endl;
    
    const string btreeTmp = BTREE_INDEX_FILE + ".tmp";
    const string dedupTmp = DEDUP_INDEX_FILE + ".tmp";
    
    bool btreeSaved = btree->saveToDisk(btreeTmp);
    bool dedupSaved = dedup->saveToDisk(dedupTmp);
    
    if (btreeSaved && dedupSaved) 
    {
        if (rename(btreeTmp.c_str(), BTREE_INDEX_FILE.c_str()) == 0 &&
            rename(dedupTmp.c_str(), DEDUP_INDEX_FILE.c_str()) == 0) 
        {
            cout << "  ✓ B-Tree index saved (" << btree->getFileCount() << " files)" << endl;
            cout << "  ✓ Dedup index saved (" << dedup->size() << " chunks)" << endl;
            cout << "  ✓ Atomic write completed successfully" << endl;
        }
        else 
        {
            cerr << "  ✗ Failed to rename temp files (indexes may be inconsistent)" << endl;
            remove(btreeTmp.c_str());
            remove(dedupTmp.c_str());
        }
    }
    else 
    {
        cerr << "  ✗ Failed to save indexes!" << endl;
        if (!btreeSaved) cerr << "    - B-Tree save failed" << endl;
        if (!dedupSaved) cerr << "    - Dedup save failed" << endl;
        
        remove(btreeTmp.c_str());
        remove(dedupTmp.c_str());
        
        cerr << "  ✓ Old indexes remain intact (no changes made)" << endl;
    }
}

void loadIndexes() 
{
    cout << "\n📂 Loading indexes from disk..." << endl;
    
    bool btreeLoaded = btree->loadFromDisk(BTREE_INDEX_FILE);
    bool dedupLoaded = dedup->loadFromDisk(DEDUP_INDEX_FILE);
    
    if (btreeLoaded && !dedupLoaded) 
    {
        cerr << "  ⚠ WARNING: B-Tree loaded but Dedup failed!" << endl;
        cerr << "  This indicates a partial save failure." << endl;
        cerr << "  Discarding B-Tree to prevent inconsistency..." << endl;
        delete btree;
        btree = new BTreeIndex(3);
        btreeLoaded = false;
    }
    else if (!btreeLoaded && dedupLoaded) 
    {
        cerr << "  ⚠ WARNING: Dedup loaded but B-Tree failed!" << endl;
        cerr << "  This indicates a partial save failure." << endl;
        cerr << "  Discarding Dedup to prevent inconsistency..." << endl;
        delete dedup;
        dedup = new DeduplicationEngine();
        dedupLoaded = false;
    }
    
    if (btreeLoaded && dedupLoaded) 
    {
        int btreeFiles = btree->getFileCount();
        int dedupChunks = dedup->size();
        
        cout << "  ✓ B-Tree index loaded (" << btreeFiles << " files)" << endl;
        cout << "  ✓ Dedup index loaded (" << dedupChunks << " chunks)" << endl;
        
        if (btreeFiles > 0 && dedupChunks == 0) 
        {
            cerr << "  ⚠ INCONSISTENCY DETECTED: Files exist but no chunks!" << endl;
            cerr << "  Starting fresh to avoid data corruption..." << endl;
            delete btree;
            delete dedup;
            btree = new BTreeIndex(3);
            dedup = new DeduplicationEngine();
        }
        else 
        {
            cout << "  ✓ Consistency check passed - indexes are valid" << endl;
        }
    }
    else if (!btreeLoaded && !dedupLoaded) 
    {
        cout << "  ℹ No existing indexes found (first run or clean state)" << endl;
    }
}

void signalHandler(int signum) 
{
    cout << "\n\n🛑 Shutting down server..." << endl;
    saveIndexes();
    cout << "  ✓ Graceful shutdown complete!" << endl;
    exit(0);
}

string hashToHex(unsigned char* hash, int length) 
{
    stringstream ss;
    for (int i = 0; i < length; i++) 
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

string calculateSHA256(const char* data, int size) 
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)data, size, hash);
    return hashToHex(hash, SHA256_DIGEST_LENGTH);
}

void handleCheckRequest(int clientSocket) 
{
    int urlLength = 0;
    recv(clientSocket, &urlLength, sizeof(urlLength), 0);
    
    if (urlLength <= 0 || urlLength > 1024) 
    {
        close(clientSocket);
        return;
    }
    
    char* urlBuffer = new char[urlLength + 1];
    recv(clientSocket, urlBuffer, urlLength, 0);
    urlBuffer[urlLength] = '\0';
    string url(urlBuffer);
    delete[] urlBuffer;
    
    cout << "\n[CHECK] Client checking: " << url << endl;
    
    bool exists = false;
    {
        lock_guard<mutex> lock(serverMutex);
        exists = btree->exists(url);
    }
    
    char response = exists ? 'Y' : 'N';
    send(clientSocket, &response, 1, 0);
    
    cout << "  Response: " << (exists ? "EXISTS" : "NOT FOUND") << endl;
    
    close(clientSocket);
}

void handleDownloadRequest(int clientSocket) 
{
    int urlLength = 0;
    recv(clientSocket, &urlLength, sizeof(urlLength), 0);
    
    if (urlLength <= 0 || urlLength > 1024) 
    {
        close(clientSocket);
        return;
    }
    
    char* urlBuffer = new char[urlLength + 1];
    recv(clientSocket, urlBuffer, urlLength, 0);
    urlBuffer[urlLength] = '\0';
    string url(urlBuffer);
    delete[] urlBuffer;
    
    cout << "\n[DOWNLOAD] Client requesting: " << url << endl;
    
    FileMetadata* fileMeta = nullptr;
    {
        lock_guard<mutex> lock(serverMutex);
        fileMeta = btree->search(url);
    }
    
    if (fileMeta == nullptr) 
    {
        cout << "  Error: File not found" << endl;
        close(clientSocket);
        return;
    }
    
    send(clientSocket, &fileMeta->fileSize, sizeof(fileMeta->fileSize), 0);
    send(clientSocket, &fileMeta->totalChunks, sizeof(fileMeta->totalChunks), 0);
    
    cout << "  File size: " << fileMeta->fileSize << " bytes" << endl;
    cout << "  Total chunks: " << fileMeta->totalChunks << endl;
    
    for (int i = 0; i < fileMeta->totalChunks; i++) 
    {
        ChunkMetadata* chunk = fileMeta->chunkIndex->getChunk(i);
        
        if (chunk == nullptr) 
        {
            cout << "  Error: Chunk " << i << " not found in index!" << endl;
            break;
        }
        
        send(clientSocket, &chunk->size, sizeof(chunk->size), 0);
        
        char* buffer = new char[chunk->size];
        storage->readChunk(chunk->offset, chunk->size, buffer);
        
        int totalSent = 0;
        while (totalSent < chunk->size) 
        {
            int sent = send(clientSocket, buffer + totalSent, chunk->size - totalSent, 0);
            if (sent <= 0) break;
            totalSent += sent;
        }
        
        delete[] buffer;
        
        if ((i + 1) % 10 == 0 || i == fileMeta->totalChunks - 1) 
        {
            cout << "\r  Sending: " << (i + 1) << "/" << fileMeta->totalChunks << " chunks" << flush;
        }
    }
    
    cout << endl;
    cout << "  ✓ Download complete" << endl;
    
    close(clientSocket);
}
void handleUploadRequest(int clientSocket) 
{
    
    int urlLength = 0;
    recv(clientSocket, &urlLength, sizeof(urlLength), 0);
    
    if (urlLength <= 0 || urlLength > 1024) 
    {
        close(clientSocket);
        return;
    }
    
    char* urlBuffer = new char[urlLength + 1];
    recv(clientSocket, urlBuffer, urlLength, 0);
    urlBuffer[urlLength] = '\0';
    string url(urlBuffer);
    delete[] urlBuffer;
    
    cout << "\n[UPLOAD] Client uploading: " << url << endl;
    
    long fileSize = 0;
    int totalChunks = 0;
    recv(clientSocket, &fileSize, sizeof(fileSize), 0);
    recv(clientSocket, &totalChunks, sizeof(totalChunks), 0);
    
    cout << "  File size: " << fileSize << " bytes" << endl;
    cout << "  Total chunks: " << totalChunks << endl;
    
    {
        lock_guard<mutex> lock(serverMutex);
        if (btree->exists(url)) 
        {
            cout << "  ⚠ File already exists on server!" << endl;
            cout << "  Rejecting duplicate upload..." << endl;
            cout << "  (Use download to retrieve existing file)" << endl;
            
            int rejectCode = -1;
            send(clientSocket, &rejectCode, sizeof(rejectCode), 0);
            
            close(clientSocket);
            return;
        }
    }
    
    FileMetadata fileMeta;
    fileMeta.url = url;
    fileMeta.fileSize = fileSize;
    fileMeta.totalChunks = totalChunks;
    
    fileMeta.chunkIndex = new ChunkIndex();
    
    int savedBytes = 0;
    int duplicates = 0;
    
    for (int i = 0; i < totalChunks; i++) 
    {
        int chunkID = 0;
        int chunkSize = 0;
        recv(clientSocket, &chunkID, sizeof(chunkID), 0);
        recv(clientSocket, &chunkSize, sizeof(chunkSize), 0);
        
        char* buffer = new char[chunkSize];
        int totalReceived = 0;
        
        while (totalReceived < chunkSize) 
        {
            int received = recv(clientSocket, buffer + totalReceived, chunkSize - totalReceived, 0);
            if (received <= 0) break;
            totalReceived += received;
        }
        
        if (totalReceived != chunkSize) 
        {
            delete[] buffer;
            continue;
        }
        
        string hash = calculateSHA256(buffer, chunkSize);
        
        long offset;
        bool isDuplicate = false;
        
        {
            lock_guard<mutex> lock(serverMutex);
            
            if (dedup->exists(hash)) 
            {
                offset = dedup->getOffset(hash);
                isDuplicate = true;
                duplicates++;
                savedBytes += chunkSize;
            }
            else 
            {
                offset = storage->saveChunk(buffer, chunkSize);
                dedup->insert(hash, offset);
            }
        }
        
        ChunkMetadata chunk(chunkID, offset, chunkSize, hash);
        fileMeta.chunkIndex->addChunk(chunk);
        
        send(clientSocket, &offset, sizeof(offset), 0);
        
        delete[] buffer;
        
        if ((i + 1) % 10 == 0 || i == totalChunks - 1) 
        {
            cout << "\r  Progress: " << (i + 1) << "/" << totalChunks << " chunks" << flush;
        }
    }
    
    cout << endl;
    
    {
        lock_guard<mutex> lock(serverMutex);
        btree->insert(url, fileMeta);
    }
    
    cout << "  ✓ File indexed in B-Tree" << endl;
    cout << "  Duplicates found: " << duplicates << "/" << totalChunks << endl;
    cout << "  Space saved: " << savedBytes << " bytes" << endl;
    cout << "  Unique chunks: " << dedup->size() << endl;
    cout << "  Storage size: " << storage->getCurrentSize() << " bytes" << endl;
    
    cout << "\n B-TREE STRUCTURE AFTER INSERTION:" << endl;
    btree->printTree();
    
    saveIndexes();
    
    close(clientSocket);
}
void handleClient(int clientSocket) 
{
    char requestType = 0;
    int received = recv(clientSocket, &requestType, 1, 0);
    
    if (received <= 0) 
    {
        close(clientSocket);
        return;
    }
    
    switch (requestType) 
    {
        case 'C':
            handleCheckRequest(clientSocket);
            break;
        case 'D':
            handleDownloadRequest(clientSocket);
            break;
        case 'U':
            handleUploadRequest(clientSocket);
            break;
        default:
            cout << "\n[ERROR] Unknown request type: " << requestType << endl;
            close(clientSocket);
            break;
    }
}

int main() 
{
    cout << "========================================" << endl;
    cout << "  Deduplication Storage Server v2.0    " << endl;
    cout << "========================================" << endl;
    cout << endl;
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    storage = new StorageManager("server-storage/storage.bin");
    dedup = new DeduplicationEngine();
    btree = new BTreeIndex(3);
    
    loadIndexes();
    
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) 
    {
        cerr << "Error: Failed to create socket" << endl;
        return 1;
    }
    
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        cerr << "Error: Failed to bind to port 8080" << endl;
        close(serverSocket);
        return 1;
    }
    
    if (listen(serverSocket, 10) < 0) 
    {
        cerr << "Error: Failed to listen" << endl;
        close(serverSocket);
        return 1;
    }
    
    cout << "\n✓ Server listening on port 8080..." << endl;
    cout << "Ready to receive files with deduplication!" << endl;
    cout << "Press Ctrl+C to shutdown gracefully" << endl;
    cout << endl;
    
    while (true) 
    {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) 
        {
            continue;
        }
        
        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }
    
    delete storage;
    delete dedup;
    delete btree;
    close(serverSocket);
    return 0;
}
