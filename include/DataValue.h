#pragma once
#include <cstring>
#include <cstdio>
#include <cstdlib>

enum class DataType { INT, DOUBLE, STRING, NULLVAL };

class DataValue {
public:
    DataType type;
    int    ival;
    double dval;
    char   sval[256];

    DataValue() : type(DataType::NULLVAL), ival(0), dval(0.0) { sval[0] = '\0'; }
    DataValue(int v) : type(DataType::INT), ival(v), dval(0.0) { sval[0] = '\0'; }
    DataValue(double v) : type(DataType::DOUBLE), ival(0), dval(v) { sval[0] = '\0'; }
    DataValue(const char* v) : type(DataType::STRING), ival(0), dval(0.0) {
        strncpy(sval, v, 255); sval[255] = '\0';
    }

    bool operator==(const DataValue& o) const {
        if (type != o.type) return false;
        switch(type) {
            case DataType::INT: return ival == o.ival;
            case DataType::DOUBLE: return dval == o.dval;
            case DataType::STRING: return strcmp(sval, o.sval) == 0;
            default: return true;
        }
    }
    bool operator!=(const DataValue& o) const { return !(*this == o); }

    bool operator<(const DataValue& o) const {
        if (type == DataType::INT && o.type == DataType::INT) return ival < o.ival;
        if (type == DataType::DOUBLE && o.type == DataType::DOUBLE) return dval < o.dval;
        if (type == DataType::INT && o.type == DataType::DOUBLE) return (double)ival < o.dval;
        if (type == DataType::DOUBLE && o.type == DataType::INT) return dval < (double)o.ival;
        if (type == DataType::STRING && o.type == DataType::STRING) return strcmp(sval, o.sval) < 0;
        return false;
    }

    bool operator>(const DataValue& o) const { return o < *this; }
    bool operator<=(const DataValue& o) const { return !(o < *this); }
    bool operator>=(const DataValue& o) const { return !(*this < o); }

    void toString(char* buf, int bufLen) const {
        switch(type) {
            case DataType::INT:    snprintf(buf, bufLen, "%d", ival); break;
            case DataType::DOUBLE: snprintf(buf, bufLen, "%.2f", dval); break;
            case DataType::STRING: snprintf(buf, bufLen, "%s", sval); break;
            default:               snprintf(buf, bufLen, "NULL"); break;
        }
    }

    int asInt() const {
        if (type == DataType::INT) return ival;
        if (type == DataType::DOUBLE) return (int)dval;
        if (type == DataType::STRING) return atoi(sval);
        return 0;
    }

    double asDouble() const {
        if (type == DataType::DOUBLE) return dval;
        if (type == DataType::INT) return (double)ival;
        if (type == DataType::STRING) return atof(sval);
        return 0.0;
    }
};

// A row is a fixed-size array of DataValues
static const int MAX_COLS = 32;

struct Row {
    DataValue cols[MAX_COLS];
    int numCols;
    Row() : numCols(0) {}
};

// Schema column definition
struct ColDef {
    char name[64];
    DataType type;
    ColDef() { name[0] = '\0'; type = DataType::STRING; }
    ColDef(const char* n, DataType t) : type(t) {
        strncpy(name, n, 63); name[63] = '\0';
    }
};

struct Schema {
    char tableName[64];
    ColDef cols[MAX_COLS];
    int numCols;
    Schema() : numCols(0) { tableName[0] = '\0'; }

    int colIndex(const char* colName) const {
        for (int i = 0; i < numCols; i++)
            if (strcmp(cols[i].name, colName) == 0) return i;
        return -1;
    }
};
