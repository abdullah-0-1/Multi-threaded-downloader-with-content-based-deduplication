#include "../include/downloader.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

using namespace std;

Downloader::Downloader() 
{
    fileSize = 0;
    numThreads = 8;
    chunksDownloaded = 0;
    totalChunks = 0;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

Downloader::~Downloader() 
{
    curl_global_cleanup();
}

size_t Downloader::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) 
{
    size_t totalSize = size * nmemb;
    string* buffer = static_cast<string*>(userp);
    buffer->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

struct WriterContext {
    int fd;
    long currentOffset;  
    char buffer[64*1024]; 
    size_t bufferPos;    
};

size_t streamWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) 
{
    size_t dataSize = size * nmemb;
    WriterContext* ctx = static_cast<WriterContext*>(userp);
    if (dataSize == 0) return 0;

    char* data = static_cast<char*>(contents);
    size_t remaining = dataSize;
    
    while (remaining > 0) 
    {
        size_t bufferSpace = sizeof(ctx->buffer) - ctx->bufferPos;
        size_t copySize = min(remaining, bufferSpace);
        
        memcpy(ctx->buffer + ctx->bufferPos, data + (dataSize - remaining), copySize);
        ctx->bufferPos += copySize;
        remaining -= copySize;
        
        if (ctx->bufferPos == sizeof(ctx->buffer)) 
        {
            ssize_t written = pwrite(ctx->fd, ctx->buffer, ctx->bufferPos, ctx->currentOffset);
            if (written < 0) return 0;
            
            ctx->currentOffset += written;
            ctx->bufferPos = 0;
        }
    }
    
    return dataSize;
}

void flushBuffer(WriterContext* ctx) 
{
    if (ctx->bufferPos > 0) 
    {
        ssize_t written = pwrite(ctx->fd, ctx->buffer, ctx->bufferPos, ctx->currentOffset);
        if (written > 0) 
        {
            ctx->currentOffset += written;
        }
        ctx->bufferPos = 0;
    }
}

long Downloader::fetchFileSize(const string& url) 
{
    CURL* curl = curl_easy_init();
    if (!curl) 
    {
        cerr << "Error: Failed to initialize CURL" << endl;
        return -1;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); 
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    
    CURLcode res = curl_easy_perform(curl);
    
    long size = -1;
    if (res == CURLE_OK) 
    {
        curl_off_t cl;
        res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
        if (res == CURLE_OK && cl > 0) 
        {
            size = static_cast<long>(cl);
        }
    }
    
    curl_easy_cleanup(curl);
    return size;
}

bool Downloader::supportsRangeRequests(const string& url) 
{
    CURL* curl = curl_easy_init();
    if (!curl) 
    {
        return false;
    }
    
    string headerBuffer;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerBuffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) 
    {
        return false;
    }
    
    return headerBuffer.find("Accept-Ranges: bytes") != string::npos;
}

void Downloader::createChunks(long fileSize, int numChunks) 
{
    const long MIN_CHUNK_SIZE = 1 * 1024 * 1024;
    
    long chunkSize = fileSize / numChunks;
    
    if (chunkSize < MIN_CHUNK_SIZE && fileSize > MIN_CHUNK_SIZE) 
    {
        numChunks = fileSize / MIN_CHUNK_SIZE;
        if (numChunks < 1) numChunks = 1;
        chunkSize = fileSize / numChunks;
    }
    
    long remaining = fileSize % numChunks;
    
    long currentStart = 0;
    for (int i = 0; i < numChunks; i++) 
    {
        long currentChunkSize = chunkSize;
        if (i == numChunks - 1) 
        {
            currentChunkSize += remaining;
        }
        
        long currentEnd = currentStart + currentChunkSize - 1;
        
        ChunkTask task(i + 1, currentStart, currentEnd);
        taskHeap.push(task);
        
        currentStart = currentEnd + 1;
    }
    
    totalChunks = numChunks;
    
    cout << "Split into " << numChunks << " chunks of ~" 
         << formatBytes(chunkSize) << endl;
}

void Downloader::createEmptyFile(const string& path, long size) 
{
    ofstream file(path, ios::binary);
    if (!file) 
    {
        throw runtime_error("Failed to create output file");
    }
    
    file.seekp(size - 1);
    file.put('\0');
    file.close();
}

