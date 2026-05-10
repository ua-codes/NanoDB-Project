#pragma once
#include "DataValue.h"
#include "DynamicArray.h"
#include "Logger.h"
#include <cstdio>
#include <cstring>

static const int MAX_TABLES = 16;
static const int MAX_ROWS_PER_TABLE = 100000;

class FileManager {
private:
    char dataDir[256];

    void tableFilePath(const char* tableName, char* out, int len) {
        snprintf(out, len, "%s/%s.ndb", dataDir, tableName);
    }

    void schemaFilePath(const char* tableName, char* out, int len) {
        snprintf(out, len, "%s/%s.schema", dataDir, tableName);
    }

public:
    FileManager() { strncpy(dataDir, "data", 255); }

    void setDataDir(const char* dir) {
        strncpy(dataDir, dir, 255);
    }

    bool saveSchema(const Schema& schema) {
        char path[512];
        schemaFilePath(schema.tableName, path, sizeof(path));
        FILE* f = fopen(path, "wb");
        if (!f) { gLogger.error("Cannot open schema file: %s", path); return false; }
        fwrite(&schema, sizeof(Schema), 1, f);
        fclose(f);
        return true;
    }

    bool loadSchema(const char* tableName, Schema& out) {
        char path[512];
        schemaFilePath(tableName, path, sizeof(path));
        FILE* f = fopen(path, "rb");
        if (!f) { gLogger.warn("Schema file not found: %s", path); return false; }
        fread(&out, sizeof(Schema), 1, f);
        fclose(f);
        return true;
    }

    bool saveRows(const char* tableName, const DynamicArray<Row>& rows) {
        char path[512];
        tableFilePath(tableName, path, sizeof(path));
        FILE* f = fopen(path, "wb");
        if (!f) { gLogger.error("Cannot open data file: %s", path); return false; }
        int count = rows.size();
        fwrite(&count, sizeof(int), 1, f);
        for (int i = 0; i < count; i++) fwrite(&rows[i], sizeof(Row), 1, f);
        fclose(f);
        gLogger.info("Saved %d rows to %s", count, path);
        return true;
    }

    bool loadRows(const char* tableName, DynamicArray<Row>& out) {
        char path[512];
        tableFilePath(tableName, path, sizeof(path));
        FILE* f = fopen(path, "rb");
        if (!f) { gLogger.warn("Data file not found: %s", path); return false; }
        int count = 0;
        fread(&count, sizeof(int), 1, f);
        out.clear();
        Row row;
        for (int i = 0; i < count; i++) {
            fread(&row, sizeof(Row), 1, f);
            out.push_back(row);
        }
        fclose(f);
        gLogger.info("Loaded %d rows from %s", count, path);
        return true;
    }

    bool appendRow(const char* tableName, const Row& row) {
        char path[512];
        tableFilePath(tableName, path, sizeof(path));
        // Read existing count
        FILE* f = fopen(path, "r+b");
        if (!f) {
            // Create new file
            f = fopen(path, "wb");
            if (!f) { gLogger.error("Cannot create data file: %s", path); return false; }
            int count = 1;
            fwrite(&count, sizeof(int), 1, f);
            fwrite(&row, sizeof(Row), 1, f);
            fclose(f);
            return true;
        }
        int count = 0;
        fread(&count, sizeof(int), 1, f);
        count++;
        fseek(f, 0, SEEK_SET);
        fwrite(&count, sizeof(int), 1, f);
        fseek(f, 0, SEEK_END);
        fwrite(&row, sizeof(Row), 1, f);
        fclose(f);
        return true;
    }

    bool tableExists(const char* tableName) {
        char path[512];
        schemaFilePath(tableName, path, sizeof(path));
        FILE* f = fopen(path, "rb");
        if (f) { fclose(f); return true; }
        return false;
    }
};

extern FileManager gFileManager;
