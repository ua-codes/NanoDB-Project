#pragma once
#include <cstring>
#include <functional>

template<typename K, typename V>
class HashMap {
private:
    struct Entry {
        K key;
        V value;
        Entry* next;
        Entry(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    Entry** buckets;
    int bucketCount;
    int size_;

    int hash(const K& key) const {
        // Generic hash via std::hash but no containers
        return (int)(std::hash<K>{}(key) % (unsigned)bucketCount);
    }

    void rehash() {
        int newCount = bucketCount * 2;
        Entry** newBuckets = new Entry*[newCount];
        for (int i = 0; i < newCount; i++) newBuckets[i] = nullptr;

        for (int i = 0; i < bucketCount; i++) {
            Entry* e = buckets[i];
            while (e) {
                Entry* next = e->next;
                int nb = (int)(std::hash<K>{}(e->key) % (unsigned)newCount);
                e->next = newBuckets[nb];
                newBuckets[nb] = e;
                e = next;
            }
        }
        delete[] buckets;
        buckets = newBuckets;
        bucketCount = newCount;
    }

public:
    HashMap(int buckets_ = 64) : bucketCount(buckets_), size_(0) {
        buckets = new Entry*[bucketCount];
        for (int i = 0; i < bucketCount; i++) buckets[i] = nullptr;
    }

    ~HashMap() {
        for (int i = 0; i < bucketCount; i++) {
            Entry* e = buckets[i];
            while (e) { Entry* nx = e->next; delete e; e = nx; }
        }
        delete[] buckets;
    }

    HashMap(const HashMap& o) : bucketCount(o.bucketCount), size_(0) {
        buckets = new Entry*[bucketCount];
        for (int i = 0; i < bucketCount; i++) buckets[i] = nullptr;
        for (int i = 0; i < o.bucketCount; i++) {
            Entry* e = o.buckets[i];
            while (e) { insert(e->key, e->value); e = e->next; }
        }
    }

    HashMap& operator=(const HashMap& o) {
        if (this == &o) return *this;
        this->~HashMap();
        new(this) HashMap(o);
        return *this;
    }

    void insert(const K& key, const V& val) {
        if (size_ > bucketCount * 3) rehash();
        int b = hash(key);
        Entry* e = buckets[b];
        while (e) {
            if (e->key == key) { e->value = val; return; }
            e = e->next;
        }
        Entry* ne = new Entry(key, val);
        ne->next = buckets[b];
        buckets[b] = ne;
        size_++;
    }

    bool find(const K& key, V& out) const {
        int b = hash(key);
        Entry* e = buckets[b];
        while (e) {
            if (e->key == key) { out = e->value; return true; }
            e = e->next;
        }
        return false;
    }

    bool contains(const K& key) const {
        V dummy;
        return find(key, dummy);
    }

    V& operator[](const K& key) {
        if (size_ > bucketCount * 3) rehash();
        int b = hash(key);
        Entry* e = buckets[b];
        while (e) {
            if (e->key == key) return e->value;
            e = e->next;
        }
        Entry* ne = new Entry(key, V{});
        ne->next = buckets[b];
        buckets[b] = ne;
        size_++;
        return ne->value;
    }

    bool erase(const K& key) {
        int b = hash(key);
        Entry* e = buckets[b]; Entry* prev = nullptr;
        while (e) {
            if (e->key == key) {
                if (prev) prev->next = e->next; else buckets[b] = e->next;
                delete e; size_--;
                return true;
            }
            prev = e; e = e->next;
        }
        return false;
    }

    int size() const { return size_; }
    bool empty() const { return size_ == 0; }

    // Iterate: call fn(key, value) for each entry
    template<typename Fn>
    void forEach(Fn fn) const {
        for (int i = 0; i < bucketCount; i++) {
            Entry* e = buckets[i];
            while (e) { fn(e->key, e->value); e = e->next; }
        }
    }
};
