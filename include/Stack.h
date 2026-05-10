#pragma once
#include <stdexcept>

template<typename T>
class Stack {
private:
    T* data;
    int capacity;
    int top_;

    void resize() {
        int newCap = capacity * 2;
        T* nd = new T[newCap];
        for (int i = 0; i <= top_; i++) nd[i] = data[i];
        delete[] data;
        data = nd;
        capacity = newCap;
    }

public:
    Stack(int cap = 64) : capacity(cap), top_(-1) {
        data = new T[capacity];
    }

    ~Stack() { delete[] data; }

    Stack(const Stack& o) : capacity(o.capacity), top_(o.top_) {
        data = new T[capacity];
        for (int i = 0; i <= top_; i++) data[i] = o.data[i];
    }

    Stack& operator=(const Stack& o) {
        if (this == &o) return *this;
        delete[] data;
        capacity = o.capacity; top_ = o.top_;
        data = new T[capacity];
        for (int i = 0; i <= top_; i++) data[i] = o.data[i];
        return *this;
    }

    void push(const T& val) {
        if (top_ + 1 >= capacity) resize();
        data[++top_] = val;
    }

    void pop() {
        if (top_ < 0) throw std::underflow_error("Stack underflow");
        top_--;
    }

    T& top() {
        if (top_ < 0) throw std::underflow_error("Stack is empty");
        return data[top_];
    }

    const T& top() const {
        if (top_ < 0) throw std::underflow_error("Stack is empty");
        return data[top_];
    }

    bool empty() const { return top_ < 0; }
    int size() const { return top_ + 1; }
    void clear() { top_ = -1; }
};
