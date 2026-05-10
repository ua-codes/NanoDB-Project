#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdlib>

#include "DynamicArray.h"
#include "Stack.h"
#include "Queue.h"
#include "DoublyLinkedList.h"
#include "HashMap.h"
#include "AVLTree.h"
#include "DataValue.h"
#include "Parser.h"
#include "Graph.h"

// ─── Test framework ──────────────────────────────────────────────────────────
static int passed = 0, failed = 0;

#define TEST(name) void test_##name()
#define RUN(name) do { \
    printf("  Running %-40s", #name "..."); \
    try { test_##name(); printf("PASS\n"); passed++; } \
    catch(...) { printf("FAIL\n"); failed++; } \
} while(0)

#define CHECK(cond) do { if(!(cond)) { printf("\n    ASSERTION FAILED: %s (line %d)\n", #cond, __LINE__); throw 0; } } while(0)

// ─── DynamicArray Tests ───────────────────────────────────────────────────────
TEST(dynamic_array_push_pop) {
    DynamicArray<int> arr;
    for (int i = 0; i < 100; i++) arr.push_back(i);
    CHECK(arr.size() == 100);
    CHECK(arr[0] == 0);
    CHECK(arr[99] == 99);
    arr.pop_back();
    CHECK(arr.size() == 99);
}

TEST(dynamic_array_resize) {
    DynamicArray<int> arr(4);
    for (int i = 0; i < 64; i++) arr.push_back(i);
    CHECK(arr.size() == 64);
    CHECK(arr[63] == 63);
}

TEST(dynamic_array_remove_at) {
    DynamicArray<int> arr;
    arr.push_back(10); arr.push_back(20); arr.push_back(30);
    arr.remove_at(1);
    CHECK(arr.size() == 2);
    CHECK(arr[0] == 10);
    CHECK(arr[1] == 30);
}

TEST(dynamic_array_find) {
    DynamicArray<int> arr;
    arr.push_back(5); arr.push_back(15); arr.push_back(25);
    CHECK(arr.find(15) == 1);
    CHECK(arr.find(99) == -1);
}

// ─── Stack Tests ──────────────────────────────────────────────────────────────
TEST(stack_basic) {
    Stack<int> s;
    CHECK(s.empty());
    s.push(1); s.push(2); s.push(3);
    CHECK(s.size() == 3);
    CHECK(s.top() == 3);
    s.pop();
    CHECK(s.top() == 2);
    s.pop(); s.pop();
    CHECK(s.empty());
}

TEST(stack_resize) {
    Stack<int> s(4);
    for (int i = 0; i < 100; i++) s.push(i);
    CHECK(s.size() == 100);
    CHECK(s.top() == 99);
}

// ─── Queue Tests ─────────────────────────────────────────────────────────────
TEST(queue_basic) {
    Queue<int> q;
    q.enqueue(1); q.enqueue(2); q.enqueue(3);
    CHECK(q.front() == 1);
    q.dequeue();
    CHECK(q.front() == 2);
    CHECK(q.size() == 2);
}

TEST(priority_queue_basic) {
    PriorityQueue<int> pq;
    pq.push(5); pq.push(1); pq.push(3); pq.push(2); pq.push(4);
    int prev = pq.top(); pq.pop();
    while (!pq.empty()) {
        int cur = pq.top(); pq.pop();
        CHECK(cur >= prev);
        prev = cur;
    }
}

// ─── DoublyLinkedList Tests ───────────────────────────────────────────────────
TEST(dll_push_pop) {
    DoublyLinkedList<int> dll;
    dll.push_back(1); dll.push_back(2); dll.push_back(3);
    CHECK(dll.size() == 3);
    CHECK(dll.front() == 1);
    CHECK(dll.back() == 3);
    dll.pop_front();
    CHECK(dll.front() == 2);
    dll.pop_back();
    CHECK(dll.back() == 2);
}

TEST(dll_move_to_front) {
    DoublyLinkedList<int> dll;
    dll.push_back(1); dll.push_back(2); dll.push_back(3);
    auto* tail = dll.back_node();
    dll.move_to_front(tail);
    CHECK(dll.front() == 3);
}

// ─── HashMap Tests ────────────────────────────────────────────────────────────
TEST(hashmap_insert_find) {
    HashMap<int, int> hm;
    hm.insert(1, 100); hm.insert(2, 200); hm.insert(3, 300);
    int v = 0;
    CHECK(hm.find(1, v) && v == 100);
    CHECK(hm.find(2, v) && v == 200);
    CHECK(!hm.find(99, v));
}

TEST(hashmap_erase) {
    HashMap<int, int> hm;
    hm.insert(1, 10); hm.insert(2, 20);
    CHECK(hm.erase(1));
    int v = 0;
    CHECK(!hm.find(1, v));
    CHECK(hm.find(2, v) && v == 20);
}

TEST(hashmap_operator_bracket) {
    HashMap<int, int> hm;
    hm[5] = 50; hm[6] = 60;
    CHECK(hm[5] == 50);
    CHECK(hm[6] == 60);
    CHECK(hm.size() == 2);
}

TEST(hashmap_string_key) {
    HashMap<const char*, int> hm;
    // Using int keys for string-like behavior
    HashMap<int,int> hm2;
    for (int i = 0; i < 200; i++) hm2.insert(i, i*2);
    int v = 0;
    CHECK(hm2.find(100, v) && v == 200);
    CHECK(hm2.size() == 200);
}

// ─── AVL Tree Tests ───────────────────────────────────────────────────────────
TEST(avl_insert_find) {
    AVLTree<int, int> tree;
    tree.insert(5, 50); tree.insert(3, 30); tree.insert(7, 70);
    tree.insert(1, 10); tree.insert(4, 40);
    int v = 0;
    CHECK(tree.find(5, v) && v == 50);
    CHECK(tree.find(1, v) && v == 10);
    CHECK(!tree.find(99, v));
    CHECK(tree.size() == 5);
}

TEST(avl_remove) {
    AVLTree<int, int> tree;
    for (int i = 1; i <= 10; i++) tree.insert(i, i*10);
    tree.remove(5);
    int v = 0;
    CHECK(!tree.find(5, v));
    CHECK(tree.size() == 9);
}

TEST(avl_inorder_sorted) {
    AVLTree<int, int> tree;
    int keys[] = {5, 2, 8, 1, 3, 7, 9};
    for (int k : keys) tree.insert(k, k);
    DynamicArray<int> ks, vs;
    tree.inorder(ks, vs);
    CHECK(ks.size() == 7);
    for (int i = 1; i < ks.size(); i++) CHECK(ks[i] > ks[i-1]);
}

TEST(avl_range_query) {
    AVLTree<int, int> tree;
    for (int i = 1; i <= 20; i++) tree.insert(i, i);
    DynamicArray<int> ks, vs;
    tree.rangeQuery(5, 10, ks, vs);
    CHECK(ks.size() == 6); // 5,6,7,8,9,10
    CHECK(ks[0] == 5);
    CHECK(ks[5] == 10);
}

// ─── DataValue Tests ─────────────────────────────────────────────────────────
TEST(datavalue_types) {
    DataValue i(42);
    DataValue d(3.14);
    DataValue s("hello");
    CHECK(i.type == DataType::INT && i.ival == 42);
    CHECK(d.type == DataType::DOUBLE);
    CHECK(s.type == DataType::STRING);
    CHECK(strcmp(s.sval, "hello") == 0);
}

TEST(datavalue_comparison) {
    DataValue a(10), b(20);
    CHECK(a < b); CHECK(b > a); CHECK(a != b);
    DataValue s1("apple"), s2("banana");
    CHECK(s1 < s2);
}

TEST(datavalue_tostring) {
    char buf[64];
    DataValue(42).toString(buf, 64);    CHECK(strcmp(buf, "42") == 0);
    DataValue(3.14).toString(buf, 64);  // "3.14" approx
    DataValue("hi").toString(buf, 64);  CHECK(strcmp(buf, "hi") == 0);
}

// ─── Parser Tests ─────────────────────────────────────────────────────────────
TEST(parser_select) {
    Parser p;
    ParsedQuery q = p.parse("SELECT * FROM customer WHERE c_custkey = 42");
    CHECK(q.type == QueryType::SELECT);
    CHECK(strcmp(q.tableName, "customer") == 0);
    CHECK(q.whereClause.valid);
    CHECK(q.whereClause.value.asInt() == 42);
}

TEST(parser_insert) {
    Parser p;
    ParsedQuery q = p.parse("INSERT INTO customer VALUES 999 TestName TestAddr 5 11-111-111-1111 500.00 BUILDING");
    CHECK(q.type == QueryType::INSERT);
    CHECK(strcmp(q.tableName, "customer") == 0);
    CHECK(q.values[0].asInt() == 999);
}

TEST(parser_update) {
    Parser p;
    ParsedQuery q = p.parse("UPDATE customer SET c_acctbal = 1234 WHERE c_custkey = 5");
    CHECK(q.type == QueryType::UPDATE);
    CHECK(strcmp(q.setCol, "c_acctbal") == 0);
    CHECK(q.setValue.asInt() == 1234);
}

TEST(parser_join) {
    Parser p;
    ParsedQuery q = p.parse("SELECT * FROM customer JOIN orders ON customer.c_custkey = orders.o_custkey");
    CHECK(q.type == QueryType::JOIN_SELECT);
    CHECK(q.joinSpec.valid);
    CHECK(strcmp(q.joinSpec.col1, "c_custkey") == 0);
    CHECK(strcmp(q.joinSpec.col2, "o_custkey") == 0);
}

TEST(parser_infix_postfix) {
    char postfix[256];
    Parser::infixToPostfix("3 + 4 * 2", postfix, sizeof(postfix));
    // Should contain 3, 4, 2, *, +
    CHECK(strchr(postfix, '+') != nullptr);
    CHECK(strchr(postfix, '*') != nullptr);
}

// ─── Graph Tests ──────────────────────────────────────────────────────────────
TEST(graph_kruskal) {
    Graph g(4);
    g.addEdge(0,1,1); g.addEdge(0,2,4); g.addEdge(1,2,2); g.addEdge(1,3,5); g.addEdge(2,3,1);
    DynamicArray<Edge> mst = g.kruskalMST();
    CHECK(mst.size() == 3); // n-1 edges
    double total = 0;
    for (int i = 0; i < mst.size(); i++) total += mst[i].weight;
    CHECK(total < 10.0); // MST should be minimal
}

// ─── Main ────────────────────────────────────────────────────────────────────
int main() {
    printf("╔══════════════════════════════════════╗\n");
    printf("║        NanoDB Test Runner            ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    printf("[DynamicArray]\n");
    RUN(dynamic_array_push_pop);
    RUN(dynamic_array_resize);
    RUN(dynamic_array_remove_at);
    RUN(dynamic_array_find);

    printf("[Stack]\n");
    RUN(stack_basic);
    RUN(stack_resize);

    printf("[Queue]\n");
    RUN(queue_basic);
    RUN(priority_queue_basic);

    printf("[DoublyLinkedList]\n");
    RUN(dll_push_pop);
    RUN(dll_move_to_front);

    printf("[HashMap]\n");
    RUN(hashmap_insert_find);
    RUN(hashmap_erase);
    RUN(hashmap_operator_bracket);
    RUN(hashmap_string_key);

    printf("[AVLTree]\n");
    RUN(avl_insert_find);
    RUN(avl_remove);
    RUN(avl_inorder_sorted);
    RUN(avl_range_query);

    printf("[DataValue]\n");
    RUN(datavalue_types);
    RUN(datavalue_comparison);
    RUN(datavalue_tostring);

    printf("[Parser]\n");
    RUN(parser_select);
    RUN(parser_insert);
    RUN(parser_update);
    RUN(parser_join);
    RUN(parser_infix_postfix);

    printf("[Graph]\n");
    RUN(graph_kruskal);

    printf("\n══════════════════════════════════════\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    printf("══════════════════════════════════════\n");

    return failed > 0 ? 1 : 0;
}