void Downloader::workerThread(int threadId) 
{
    CURL* curl = curl_easy_init();
    if (!curl) 
    {
        cerr << "Error: Failed to initialize CURL for thread " << threadId << endl;
        return;
    }
    
    int fd = open(outputPath.c_str(), O_WRONLY);
    if (fd < 0) 
    {
        cerr << "Error: Failed to open file for thread " << threadId << endl;
        curl_easy_cleanup(curl);
        return;
    }
    
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 IDM-Clone/1.0");
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
    
    curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 128*1024L);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
    curl_easy_setopt(curl, CURLOPT_PIPEWAIT, 1L);
    
    while (true) 
    {
        ChunkTask task;
        
        {
            lock_guard<mutex> lock(heapMutex);
            if (taskHeap.isEmpty()) 
            {
                threadStatus[threadId] = 0;
                break;
            }
            
            task = taskHeap.pop();
            
            long taskSize = task.getSize();
            
            if (taskHeap.size() < numThreads && taskSize > 2 * 1024 * 1024) 
            {
                long midPoint = task.startByte + (taskSize / 2);
                
                ChunkTask newTask(totalChunks + 1, midPoint, task.endByte);
                totalChunks++; 
                taskHeap.push(newTask);
                
                task.endByte = midPoint - 1;
            }
            
            threadStatus[threadId] = task.id;
        }
        
        static int updateCounter = 0;
        if ((++updateCounter % 4) == 0)
        {
            lock_guard<mutex> lock(consoleMutex);
            displayProgressBar();
        }
        
        int retries = 5;
        bool success = false;
        
        for (int attempt = 0; attempt < retries && !success; attempt++) 
        {
            success = downloadChunk(task, curl, url, fd);
            
            if (!success && attempt < retries - 1) 
            {
                this_thread::sleep_for(chrono::milliseconds(500));
            }
        }
        
        if (success) 
        {
            lock_guard<mutex> lock(consoleMutex);
            chunksDownloaded++;
            threadStatus[threadId] = 0;
            
            displayProgressBar();
        }
        else 
        {
            lock_guard<mutex> lock(consoleMutex);
            threadStatus[threadId] = 0;
            cerr << "\nError: Failed to download chunk " << task.id << " after " << retries << " attempts" << endl;
        }
    }
    
    close(fd);
    curl_easy_cleanup(curl);
}

bool Downloader::downloadChunk(const ChunkTask& task, CURL* curl, const string& downloadUrl, int fd) 
{
    if (!curl || fd < 0) 
    {
        return false;
    }
    
    WriterContext ctx;
    ctx.fd = fd;  
    ctx.currentOffset = task.startByte; 
    ctx.bufferPos = 0;
    
    string range = to_string(task.startByte) + "-" + to_string(task.endByte);
    
    curl_easy_setopt(curl, CURLOPT_URL, downloadUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, streamWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    flushBuffer(&ctx);
    
    if (res != CURLE_OK || (response_code != 200 && response_code != 206)) 
    {
        return false;
    }
    
    return true;
}

void Downloader::displayProgressBar() 
{
    cout << "\r\033[K";
    
    int percentage = (chunksDownloaded * 100) / totalChunks;
    
    int barWidth = 20;
    int filledWidth = (barWidth * chunksDownloaded) / totalChunks;
    
    cout << "[";
    cout << string(filledWidth, '=');
    if (filledWidth < barWidth) cout << ">";
    cout << string(barWidth - filledWidth - (filledWidth < barWidth ? 1 : 0), ' ');
    cout << "] " << percentage << "% (" << chunksDownloaded << "/" << totalChunks << ") ";
    
    cout << "| T[";
    for (size_t i = 0; i < threadStatus.size(); i++) 
    {
        if (i > 0) 
        {
            cout << ",";
        }
        if (threadStatus[i] > 0) 
        {
            cout << threadStatus[i];
        } 
        else 
        {
            cout << "-";
        }
    }
    cout << "]";
    
    cout.flush();
}

bool Downloader::downloadSingleThreaded()
{
    CURL* curl = curl_easy_init();
    if (!curl) 
    {
        cerr << "Error: Failed to initialize CURL" << endl;
        return false;
    }
    
    FILE* fp = fopen(outputPath.c_str(), "wb");
    if (!fp) 
    {
        cerr << "Error: Cannot open file for writing" << endl;
        curl_easy_cleanup(curl);
        return false;
    }
    
    cout << "Downloading entire file..." << endl;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, 
        +[](void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) -> int {
            if (dltotal > 0) 
            {
                int percentage = static_cast<int>((dlnow * 100) / dltotal);
                cout << "\rProgress: " << percentage << "% (" 
                     << (dlnow / 1024 / 1024) << " MB / " 
                     << (dltotal / 1024 / 1024) << " MB)    ";
                cout.flush();
            }
            return 0;
        });
    
    CURLcode res = curl_easy_perform(curl);
    
    fclose(fp);
    curl_easy_cleanup(curl);
    
    cout << endl;
    
    if (res != CURLE_OK) 
    {
        cerr << "Error: Download failed - " << curl_easy_strerror(res) << endl;
        return false;
    }
    
    return true;
}

