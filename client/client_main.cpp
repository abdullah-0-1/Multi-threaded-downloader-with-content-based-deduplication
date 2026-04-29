#include "../include/downloader.h"
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>

using namespace std;

const char* SERVER_IP = getenv("SERVER_IP") ? getenv("SERVER_IP") : "127.0.0.1";
const int SERVER_PORT = getenv("SERVER_PORT") ? atoi(getenv("SERVER_PORT")) : 8080;
const int CHUNK_SIZE = 256 * 1024;

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

bool checkFileOnServer(const string& url) 
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        close(sock);
        return false;
    }
    
    char requestType = 'C';
    send(sock, &requestType, 1, 0);
    
    int urlLength = url.length();
    send(sock, &urlLength, sizeof(urlLength), 0);
    send(sock, url.c_str(), urlLength, 0);
    
    char response = 0;
    recv(sock, &response, 1, 0);
    
    close(sock);
    return (response == 'Y');
}

bool downloadFromServer(const string& url, const string& outputPath) 
{
    cout << "\n========================================" << endl;
    cout << "  Downloading from Server" << endl;
    cout << "========================================" << endl;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) 
    {
        cerr << "Error: Failed to create socket" << endl;
        return false;
    }
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        cerr << "Error: Failed to connect to server" << endl;
        close(sock);
        return false;
    }
    
    cout << "✓ Connected to server: " << SERVER_IP << ":" << SERVER_PORT << endl;
    
    char requestType = 'D';
    send(sock, &requestType, 1, 0);
    
    int urlLength = url.length();
    send(sock, &urlLength, sizeof(urlLength), 0);
    send(sock, url.c_str(), urlLength, 0);
    
    cout << "✓ Requesting file: " << url << endl;
    
    long fileSize = 0;
    int totalChunks = 0;
    recv(sock, &fileSize, sizeof(fileSize), 0);
    recv(sock, &totalChunks, sizeof(totalChunks), 0);
    
    cout << "  File size: " << fileSize << " bytes" << endl;
    cout << "  Total chunks: " << totalChunks << endl;
    
    ofstream outFile(outputPath, ios::binary);
    if (!outFile) 
    {
        cerr << "Error: Failed to create output file" << endl;
        close(sock);
        return false;
    }
    
    long bytesReceived = 0;
    for (int i = 0; i < totalChunks; i++) 
    {
        int chunkSize = 0;
        recv(sock, &chunkSize, sizeof(chunkSize), 0);
        
        char* buffer = new char[chunkSize];
        int totalRead = 0;
        
        while (totalRead < chunkSize) 
        {
            int received = recv(sock, buffer + totalRead, chunkSize - totalRead, 0);
            if (received <= 0) break;
            totalRead += received;
        }
        
        if (totalRead != chunkSize) 
        {
            delete[] buffer;
            cerr << "\nError: Incomplete chunk received" << endl;
            outFile.close();
            close(sock);
            return false;
        }
        
        outFile.write(buffer, chunkSize);
        bytesReceived += chunkSize;
        
        delete[] buffer;
        
        int progress = ((i + 1) * 100) / totalChunks;
        cout << "\r  Progress: [";
        int barWidth = 30;
        int pos = (barWidth * progress) / 100;
        for (int j = 0; j < barWidth; j++) 
        {
            if (j < pos) cout << "=";
            else if (j == pos) cout << ">";
            else cout << " ";
        }
        cout << "] " << progress << "% (" << (i + 1) << "/" << totalChunks << " chunks)" << flush;
    }
    
    cout << endl;
    outFile.close();
    close(sock);
    
    cout << "✓ Download complete: " << outputPath << endl;
    cout << "  Total bytes: " << bytesReceived << endl;
    
    return true;
}

