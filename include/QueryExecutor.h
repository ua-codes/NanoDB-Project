#pragma once
#include "Parser.h"
#include "DataValue.h"
#include "DynamicArray.h"
#include "IndexManager.h"
#include "JoinOptimizer.h"
#include "FileManager.h"
#include "BufferPool.h"
#include "Logger.h"
#include <cstring>
#include <cstdio>

struct TableEntry {
    Schema schema;
    DynamicArray<Row> rows;
    bool loaded;
    TableEntry() : loaded(false) {}
};

class QueryExecutor {
private:
    static const int MAX_REGISTERED_TABLES = 16;
    TableEntry tables[MAX_REGISTERED_TABLES];
    char tableNames[MAX_REGISTERED_TABLES][64];
    int  tableCount;

    IndexManager custIndex;
    BufferPool   bufPool;

    int findTable(const char* name) {
        for (int i = 0; i < tableCount; i++)
            if (strcmp(tableNames[i], name) == 0) return i;
        return -1;
    }

    bool evalCondition(const Row& row, const Schema& schema, const Condition& cond) {
        if (!cond.valid) return true;
        int ci = schema.colIndex(cond.colName);
        // Try stripping table prefix
        if (ci < 0) {
            const char* dot = strchr(cond.colName, '.');
            if (dot) ci = schema.colIndex(dot+1);
        }
        if (ci < 0) return true;

        const DataValue& v = row.cols[ci];
        switch(cond.op) {
            case Op::EQ:  return v == cond.value;
            case Op::NEQ: return v != cond.value;
            case Op::LT:  return v <  cond.value;
            case Op::GT:  return v >  cond.value;
            case Op::LTE: return v <= cond.value;
            case Op::GTE: return v >= cond.value;
        }
        return true;
    }

public:
    QueryExecutor() : tableCount(0), bufPool(64) {}

    bool registerTable(const Schema& schema) {
        if (tableCount >= MAX_REGISTERED_TABLES) return false;
        tables[tableCount].schema = schema;
        strncpy(tableNames[tableCount], schema.tableName, 63);
        tableCount++;
        return true;
    }

    bool loadTable(const char* name) {
        int idx = findTable(name);
        if (idx < 0) return false;
        if (tables[idx].loaded) return true;
        bool ok = gFileManager.loadRows(name, tables[idx].rows);
        tables[idx].loaded = ok;
        // Build index for customer table
        if (ok && strcmp(name, "customer") == 0) {
            custIndex.build(tables[idx].rows, tables[idx].schema);
        }
        return ok;
    }

    bool insertRow(const char* tableName, const Row& row) {
        int idx = findTable(tableName);
        if (idx < 0) { gLogger.error("INSERT: table '%s' not found", tableName); return false; }
        tables[idx].rows.push_back(row);
        tables[idx].loaded = true;
        return true;
    }

    // Execute SELECT and print results, return count
    int executeSelect(const ParsedQuery& q, int limit = 20) {
        int idx = findTable(q.tableName);
        if (idx < 0) { gLogger.error("SELECT: table '%s' not found", q.tableName); return 0; }
        if (!tables[idx].loaded) loadTable(q.tableName);

        auto& t = tables[idx];
        int count = 0;
        char buf[256];

        // Print header
        printf("--- SELECT FROM %s ---\n", q.tableName);
        for (int i = 0; i < t.schema.numCols; i++) {
            printf("%-18s", t.schema.cols[i].name);
        }
        printf("\n");

        // Use index if WHERE on c_custkey
        bool useIndex = false;
        if (q.whereClause.valid && strcmp(q.tableName, "customer") == 0
            && strcmp(q.whereClause.colName, "c_custkey") == 0
            && q.whereClause.op == Op::EQ && custIndex.isBuilt()) {
            useIndex = true;
        }

        if (useIndex) {
            DynamicArray<int> rowIdxs;
            custIndex.lookup(q.whereClause.value.asInt(), rowIdxs);
            for (int i = 0; i < rowIdxs.size() && count < limit; i++) {
                const Row& row = t.rows[rowIdxs[i]];
                for (int c = 0; c < row.numCols; c++) {
                    row.cols[c].toString(buf, sizeof(buf));
                    printf("%-18s", buf);
                }
                printf("\n"); count++;
            }
        } else {
            for (int i = 0; i < t.rows.size() && count < limit; i++) {
                if (evalCondition(t.rows[i], t.schema, q.whereClause)) {
                    const Row& row = t.rows[i];
                    for (int c = 0; c < row.numCols; c++) {
                        row.cols[c].toString(buf, sizeof(buf));
                        printf("%-18s", buf);
                    }
                    printf("\n"); count++;
                }
            }
        }
        printf("(%d rows returned)\n\n", count);
        return count;
    }

