#pragma once

#include <string>
#include <vector>
using namespace std;

template<typename K, typename V>
struct HashNode 
{
    K key;
    V value;
    HashNode* next;
    
    HashNode(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
};

template<typename K, typename V>
class HashMap 
{
private:
    HashNode<K, V>** table;
    int capacity;
    int count;
    
    int hash(const string& key) const 
    {
        unsigned long hashValue = 5381;
        for (char c : key) 
        {
            hashValue = ((hashValue << 5) + hashValue) + c;
        }
        return hashValue % capacity;
    }
    
    template<typename T>
    int hash(const T& key) const 
    {
        return (int)(key % capacity);
    }
    
public:
    HashMap(int cap = 10007) : capacity(cap), count(0)
    {
        table = new HashNode<K, V>*[capacity];
        for (int i = 0; i < capacity; i++) 
        {
            table[i] = nullptr;
        }
    }
    
    ~HashMap() 
    {
        clear();
        delete[] table;
    }
    
    void insert(const K& key, const V& value) 
    {
        int index = hash(key);
        HashNode<K, V>* current = table[index];
        
        while (current != nullptr) 
        {
            if (current->key == key) 
            {
                current->value = value;
                return;
            }
            current = current->next;
        }
        
        HashNode<K, V>* newNode = new HashNode<K, V>(key, value);
        newNode->next = table[index];
        table[index] = newNode;
        count++;
    }
    
    V* find(const K& key) 
    {
        int index = hash(key);
        HashNode<K, V>* current = table[index];
        
        while (current != nullptr) 
        {
            if (current->key == key) 
            {
                return &current->value;
            }
            current = current->next;
        }
        
        return nullptr;
    }
    
    bool contains(const K& key) const 
    {
        int index = hash(key);
        HashNode<K, V>* current = table[index];
        
        while (current != nullptr) 
        {
            if (current->key == key) 
            {
                return true;
            }
            current = current->next;
        }
        
        return false;
    }
    
    int size() const 
    {
        return count;
    }
    
    void clear() 
    {
        for (int i = 0; i < capacity; i++) 
        {
            HashNode<K, V>* current = table[i];
            while (current != nullptr) 
            {
                HashNode<K, V>* temp = current;
                current = current->next;
                delete temp;
            }
            table[i] = nullptr;
        }
        count = 0;
    }
    
    V& operator[](const K& key) 
    {
        int index = hash(key);
        HashNode<K, V>* current = table[index];
        
        while (current != nullptr) 
        {
            if (current->key == key) 
            {
                return current->value;
            }
            current = current->next;
        }
        
        HashNode<K, V>* newNode = new HashNode<K, V>(key, V());
        newNode->next = table[index];
        table[index] = newNode;
        count++;
        
        return newNode->value;
    }
    
    class Iterator 
    {
    private:
        HashMap<K, V>* map;
        int bucketIndex;
        HashNode<K, V>* current;
        
        void advance() 
        {
            if (current != nullptr && current->next != nullptr) 
            {
                current = current->next;
                return;
            }
            
            current = nullptr;
            bucketIndex++;
            while (bucketIndex < map->capacity && map->table[bucketIndex] == nullptr) 
            {
                bucketIndex++;
            }
            
            if (bucketIndex < map->capacity) 
            {
                current = map->table[bucketIndex];
            }
        }
        
    public:
        Iterator(HashMap<K, V>* m, int bucket, HashNode<K, V>* node) 
            : map(m), bucketIndex(bucket), current(node) {}
        
        pair<K, V> operator*() const 
        {
            return {current->key, current->value};
        }
        
        Iterator& operator++() 
        {
            advance();
            return *this;
        }
        
        bool operator!=(const Iterator& other) const 
        {
            return current != other.current || bucketIndex != other.bucketIndex;
        }
    };
    
    Iterator begin() 
    {
        for (int i = 0; i < capacity; i++) 
        {
            if (table[i] != nullptr) 
            {
                return Iterator(this, i, table[i]);
            }
        }
        return end();
    }
    
    Iterator end() 
    {
        return Iterator(this, capacity, nullptr);
    }
};