bool uploadToServer(const string& url, const string& filePath) 
{
    cout << "\n========================================" << endl;
    cout << "  Uploading to Server" << endl;
    cout << "========================================" << endl;
    
    ifstream inFile(filePath, ios::binary | ios::ate);
    if (!inFile) 
    {
        cerr << "Error: Failed to open file: " << filePath << endl;
        return false;
    }
    
    long fileSize = inFile.tellg();
    inFile.seekg(0, ios::beg);
    
    int totalChunks = (fileSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
    
    cout << "  File: " << filePath << endl;
    cout << "  Size: " << fileSize << " bytes" << endl;
    cout << "  Chunks: " << totalChunks << endl;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) 
    {
        cerr << "Error: Failed to create socket" << endl;
        inFile.close();
        return false;
    }
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) 
    {
        cerr << "Error: Failed to connect to server" << endl;
        close(sock);
        inFile.close();
        return false;
    }
    
    cout << "✓ Connected to server: " << SERVER_IP << ":" << SERVER_PORT << endl;
    
    char requestType = 'U';
    send(sock, &requestType, 1, 0);
    
    int urlLength = url.length();
    send(sock, &urlLength, sizeof(urlLength), 0);
    send(sock, url.c_str(), urlLength, 0);
    send(sock, &fileSize, sizeof(fileSize), 0);
    send(sock, &totalChunks, sizeof(totalChunks), 0);
    
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    int serverResponse = 0;
    int recvBytes = recv(sock, &serverResponse, sizeof(serverResponse), 0);
    
    if (recvBytes > 0 && serverResponse == -1) 
    {
        cout << "\n⚠ File already exists on server (100% deduplicated)" << endl;
        cout << "✓ SUCCESS: File uploaded to server!" << endl;
        cout << "  Server identifier: " << url << endl;
        inFile.close();
        close(sock);
        return true;
    }
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    cout << "✓ Uploading: " << url << endl;
    
    for (int i = 0; i < totalChunks; i++) 
    {
        int chunkSize = min((long)CHUNK_SIZE, fileSize - (i * CHUNK_SIZE));
        char* buffer = new char[chunkSize];
        
        inFile.read(buffer, chunkSize);
        
        int chunkID = i;
        send(sock, &chunkID, sizeof(chunkID), 0);
        send(sock, &chunkSize, sizeof(chunkSize), 0);
        
        int totalSent = 0;
        while (totalSent < chunkSize) 
        {
            int sent = send(sock, buffer + totalSent, chunkSize - totalSent, 0);
            if (sent <= 0) break;
            totalSent += sent;
        }
        
        long offset = 0;
        recv(sock, &offset, sizeof(offset), 0);
        
        delete[] buffer;
        
        int progress = ((i + 1) * 100) / totalChunks;
        cout << "\r  Progress: [";
        int barWidth = 30;
        int pos = (barWidth * progress) / 100;
        for (int j = 0; j < barWidth; j++) 
        {
            if (j < pos) cout << "=";
            else if (j == pos) cout << ">";
            else cout << " ";
        }
        cout << "] " << progress << "% (" << (i + 1) << "/" << totalChunks << " chunks)" << flush;
    }
    
    cout << endl;
    
    inFile.close();
    close(sock);
    
    cout << "✓ Upload complete!" << endl;
    
    return true;
}

int main(int argc, char* argv[]) 
{
    cout << "========================================" << endl;
    cout << "  Integrated Download Client v2.0" << endl;
    cout << "========================================" << endl;
    cout << endl;
    
    cout << "Select operation:" << endl;
    cout << "  [1] Download file from URL" << endl;
    cout << "  [2] Upload local file to server" << endl;
    cout << "Enter choice (1 or 2): ";
    
    int choice;
    cin >> choice;
    cin.ignore();
    
    if (choice == 1) 
    {
        string url;
        cout << "\nEnter URL to download: ";
        getline(cin, url);
        
        if (url.empty()) 
        {
            cerr << "Error: URL cannot be empty" << endl;
            return 1;
        }
        
        cout << "\nURL: " << url << endl;
        
        Downloader downloader;
        string filename = downloader.getFileName(url);
        string tempDownloadPath = "downloads/" + filename;
        
        cout << "\n[1] Checking server..." << endl;
        bool existsOnServer = checkFileOnServer(url);
        
        if (existsOnServer) 
        {
            cout << "✓ File found on server!" << endl;
            
            if (downloadFromServer(url, tempDownloadPath)) 
            {
                cout << "\n✓ SUCCESS: Downloaded from server (deduplicated storage)" << endl;
                cout << "  File saved to: " << tempDownloadPath << endl;
                return 0;
            }
            else 
            {
                cerr << "\n✗ ERROR: Failed to download from server" << endl;
                return 1;
            }
        }
        else 
        {
            cout << "✗ File not found on server" << endl;
            
            cout << "\n[2] Downloading from original URL..." << endl;
            
            if (!downloader.downloadFile(url, tempDownloadPath, 8)) 
            {
                cerr << "\n✗ ERROR: Failed to download file" << endl;
                return 1;
            }
            
            cout << "\n✓ Download complete: " << tempDownloadPath << endl;
            
            cout << "\n[3] Uploading to server for deduplication..." << endl;
            
            if (!uploadToServer(url, tempDownloadPath)) 
            {
                cerr << "\n✗ ERROR: Failed to upload to server" << endl;
                return 1;
            }
            
            cout << "\n✓ SUCCESS: File downloaded and uploaded to server" << endl;
            cout << "  Local file: " << tempDownloadPath << endl;
            cout << "  Next client requesting this URL will download from server!" << endl;
        }
    }
    else if (choice == 2) 
    {

        string filePath;
        cout << "\nEnter local file path to upload: ";
        getline(cin, filePath);
        
        if (filePath.empty()) 
        {
            cerr << "Error: File path cannot be empty" << endl;
            return 1;
        }
        
        ifstream testFile(filePath);
        if (!testFile.good()) 
        {
            cerr << "Error: File not found: " << filePath << endl;
            return 1;
        }
        testFile.close();
        
        size_t lastSlash = filePath.find_last_of("/\\");
        string filename = (lastSlash != string::npos) ? filePath.substr(lastSlash + 1) : filePath;
        
        string url = "local://" + filename;
        
        cout << "\nFile: " << filePath << endl;
        cout << "Server identifier: " << url << endl;
        
        cout << "\n[1] Uploading to server..." << endl;
        
        if (!uploadToServer(url, filePath)) 
        {
            cerr << "\n✗ ERROR: Failed to upload to server" << endl;
            return 1;
        }
        
        cout << "\n✓ SUCCESS: File uploaded to server!" << endl;
        cout << "  Server identifier: " << url << endl;
        cout << "  You can now download it using option 1 with URL: " << url << endl;
    }
    else 
    {
        cerr << "Error: Invalid choice. Please enter 1 or 2." << endl;
        return 1;
    }
    
    return 0;
}
