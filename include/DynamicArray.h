#pragma once
#include <cstring>
#include <stdexcept>

template<typename T>
class DynamicArray {
private:
    T* data;
    int capacity;
    int size_;

    void resize() {
        int newCap = capacity * 2;
        T* newData = new T[newCap];
        for (int i = 0; i < size_; i++) newData[i] = data[i];
        delete[] data;
        data = newData;
        capacity = newCap;
    }

public:
    DynamicArray(int initialCap = 16) : capacity(initialCap), size_(0) {
        data = new T[capacity];
    }

    ~DynamicArray() { delete[] data; }

    DynamicArray(const DynamicArray& o) : capacity(o.capacity), size_(o.size_) {
        data = new T[capacity];
        for (int i = 0; i < size_; i++) data[i] = o.data[i];
    }

    DynamicArray& operator=(const DynamicArray& o) {
        if (this == &o) return *this;
        delete[] data;
        capacity = o.capacity; size_ = o.size_;
        data = new T[capacity];
        for (int i = 0; i < size_; i++) data[i] = o.data[i];
        return *this;
    }

    void push_back(const T& val) {
        if (size_ >= capacity) resize();
        data[size_++] = val;
    }

    void pop_back() {
        if (size_ > 0) size_--;
    }

    T& operator[](int i) { return data[i]; }
    const T& operator[](int i) const { return data[i]; }

    T& at(int i) {
        if (i < 0 || i >= size_) throw std::out_of_range("DynamicArray: index out of range");
        return data[i];
    }

    int size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear() { size_ = 0; }

    T* begin() { return data; }
    T* end() { return data + size_; }
    const T* begin() const { return data; }
    const T* end() const { return data + size_; }

    void remove_at(int idx) {
        if (idx < 0 || idx >= size_) return;
        for (int i = idx; i < size_ - 1; i++) data[i] = data[i+1];
        size_--;
    }

    int find(const T& val) const {
        for (int i = 0; i < size_; i++) if (data[i] == val) return i;
        return -1;
    }
};
