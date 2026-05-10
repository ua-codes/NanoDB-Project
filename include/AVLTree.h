#pragma once
#include "DynamicArray.h"

template<typename K, typename V>
class AVLTree {
public:
    struct Node {
        K key;
        V value;
        Node* left;
        Node* right;
        int height;
        Node(const K& k, const V& v) : key(k), value(v), left(nullptr), right(nullptr), height(1) {}
    };

private:
    Node* root_;
    int size_;

    int height(Node* n) { return n ? n->height : 0; }

    int bf(Node* n) { return n ? height(n->left) - height(n->right) : 0; }

    void updateH(Node* n) {
        if (!n) return;
        int lh = height(n->left), rh = height(n->right);
        n->height = 1 + (lh > rh ? lh : rh);
    }

    Node* rotR(Node* y) {
        Node* x = y->left; Node* T2 = x->right;
        x->right = y; y->left = T2;
        updateH(y); updateH(x);
        return x;
    }

    Node* rotL(Node* x) {
        Node* y = x->right; Node* T2 = y->left;
        y->left = x; x->right = T2;
        updateH(x); updateH(y);
        return y;
    }

    Node* balance(Node* n) {
        updateH(n);
        int b = bf(n);
        if (b > 1) {
            if (bf(n->left) < 0) n->left = rotL(n->left);
            return rotR(n);
        }
        if (b < -1) {
            if (bf(n->right) > 0) n->right = rotR(n->right);
            return rotL(n);
        }
        return n;
    }

    Node* insert(Node* n, const K& key, const V& val, bool& inserted) {
        if (!n) { inserted = true; size_++; return new Node(key, val); }
        if (key < n->key) n->left = insert(n->left, key, val, inserted);
        else if (key > n->key) n->right = insert(n->right, key, val, inserted);
        else { n->value = val; return n; }
        return balance(n);
    }

    Node* minNode(Node* n) { while (n->left) n = n->left; return n; }

    Node* remove(Node* n, const K& key, bool& removed) {
        if (!n) return nullptr;
        if (key < n->key) n->left = remove(n->left, key, removed);
        else if (key > n->key) n->right = remove(n->right, key, removed);
        else {
            removed = true; size_--;
            if (!n->left || !n->right) {
                Node* child = n->left ? n->left : n->right;
                delete n; return child;
            }
            Node* succ = minNode(n->right);
            n->key = succ->key; n->value = succ->value;
            bool dummy = false;
            n->right = remove(n->right, succ->key, dummy);
            size_++; // compensate double decrement
        }
        return balance(n);
    }

    Node* find(Node* n, const K& key) const {
        if (!n) return nullptr;
        if (key < n->key) return find(n->left, key);
        if (key > n->key) return find(n->right, key);
        return n;
    }

    void inorder(Node* n, DynamicArray<K>& keys, DynamicArray<V>& vals) const {
        if (!n) return;
        inorder(n->left, keys, vals);
        keys.push_back(n->key);
        vals.push_back(n->value);
        inorder(n->right, keys, vals);
    }

    void destroy(Node* n) {
        if (!n) return;
        destroy(n->left); destroy(n->right); delete n;
    }

public:
    AVLTree() : root_(nullptr), size_(0) {}
    ~AVLTree() { destroy(root_); }

    void insert(const K& key, const V& val) {
        bool inserted = false;
        root_ = insert(root_, key, val, inserted);
    }

    bool remove(const K& key) {
        bool removed = false;
        root_ = remove(root_, key, removed);
        return removed;
    }

    bool find(const K& key, V& out) const {
        Node* n = find(root_, key);
        if (n) { out = n->value; return true; }
        return false;
    }

    bool contains(const K& key) const {
        V dummy;
        return find(key, dummy);
    }

    int size() const { return size_; }
    bool empty() const { return size_ == 0; }
    Node* root() const { return root_; }

    void inorder(DynamicArray<K>& keys, DynamicArray<V>& vals) const {
        inorder(root_, keys, vals);
    }

    // Range query: find all keys in [lo, hi]
    void rangeQuery(Node* n, const K& lo, const K& hi,
                    DynamicArray<K>& keys, DynamicArray<V>& vals) const {
        if (!n) return;
        if (lo < n->key) rangeQuery(n->left, lo, hi, keys, vals);
        if (n->key >= lo && n->key <= hi) { keys.push_back(n->key); vals.push_back(n->value); }
        if (hi > n->key) rangeQuery(n->right, lo, hi, keys, vals);
    }

    void rangeQuery(const K& lo, const K& hi,
                    DynamicArray<K>& keys, DynamicArray<V>& vals) const {
        rangeQuery(root_, lo, hi, keys, vals);
    }
};
