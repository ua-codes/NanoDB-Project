#pragma once
#include "DoublyLinkedList.h"
#include "HashMap.h"
#include "Logger.h"

static const int PAGE_SIZE = 4096;
static const int DEFAULT_POOL_SIZE = 64;

struct Page {
    int pageId;
    char data[PAGE_SIZE];
    bool dirty;
    int  tableId;
    Page() : pageId(-1), dirty(false), tableId(-1) { }
};

struct LRUEntry {
    int pageId;
    Page* page;
};

class BufferPool {
private:
    Page* pages;          // pool of page frames
    int poolSize;
    int* freeList;
    int freeCount;

    // LRU list of LRUEntry
    DoublyLinkedList<LRUEntry> lruList;
    // pageId -> list node
    HashMap<int, DoublyLinkedList<LRUEntry>::Node*> pageMap;

    int hits, misses;

    int allocFrame() {
        if (freeCount > 0) return freeList[--freeCount];
        return evict();
    }

    int evict() {
        // evict LRU (tail of list)
        if (lruList.empty()) return -1;
        auto* node = lruList.back_node();
        int pid = node->data.pageId;
        Page* p = node->data.page;
        if (p->dirty) {
            // Would flush to disk here
            gLogger.debug("Evicting dirty page %d", pid);
            p->dirty = false;
        }
        lruList.pop_back();
        pageMap.erase(pid);
        // return this frame index
        return (int)(p - pages);
    }

public:
    BufferPool(int size = DEFAULT_POOL_SIZE) : poolSize(size), freeCount(size), hits(0), misses(0) {
        pages = new Page[poolSize];
        freeList = new int[poolSize];
        for (int i = 0; i < poolSize; i++) freeList[i] = i;
    }

    ~BufferPool() { delete[] pages; delete[] freeList; }

    Page* getPage(int pageId, int tableId) {
        DoublyLinkedList<LRUEntry>::Node* node = nullptr;
        if (pageMap.find(pageId, node)) {
            hits++;
            lruList.move_to_front(node);
            return node->data.page;
        }
        misses++;
        int frame = allocFrame();
        if (frame < 0) { gLogger.error("Buffer pool full!"); return nullptr; }
        Page* p = &pages[frame];
        p->pageId = pageId;
        p->tableId = tableId;
        p->dirty = false;
        LRUEntry entry{pageId, p};
        lruList.push_front(entry);
        pageMap.insert(pageId, lruList.front_node());
        return p;
    }

    void markDirty(int pageId) {
        DoublyLinkedList<LRUEntry>::Node* node = nullptr;
        if (pageMap.find(pageId, node)) node->data.page->dirty = true;
    }

    void flushAll() {
        auto* cur = lruList.head();
        while (cur) {
            if (cur->data.page->dirty) {
                gLogger.debug("Flushing page %d", cur->data.pageId);
                cur->data.page->dirty = false;
            }
            cur = cur->next;
        }
    }

    int getHits()   const { return hits; }
    int getMisses() const { return misses; }
    int getPoolSize() const { return poolSize; }
    double hitRate() const {
        int total = hits + misses;
        return total > 0 ? (double)hits / total * 100.0 : 0.0;
    }
};