bool Downloader::downloadFile(const string& url, const string& outputPath, int numThreads) 
{
    this->url = url;
    this->outputPath = outputPath;
    this->numThreads = numThreads;
    
    cout << "=== Multi-Threaded Downloader ===" << endl;
    cout << "URL: " << url << endl;
    cout << "Output: " << outputPath << endl;
    cout << endl;
    
    cout << "Fetching file information..." << endl;
    fileSize = fetchFileSize(url);
    
    if (fileSize <= 0) 
    {
        cout << "Warning: Could not determine file size" << endl;
        cout << "Server may not support HEAD/range requests (e.g., Catbox)" << endl;
        cout << "Attempting single-threaded download without size info..." << endl;
        cout << endl;
        
        auto startTime = chrono::high_resolution_clock::now();
        
        bool success = downloadSingleThreaded();
        
        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();
        
        if (success) 
        {
            cout << endl;
            cout << "Download completed in " << duration << " seconds!" << endl;
        }
        
        return success;
    }
    
    cout << "File size: " << formatBytes(fileSize) << endl;
    
    if (!supportsRangeRequests(url)) 
    {
        cout << "Warning: Server does not support range requests" << endl;
        cout << "Using single-threaded download mode..." << endl;
        cout << endl;
        
        auto startTime = chrono::high_resolution_clock::now();
        
        bool success = downloadSingleThreaded();
        
        auto endTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();
        
        if (success) 
        {
            cout << endl;
            cout << "Download completed in " << duration << " seconds!" << endl;
            if (duration > 0) 
            {
                cout << "Average speed: " << formatBytes(fileSize / duration) << "/s" << endl;
            }
        }
        
        return success;
    }
    
    cout << "Using " << numThreads << " threads" << endl;
    cout << endl;
    
    try 
    {
        createEmptyFile(outputPath, fileSize);
    }
    catch (const exception& e) 
    {
        cerr << "Error: " << e.what() << endl;
        return false;
    }
    
    createChunks(fileSize, numThreads * 16);
    
    threadStatus.resize(numThreads, 0);
    
    cout << "Starting download..." << endl;
    cout << endl;
    auto startTime = chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numThreads; i++) 
    {
        workers.push_back(thread(&Downloader::workerThread, this, i));
    }
    
    for (auto& worker : workers) 
    {
        worker.join();
    }
    
    auto endTime = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(endTime - startTime).count();
    
    cout << endl;
    cout << endl;
    cout << "Download completed in " << duration << " seconds!" << endl;
    if (duration > 0) 
    {
        cout << "Average speed: " << formatBytes(fileSize / duration) << "/s" << endl;
    }
    
    return true;
}

string Downloader::getFileName(const string& url) 
{
    if (url.find("local://") == 0) 
    {
        return url.substr(8);
    }
    
    size_t pos = url.find_last_of('/');
    if (pos != string::npos) 
    {
        return url.substr(pos + 1);
    }
    
    if (!url.empty() && url.find("://") == string::npos) 
    {
        return url;
    }
    
    return "download.bin";
}

string Downloader::formatBytes(long bytes) 
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) 
    {
        size /= 1024.0;
        unit++;
    }
    
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
    return string(buffer);
}
