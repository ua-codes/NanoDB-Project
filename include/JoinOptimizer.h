#pragma once
#include "DataValue.h"
#include "DynamicArray.h"
#include "Logger.h"
#include <cstring>

struct JoinResult {
    Row* rows;
    int  count;
    int  capacity;
    Schema schema;

    JoinResult() : rows(nullptr), count(0), capacity(0) {}

    ~JoinResult() { delete[] rows; }

    void add(const Row& r) {
        if (count >= capacity) {
            int nc = capacity == 0 ? 64 : capacity * 2;
            Row* nr = new Row[nc];
            for (int i = 0; i < count; i++) nr[i] = rows[i];
            delete[] rows;
            rows = nr; capacity = nc;
        }
        rows[count++] = r;
    }
};

class JoinOptimizer {
public:
    // Nested-loop join between two tables on matching columns
    // Hardcoded join order: smaller table (t1) on outer loop
    static JoinResult* nestedLoopJoin(
        const DynamicArray<Row>& t1, const Schema& s1, const char* col1,
        const DynamicArray<Row>& t2, const Schema& s2, const char* col2)
    {
        int ci1 = s1.colIndex(col1);
        int ci2 = s2.colIndex(col2);

        if (ci1 < 0) { gLogger.error("Join: col '%s' not in '%s'", col1, s1.tableName); return nullptr; }
        if (ci2 < 0) { gLogger.error("Join: col '%s' not in '%s'", col2, s2.tableName); return nullptr; }

        JoinResult* result = new JoinResult();

        // Build combined schema
        result->schema.numCols = 0;
        for (int i = 0; i < s1.numCols && result->schema.numCols < MAX_COLS; i++) {
            char fullName[64];
            snprintf(fullName, 64, "%s.%s", s1.tableName, s1.cols[i].name);
            result->schema.cols[result->schema.numCols] = ColDef(fullName, s1.cols[i].type);
            result->schema.numCols++;
        }
        for (int i = 0; i < s2.numCols && result->schema.numCols < MAX_COLS; i++) {
            char fullName[64];
            snprintf(fullName, 64, "%s.%s", s2.tableName, s2.cols[i].name);
            result->schema.cols[result->schema.numCols] = ColDef(fullName, s2.cols[i].type);
            result->schema.numCols++;
        }

        gLogger.info("JOIN: %s(%d rows) x %s(%d rows) on %s=%s",
            s1.tableName, t1.size(), s2.tableName, t2.size(), col1, col2);

        // Hash join: build hash table on t2 (inner)
        // Use simple array-based buckets
        int HASH_SIZE = 4096;
        // Array of linked-list heads (index into t2)
        int* hashBucket = new int[HASH_SIZE];
        int* hashNext   = new int[t2.size()];
        for (int i = 0; i < HASH_SIZE; i++) hashBucket[i] = -1;
        for (int i = 0; i < t2.size(); i++) hashNext[i] = -1;

        // Build phase
        for (int i = 0; i < t2.size(); i++) {
            int key = t2[i].cols[ci2].asInt();
            int bucket = ((unsigned)key) % HASH_SIZE;
            hashNext[i] = hashBucket[bucket];
            hashBucket[bucket] = i;
        }

        // Probe phase
        int limit = 500; // Limit output for demo
        for (int i = 0; i < t1.size() && result->count < limit; i++) {
            int key = t1[i].cols[ci1].asInt();
            int bucket = ((unsigned)key) % HASH_SIZE;
            int j = hashBucket[bucket];
            while (j >= 0 && result->count < limit) {
                if (t2[j].cols[ci2].asInt() == key) {
                    // Build combined row
                    Row combined;
                    combined.numCols = 0;
                    for (int c = 0; c < t1[i].numCols && combined.numCols < MAX_COLS; c++)
                        combined.cols[combined.numCols++] = t1[i].cols[c];
                    for (int c = 0; c < t2[j].numCols && combined.numCols < MAX_COLS; c++)
                        combined.cols[combined.numCols++] = t2[j].cols[c];
                    result->add(combined);
                }
                j = hashNext[j];
            }
        }

        delete[] hashBucket;
        delete[] hashNext;

        gLogger.info("JOIN result: %d rows", result->count);
        return result;
    }
};