    int executeUpdate(const ParsedQuery& q) {
        int idx = findTable(q.tableName);
        if (idx < 0) { gLogger.error("UPDATE: table '%s' not found", q.tableName); return 0; }
        if (!tables[idx].loaded) loadTable(q.tableName);
        auto& t = tables[idx];
        int setColIdx = t.schema.colIndex(q.setCol);
        if (setColIdx < 0) { gLogger.error("UPDATE: column '%s' not found", q.setCol); return 0; }

        int count = 0;
        for (int i = 0; i < t.rows.size(); i++) {
            if (evalCondition(t.rows[i], t.schema, q.whereClause)) {
                t.rows[i].cols[setColIdx] = q.setValue;
                count++;
            }
        }
        printf("UPDATE %s: %d rows affected\n\n", q.tableName, count);
        gLogger.info("UPDATE %s: %d rows affected", q.tableName, count);
        return count;
    }

    int executeJoin(const ParsedQuery& q, int limit = 20) {
        if (!q.joinSpec.valid) { gLogger.error("Invalid JOIN spec"); return 0; }
        int i1 = findTable(q.joinSpec.table1);
        int i2 = findTable(q.joinSpec.table2);
        if (i1 < 0) { gLogger.error("JOIN: table '%s' not found", q.joinSpec.table1); return 0; }
        if (i2 < 0) { gLogger.error("JOIN: table '%s' not found", q.joinSpec.table2); return 0; }
        if (!tables[i1].loaded) loadTable(q.joinSpec.table1);
        if (!tables[i2].loaded) loadTable(q.joinSpec.table2);

        JoinResult* result = JoinOptimizer::nestedLoopJoin(
            tables[i1].rows, tables[i1].schema, q.joinSpec.col1,
            tables[i2].rows, tables[i2].schema, q.joinSpec.col2
        );
        if (!result) return 0;

        printf("--- JOIN %s x %s ---\n", q.joinSpec.table1, q.joinSpec.table2);
        // Print first 6 columns of combined schema
        int printCols = result->schema.numCols < 6 ? result->schema.numCols : 6;
        for (int i = 0; i < printCols; i++) printf("%-20s", result->schema.cols[i].name);
        printf("\n");

        char buf[256];
        int count = result->count < limit ? result->count : limit;
        for (int i = 0; i < count; i++) {
            Row& row = result->rows[i];
            int pcols = row.numCols < 6 ? row.numCols : 6;
            for (int c = 0; c < pcols; c++) {
                row.cols[c].toString(buf, sizeof(buf));
                printf("%-20s", buf);
            }
            printf("\n");
        }
        printf("(%d rows returned)\n\n", result->count);
        int totalCount = result->count;
        delete result;
        return totalCount;
    }

    int execute(const ParsedQuery& q, int limit = 20) {
        switch(q.type) {
            case QueryType::SELECT:      return executeSelect(q, limit);
            case QueryType::UPDATE:      return executeUpdate(q);
            case QueryType::JOIN_SELECT: return executeJoin(q, limit);
            case QueryType::INSERT: {
                int idx = findTable(q.tableName);
                if (idx < 0) { gLogger.error("INSERT: table not found"); return 0; }
                Row row; row.numCols = q.numCols;
                for (int i = 0; i < q.numCols; i++) row.cols[i] = q.values[i];
                insertRow(q.tableName, row);
                printf("INSERT INTO %s: 1 row\n\n", q.tableName);
                return 1;
            }
            default:
                printf("Unknown query type\n");
                return 0;
        }
    }

    bool saveTable(const char* name) {
        int idx = findTable(name);
        if (idx < 0 || !tables[idx].loaded) return false;
        return gFileManager.saveRows(name, tables[idx].rows);
    }

    DynamicArray<Row>& getRows(const char* name) {
        int idx = findTable(name);
        static DynamicArray<Row> empty;
        return idx >= 0 ? tables[idx].rows : empty;
    }

    Schema& getSchema(const char* name) {
        int idx = findTable(name);
        static Schema empty;
        return idx >= 0 ? tables[idx].schema : empty;
    }

    BufferPool& getBufferPool() { return bufPool; }
    IndexManager& getIndexManager() { return custIndex; }
};
