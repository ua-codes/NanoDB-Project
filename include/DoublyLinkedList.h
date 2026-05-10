#pragma once
#include <stdexcept>

template<typename T>
class DoublyLinkedList {
public:
    struct Node {
        T data;
        Node* prev;
        Node* next;
        Node(const T& d) : data(d), prev(nullptr), next(nullptr) {}
    };

private:
    Node* head_;
    Node* tail_;
    int size_;

public:
    DoublyLinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}

    ~DoublyLinkedList() { clear(); }

    DoublyLinkedList(const DoublyLinkedList& o) : head_(nullptr), tail_(nullptr), size_(0) {
        Node* cur = o.head_;
        while (cur) { push_back(cur->data); cur = cur->next; }
    }

    DoublyLinkedList& operator=(const DoublyLinkedList& o) {
        if (this == &o) return *this;
        clear();
        Node* cur = o.head_;
        while (cur) { push_back(cur->data); cur = cur->next; }
        return *this;
    }

    void push_front(const T& val) {
        Node* n = new Node(val);
        if (!head_) { head_ = tail_ = n; }
        else { n->next = head_; head_->prev = n; head_ = n; }
        size_++;
    }

    void push_back(const T& val) {
        Node* n = new Node(val);
        if (!tail_) { head_ = tail_ = n; }
        else { tail_->next = n; n->prev = tail_; tail_ = n; }
        size_++;
    }

    void pop_front() {
        if (!head_) return;
        Node* tmp = head_;
        head_ = head_->next;
        if (head_) head_->prev = nullptr; else tail_ = nullptr;
        delete tmp; size_--;
    }

    void pop_back() {
        if (!tail_) return;
        Node* tmp = tail_;
        tail_ = tail_->prev;
        if (tail_) tail_->next = nullptr; else head_ = nullptr;
        delete tmp; size_--;
    }

    void remove(Node* n) {
        if (!n) return;
        if (n->prev) n->prev->next = n->next; else head_ = n->next;
        if (n->next) n->next->prev = n->prev; else tail_ = n->prev;
        delete n; size_--;
    }

    Node* move_to_front(Node* n) {
        if (n == head_) return n;
        // detach
        if (n->prev) n->prev->next = n->next; else head_ = n->next;
        if (n->next) n->next->prev = n->prev; else tail_ = n->prev;
        // attach front
        n->prev = nullptr; n->next = head_;
        if (head_) head_->prev = n; else tail_ = n;
        head_ = n;
        return n;
    }

    T& front() { if (!head_) throw std::underflow_error("List empty"); return head_->data; }
    T& back() { if (!tail_) throw std::underflow_error("List empty"); return tail_->data; }
    Node* front_node() { return head_; }
    Node* back_node() { return tail_; }
    Node* head() { return head_; }
    Node* tail() { return tail_; }

    bool empty() const { return size_ == 0; }
    int size() const { return size_; }

    void clear() {
        while (head_) {
            Node* tmp = head_->next;
            delete head_;
            head_ = tmp;
        }
        tail_ = nullptr; size_ = 0;
    }
};
