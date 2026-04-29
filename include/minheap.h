#pragma once

#include <vector>
#include "chunktask.h"

using namespace std;
class MinHeap 
{
private:
    vector<ChunkTask> heap;
    
    int parent(int index);
    int leftChild(int index);
    int rightChild(int index);
    
    void heapifyUp(int index);
    void heapifyDown(int index);
    void swap(int i, int j);
    
public:
    MinHeap();
    
    void push(const ChunkTask& task);
    ChunkTask pop();
    ChunkTask top() const;
    bool isEmpty() const;
    int size() const;
    
    void clear();
};