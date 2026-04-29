#include <iostream>
#include <string>
#include "../include/downloader.h"

using namespace std;

int main(int argc, char* argv[]) 
{
    cout << "========================================" << endl;
    cout << "     Multi-Threaded IDM Clone v1.0     " << endl;
    cout << "========================================" << endl;
    cout << endl;
    
    if (argc < 2) 
    {
        cout << "Usage: " << argv[0] << " <URL> [num_threads|output_path] [num_threads]" << endl;
        cout << endl;
        cout << "Examples:" << endl;
        cout << "  " << argv[0] << " https://example.com/file.zip" << endl;
        cout << "  " << argv[0] << " https://example.com/file.zip 4" << endl;
        cout << "  " << argv[0] << " https://example.com/file.zip downloads/myfile.zip" << endl;
        cout << "  " << argv[0] << " https://example.com/file.zip downloads/myfile.zip 16" << endl;
        cout << endl;
        return 1;
    }
    
    Downloader downloader;
    
    string url = argv[1];
    string outputPath;
    int numThreads = 8;
    
    if (argc >= 3) 
    {
        string arg2 = argv[2];
        
        bool isNumber = true;
        for (char c : arg2) 
        {
            if (!isdigit(c)) 
            {
                isNumber = false;
                break;
            }
        }
        
        if (isNumber) 
        {
            numThreads = stoi(arg2);
            if (numThreads < 1 || numThreads > 32) 
            {
                cout << "Warning: Thread count must be between 1-32. Using default (8)." << endl;
                numThreads = 8;
            }
            
            string filename = downloader.getFileName(url);
            outputPath = "downloads/" + filename;
        }
        else 
        {
            outputPath = arg2;
            
            if (argc >= 4) 
            {
                numThreads = stoi(argv[3]);
                if (numThreads < 1 || numThreads > 32) 
                {
                    cout << "Warning: Thread count must be between 1-32. Using default (8)." << endl;
                    numThreads = 8;
                }
            }
        }
    }
    else 
    {
        string filename = downloader.getFileName(url);
        outputPath = "downloads/" + filename;
    }
    
    cout << "Configuration:" << endl;
    cout << "  URL: " << url << endl;
    cout << "  Output: " << outputPath << endl;
    cout << "  Threads: " << numThreads << endl;
    cout << endl;
    
    bool success = downloader.downloadFile(url, outputPath, numThreads);
    
    if (success) 
    {
        cout << endl;
        cout << "========================================" << endl;
        cout << "  ✓ Download completed successfully!   " << endl;
        cout << "========================================" << endl;
        cout << "File saved to: " << outputPath << endl;
        return 0;
    }
    else 
    {
        cout << endl;
        cout << "========================================" << endl;
        cout << "  ✗ Download failed!                   " << endl;
        cout << "========================================" << endl;
        return 1;
    }
}