#pragma once
#include "DataValue.h"
#include "DynamicArray.h"
#include "Stack.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cctype>

enum class QueryType { UNKNOWN, INSERT, SELECT, UPDATE, JOIN_SELECT };

enum class Op { EQ, NEQ, LT, GT, LTE, GTE };

struct Condition {
    char colName[64];
    Op   op;
    DataValue value;
    bool valid;
    Condition() : op(Op::EQ), valid(false) { colName[0] = '\0'; }
};

struct JoinSpec {
    char table1[64];
    char table2[64];
    char col1[64];
    char col2[64];
    bool valid;
    JoinSpec() : valid(false) {
        table1[0] = table2[0] = col1[0] = col2[0] = '\0';
    }
};

struct ParsedQuery {
    QueryType type;
    char tableName[64];
    char colNames[MAX_COLS][64];  // for INSERT or SELECT columns
    DataValue values[MAX_COLS];   // for INSERT
    int numCols;
    Condition whereClause;
    JoinSpec  joinSpec;
    char setCol[64];
    DataValue setValue;
    bool selectAll;

    ParsedQuery() : type(QueryType::UNKNOWN), numCols(0), selectAll(false) {
        tableName[0] = '\0'; setCol[0] = '\0';
    }
};

class Parser {
private:
    char tokens[256][128];
    int  tokenCount;
    int  pos;

    void tokenize(const char* sql) {
        tokenCount = 0; pos = 0;
        char buf[2048]; strncpy(buf, sql, 2047); buf[2047] = '\0';

        // Normalize: replace commas/parens with spaces, keep quotes
        char cleaned[2048]; int ci = 0;
        bool inQuote = false;
        for (int i = 0; buf[i] && ci < 2047; i++) {
            char c = buf[i];
            if (c == '\'') {
                inQuote = !inQuote;
                cleaned[ci++] = c;
            } else if (!inQuote && (c == ',' || c == '(' || c == ')')) {
                cleaned[ci++] = ' ';
            } else {
                cleaned[ci++] = c;
            }
        }
        cleaned[ci] = '\0';

        // Split by whitespace
        char* p = cleaned;
        while (*p) {
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
            if (!*p) break;

            if (*p == '\'') {
                // Quoted string token
                p++; // skip opening quote
                int ti = 0;
                while (*p && *p != '\'') {
                    if (ti < 127) tokens[tokenCount][ti++] = *p;
                    p++;
                }
                tokens[tokenCount][ti] = '\0';
                if (*p == '\'') p++; // skip closing quote
                tokenCount++;
            } else {
                int ti = 0;
                while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
                    if (ti < 127) tokens[tokenCount][ti++] = *p;
                    p++;
                }
                tokens[tokenCount][ti] = '\0';
                tokenCount++;
            }
            if (tokenCount >= 255) break;
        }
    }

    bool match(const char* s) {
        if (pos >= tokenCount) return false;
        // Case-insensitive compare
        int i = 0;
        while (s[i] && tokens[pos][i]) {
            if (toupper((unsigned char)s[i]) != toupper((unsigned char)tokens[pos][i])) return false;
            i++;
        }
        return s[i] == '\0' && tokens[pos][i] == '\0';
    }

    bool consume(const char* s) {
        if (match(s)) { pos++; return true; }
        return false;
    }

    const char* peek() {
        if (pos >= tokenCount) return "";
        return tokens[pos];
    }

    const char* next() {
        if (pos >= tokenCount) return "";
        return tokens[pos++];
    }

    Op parseOp(const char* s) {
        if (strcmp(s, "=")  == 0) return Op::EQ;
        if (strcmp(s, "!=") == 0 || strcmp(s, "<>") == 0) return Op::NEQ;
        if (strcmp(s, "<")  == 0) return Op::LT;
        if (strcmp(s, ">")  == 0) return Op::GT;
        if (strcmp(s, "<=") == 0) return Op::LTE;
        if (strcmp(s, ">=") == 0) return Op::GTE;
        return Op::EQ;
    }

    DataValue parseValue(const char* s) {
        // Check if integer
        bool isNum = true, hasDot = false;
        for (int i = (s[0]=='-'?1:0); s[i]; i++) {
            if (s[i] == '.') { hasDot = true; }
            else if (!isdigit((unsigned char)s[i])) { isNum = false; break; }
        }
        if (isNum && !hasDot && s[0] != '\0') return DataValue(atoi(s));
        if (isNum && hasDot)                  return DataValue(atof(s));
        return DataValue(s);
    }

    Condition parseCondition() {
        Condition c;
        if (pos + 2 >= tokenCount) return c;
        strncpy(c.colName, next(), 63);
        const char* opStr = next();
        c.op = parseOp(opStr);
        c.value = parseValue(next());
        c.valid = true;
        return c;
    }

