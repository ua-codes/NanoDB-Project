#pragma once
#include "AVLTree.h"
#include "DynamicArray.h"
#include "Logger.h"
#include <cstring>

// Maps a key (int) -> list of row indices
struct RowList {
    int* indices;
    int  count;
    int  capacity;

    RowList() : indices(nullptr), count(0), capacity(0) {}

    RowList(const RowList& o) : count(o.count), capacity(o.capacity) {
        if (capacity > 0) {
            indices = new int[capacity];
            for (int i = 0; i < count; i++) indices[i] = o.indices[i];
        } else indices = nullptr;
    }

    RowList& operator=(const RowList& o) {
        if (this == &o) return *this;
        delete[] indices;
        count = o.count; capacity = o.capacity;
        if (capacity > 0) {
            indices = new int[capacity];
            for (int i = 0; i < count; i++) indices[i] = o.indices[i];
        } else indices = nullptr;
        return *this;
    }

    ~RowList() { delete[] indices; }

    void add(int idx) {
        if (count >= capacity) {
            int nc = capacity == 0 ? 4 : capacity * 2;
            int* ni = new int[nc];
            for (int i = 0; i < count; i++) ni[i] = indices[i];
            delete[] indices;
            indices = ni; capacity = nc;
        }
        indices[count++] = idx;
    }
};

class IndexManager {
private:
    AVLTree<int, RowList> custIndex;  // c_custkey -> RowList
    bool built;
    char indexedTable[64];
    char indexedCol[64];

public:
    IndexManager() : built(false) {
        strncpy(indexedTable, "customer", 63);
        strncpy(indexedCol, "c_custkey", 63);
    }

    void build(const DynamicArray<Row>& rows, const Schema& schema) {
        int colIdx = schema.colIndex(indexedCol);
        if (colIdx < 0) {
            gLogger.warn("IndexManager: column '%s' not found in schema", indexedCol);
            return;
        }
        for (int i = 0; i < rows.size(); i++) {
            int key = rows[i].cols[colIdx].asInt();
            RowList dummy;
            RowList* existing = nullptr;
            // Find existing or insert new
            RowList rl;
            if (!custIndex.find(key, rl)) {
                rl = RowList();
            }
            rl.add(i);
            custIndex.insert(key, rl);
        }
        built = true;
        gLogger.info("Index built on %s.%s (%d entries)", indexedTable, indexedCol, custIndex.size());
    }

    // Lookup rows by exact custkey
    void lookup(int key, DynamicArray<int>& outRowIndices) {
        if (!built) return;
        RowList rl;
        if (custIndex.find(key, rl)) {
            for (int i = 0; i < rl.count; i++) outRowIndices.push_back(rl.indices[i]);
        }
    }

    // Range lookup [lo, hi]
    void rangeLookup(int lo, int hi, DynamicArray<int>& outRowIndices) {
        if (!built) return;
        DynamicArray<int> keys;
        DynamicArray<RowList> vals;
        custIndex.rangeQuery(lo, hi, keys, vals);
        for (int i = 0; i < vals.size(); i++) {
            RowList& rl = vals[i];
            for (int j = 0; j < rl.count; j++) outRowIndices.push_back(rl.indices[j]);
        }
    }

    bool isBuilt() const { return built; }
    int size() const { return custIndex.size(); }
};
