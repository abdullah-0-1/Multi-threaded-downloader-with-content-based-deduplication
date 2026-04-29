#include "../include/minheap.h"
#include <stdexcept>

using namespace std;

MinHeap::MinHeap() 
{}

int MinHeap::parent(int index)
{
    return (index - 1) / 2;
}

int MinHeap::leftChild(int index) 
{
    return 2 * index + 1;
}

int MinHeap::rightChild(int index) 
{
    return 2 * index + 2;
}

void MinHeap::swap(int i, int j) 
{
    ChunkTask temp = heap[i];
    heap[i] = heap[j];
    heap[j] = temp;
}

void MinHeap::heapifyUp(int index) 
{
    while (index > 0 && heap[parent(index)] > heap[index]) 
    {
        swap(index, parent(index));
        index = parent(index);
    }
}

void MinHeap::heapifyDown(int index) 
{
    int minIndex = index;
    int left = leftChild(index);
    int right = rightChild(index);
    
    if (left < heap.size() && heap[left].priority < heap[minIndex].priority) 
    {
        minIndex = left;
    }
    
    if (right < heap.size() && heap[right].priority < heap[minIndex].priority) 
    {
        minIndex = right;
    }
    
    if (minIndex != index) 
    {
        swap(index, minIndex);
        heapifyDown(minIndex);
    }
}

void MinHeap::push(const ChunkTask& task) 
{
    heap.push_back(task);
    heapifyUp(heap.size() - 1);
}

ChunkTask MinHeap::pop() 
{
    if (isEmpty()) 
    {
        throw runtime_error("Heap is empty! Cannot pop.");
    }
    
    ChunkTask minTask = heap[0];
    heap[0] = heap.back();
    heap.pop_back();
    
    if (!isEmpty()) 
    {
        heapifyDown(0);
    }
    
    return minTask;
}

ChunkTask MinHeap::top() const 
{
    if (isEmpty()) 
    {
        throw runtime_error("Heap is empty! Cannot access top.");
    }
    
    return heap[0];
}

bool MinHeap::isEmpty() const 
{
    return heap.empty();
}

int MinHeap::size() const 
{
    return heap.size();
}

void MinHeap::clear() 
{
    heap.clear();
}