public:
    Parser() : tokenCount(0), pos(0) {}

    ParsedQuery parse(const char* sql) {
        tokenize(sql);
        pos = 0;
        ParsedQuery q;

        if (match("INSERT")) {
            q.type = QueryType::INSERT;
            consume("INSERT"); consume("INTO");
            strncpy(q.tableName, next(), 63);
            consume("VALUES");
            while (pos < tokenCount) {
                q.values[q.numCols] = parseValue(next());
                q.numCols++;
                if (q.numCols >= MAX_COLS) break;
            }
        } else if (match("SELECT")) {
            consume("SELECT");
            // Check for JOIN
            // Collect select columns
            if (match("*")) { q.selectAll = true; next(); }
            else {
                while (pos < tokenCount && !match("FROM")) {
                    strncpy(q.colNames[q.numCols++], next(), 63);
                    if (q.numCols >= MAX_COLS) break;
                }
            }
            consume("FROM");
            strncpy(q.tableName, next(), 63);

            if (consume("JOIN") || match("INNER")) {
                consume("INNER"); consume("JOIN");
                q.type = QueryType::JOIN_SELECT;
                strncpy(q.joinSpec.table2, next(), 63);
                strncpy(q.joinSpec.table1, q.tableName, 63);
                consume("ON");
                // t1.col1 = t2.col2
                const char* lhs = next(); // table1.col1
                consume("=");
                const char* rhs = next(); // table2.col2
                // parse dot notation
                const char* dot1 = strchr(lhs, '.');
                const char* dot2 = strchr(rhs, '.');
                if (dot1) strncpy(q.joinSpec.col1, dot1+1, 63);
                else strncpy(q.joinSpec.col1, lhs, 63);
                if (dot2) strncpy(q.joinSpec.col2, dot2+1, 63);
                else strncpy(q.joinSpec.col2, rhs, 63);
                q.joinSpec.valid = true;
            } else {
                q.type = QueryType::SELECT;
            }

            if (consume("WHERE")) {
                q.whereClause = parseCondition();
            }
        } else if (match("UPDATE")) {
            q.type = QueryType::UPDATE;
            consume("UPDATE");
            strncpy(q.tableName, next(), 63);
            consume("SET");
            strncpy(q.setCol, next(), 63);
            consume("=");
            q.setValue = parseValue(next());
            if (consume("WHERE")) {
                q.whereClause = parseCondition();
            }
        }

        return q;
    }

    // Infix to Postfix conversion (for expression evaluation demo)
    static void infixToPostfix(const char* infix, char* postfix, int postfixLen) {
        Stack<char> opStack(64);
        char out[1024]; int oi = 0;
        bool prevWasNum = false;

        auto prec = [](char op) -> int {
            if (op == '+' || op == '-') return 1;
            if (op == '*' || op == '/') return 2;
            return 0;
        };

        for (int i = 0; infix[i]; i++) {
            char c = infix[i];
            if (c == ' ') continue;
            if (isdigit((unsigned char)c) || c == '.') {
                if (prevWasNum && oi > 0 && out[oi-1] != ' ') out[oi++] = ' ';
                out[oi++] = c;
                prevWasNum = true;
            } else if (c == '(') {
                opStack.push(c); prevWasNum = false;
            } else if (c == ')') {
                while (!opStack.empty() && opStack.top() != '(') {
                    out[oi++] = ' '; out[oi++] = opStack.top(); opStack.pop();
                }
                if (!opStack.empty()) opStack.pop();
                prevWasNum = false;
            } else if (c=='+' || c=='-' || c=='*' || c=='/') {
                out[oi++] = ' ';
                while (!opStack.empty() && prec(opStack.top()) >= prec(c)) {
                    out[oi++] = opStack.top(); out[oi++] = ' '; opStack.pop();
                }
                opStack.push(c); prevWasNum = false;
            }
        }
        while (!opStack.empty()) {
            out[oi++] = ' '; out[oi++] = opStack.top(); opStack.pop();
        }
        out[oi] = '\0';
        strncpy(postfix, out, postfixLen-1);
        postfix[postfixLen-1] = '\0';
    }
};
