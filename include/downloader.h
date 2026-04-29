#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <curl/curl.h>
#include "minheap.h"
#include "chunktask.h"

using namespace std;

class Downloader 
{
private:
    string url;
    string outputPath;
    long fileSize;
    int numThreads;
    
    MinHeap taskHeap;
    vector<thread> workers;
    
    mutex heapMutex;
    mutex consoleMutex;
    
    int chunksDownloaded;
    int totalChunks;
    vector<int> threadStatus;
    
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    long fetchFileSize(const string& url);
    bool supportsRangeRequests(const string& url);
    void createChunks(long fileSize, int numChunks);
    void createEmptyFile(const string& path, long size);
    void workerThread(int threadId);
    bool downloadChunk(const ChunkTask& task, CURL* curl, const string& url, int fd);
    bool downloadSingleThreaded();
    void displayProgressBar();
    
public:
    Downloader();
    
    ~Downloader();
    
    bool downloadFile(const string& url, const string& outputPath, int numThreads = 8);
    
    string getFileName(const string& url);
    string formatBytes(long bytes);
};