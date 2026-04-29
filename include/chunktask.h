#pragma once

using namespace std;

struct ChunkTask 
{
    int id;
    long startByte;
    long endByte;
    int priority;
    
    ChunkTask(int taskId, long start, long end) 
    {
        id = taskId;
        startByte = start;
        endByte = end;
        priority = start;
    }
    
    ChunkTask() 
    {
        id = 0;
        startByte = 0;
        endByte = 0;
        priority = 0;
    }
    
    bool operator>(const ChunkTask& other) const 
    {
        return priority > other.priority;
    }
    
    long getSize() const 
    {
        return endByte - startByte + 1;
    }
};