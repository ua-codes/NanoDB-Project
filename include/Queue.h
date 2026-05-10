#pragma once
#include <stdexcept>

template<typename T>
class Queue {
private:
    T* data;
    int capacity;
    int front_, back_, size_;

    void resize() {
        int newCap = capacity * 2;
        T* nd = new T[newCap];
        for (int i = 0; i < size_; i++) nd[i] = data[(front_ + i) % capacity];
        delete[] data;
        data = nd;
        front_ = 0; back_ = size_;
        capacity = newCap;
    }

public:
    Queue(int cap = 64) : capacity(cap), front_(0), back_(0), size_(0) {
        data = new T[capacity];
    }

    ~Queue() { delete[] data; }

    Queue(const Queue& o) : capacity(o.capacity), front_(0), back_(o.size_), size_(o.size_) {
        data = new T[capacity];
        for (int i = 0; i < size_; i++) data[i] = o.data[(o.front_ + i) % o.capacity];
    }

    Queue& operator=(const Queue& o) {
        if (this == &o) return *this;
        delete[] data;
        capacity = o.capacity; size_ = o.size_;
        data = new T[capacity];
        front_ = 0; back_ = size_;
        for (int i = 0; i < size_; i++) data[i] = o.data[(o.front_ + i) % o.capacity];
        return *this;
    }

    void enqueue(const T& val) {
        if (size_ >= capacity) resize();
        data[back_] = val;
        back_ = (back_ + 1) % capacity;
        size_++;
    }

    void dequeue() {
        if (size_ == 0) throw std::underflow_error("Queue underflow");
        front_ = (front_ + 1) % capacity;
        size_--;
    }

    T& front() {
        if (size_ == 0) throw std::underflow_error("Queue empty");
        return data[front_];
    }

    bool empty() const { return size_ == 0; }
    int size() const { return size_; }
    void clear() { front_ = back_ = size_ = 0; }
};

// Min-Heap based Priority Queue
template<typename T>
class PriorityQueue {
private:
    T* data;
    int capacity;
    int size_;

    void resize() {
        int nc = capacity * 2;
        T* nd = new T[nc];
        for (int i = 0; i < size_; i++) nd[i] = data[i];
        delete[] data;
        data = nd; capacity = nc;
    }

    void heapifyUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (data[p] > data[i]) { T tmp = data[p]; data[p] = data[i]; data[i] = tmp; i = p; }
            else break;
        }
    }

    void heapifyDown(int i) {
        while (true) {
            int l = 2*i+1, r = 2*i+2, smallest = i;
            if (l < size_ && data[l] < data[smallest]) smallest = l;
            if (r < size_ && data[r] < data[smallest]) smallest = r;
            if (smallest == i) break;
            T tmp = data[i]; data[i] = data[smallest]; data[smallest] = tmp;
            i = smallest;
        }
    }

public:
    PriorityQueue(int cap = 64) : capacity(cap), size_(0) {
        data = new T[capacity];
    }

    ~PriorityQueue() { delete[] data; }

    void push(const T& val) {
        if (size_ >= capacity) resize();
        data[size_++] = val;
        heapifyUp(size_ - 1);
    }

    void pop() {
        if (size_ == 0) throw std::underflow_error("PriorityQueue empty");
        data[0] = data[--size_];
        if (size_ > 0) heapifyDown(0);
    }

    const T& top() const {
        if (size_ == 0) throw std::underflow_error("PriorityQueue empty");
        return data[0];
    }

    bool empty() const { return size_ == 0; }
    int size() const { return size_; }
};
