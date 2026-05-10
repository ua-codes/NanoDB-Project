#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>

#include "Logger.h"
#include "FileManager.h"
#include "DataValue.h"
#include "DynamicArray.h"
#include "Parser.h"
#include "QueryExecutor.h"
#include "BufferPool.h"
#include "Graph.h"
#include "Stack.h"
#include "Queue.h"

// ─── Pseudo-random helpers ─────────────────────────────────────────────────
static unsigned int rng_state = 42;
static unsigned int rng() {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}
static int rng_range(int lo, int hi) { return lo + (int)(rng() % (unsigned)(hi - lo + 1)); }
static double rng_double(double lo, double hi) {
    return lo + ((double)(rng() % 100000) / 100000.0) * (hi - lo);
}

// ─── Schema builders ────────────────────────────────────────────────────────
static Schema buildCustomerSchema() {
    Schema s;
    strncpy(s.tableName, "customer", 63);
    s.cols[0] = ColDef("c_custkey",    DataType::INT);
    s.cols[1] = ColDef("c_name",       DataType::STRING);
    s.cols[2] = ColDef("c_address",    DataType::STRING);
    s.cols[3] = ColDef("c_nationkey",  DataType::INT);
    s.cols[4] = ColDef("c_phone",      DataType::STRING);
    s.cols[5] = ColDef("c_acctbal",    DataType::DOUBLE);
    s.cols[6] = ColDef("c_mktsegment", DataType::STRING);
    s.numCols = 7;
    return s;
}

static Schema buildOrdersSchema() {
    Schema s;
    strncpy(s.tableName, "orders", 63);
    s.cols[0] = ColDef("o_orderkey",    DataType::INT);
    s.cols[1] = ColDef("o_custkey",     DataType::INT);
    s.cols[2] = ColDef("o_orderstatus", DataType::STRING);
    s.cols[3] = ColDef("o_totalprice",  DataType::DOUBLE);
    s.cols[4] = ColDef("o_orderdate",   DataType::STRING);
    s.cols[5] = ColDef("o_orderpriority", DataType::STRING);
    s.numCols = 6;
    return s;
}

static Schema buildLineitemSchema() {
    Schema s;
    strncpy(s.tableName, "lineitem", 63);
    s.cols[0] = ColDef("l_orderkey",    DataType::INT);
    s.cols[1] = ColDef("l_partkey",     DataType::INT);
    s.cols[2] = ColDef("l_suppkey",     DataType::INT);
    s.cols[3] = ColDef("l_linenumber",  DataType::INT);
    s.cols[4] = ColDef("l_quantity",    DataType::DOUBLE);
    s.cols[5] = ColDef("l_extendedprice", DataType::DOUBLE);
    s.cols[6] = ColDef("l_discount",    DataType::DOUBLE);
    s.cols[7] = ColDef("l_tax",         DataType::DOUBLE);
    s.cols[8] = ColDef("l_returnflag",  DataType::STRING);
    s.cols[9] = ColDef("l_shipdate",    DataType::STRING);
    s.numCols = 10;
    return s;
}

// ─── Data generators ───────────────────────────────────────────────────────
static const char* segments[] = {"AUTOMOBILE","BUILDING","FURNITURE","HOUSEHOLD","MACHINERY"};
static const char* statuses[]  = {"O","F","P"};
static const char* priorities[] = {"1-URGENT","2-HIGH","3-MEDIUM","4-NOT SPECIFIED","5-LOW"};
static const char* flags[]     = {"R","A","N"};

static void generateCustomers(DynamicArray<Row>& rows, int count) {
    for (int i = 1; i <= count; i++) {
        Row r; r.numCols = 7;
        r.cols[0] = DataValue(i);
        char name[64]; snprintf(name, 64, "Customer#%09d", i);
        r.cols[1] = DataValue(name);
        char addr[64]; snprintf(addr, 64, "Addr%d", rng_range(1000, 9999));
        r.cols[2] = DataValue(addr);
        r.cols[3] = DataValue(rng_range(0, 24));
        char phone[20]; snprintf(phone, 20, "%02d-%03d-%03d-%04d",
            rng_range(10,99), rng_range(100,999), rng_range(100,999), rng_range(1000,9999));
        r.cols[4] = DataValue(phone);
        r.cols[5] = DataValue(rng_double(-999.99, 9999.99));
        r.cols[6] = DataValue(segments[rng_range(0, 4)]);
        rows.push_back(r);
    }
}

static void generateOrders(DynamicArray<Row>& rows, int count, int numCustomers) {
    for (int i = 1; i <= count; i++) {
        Row r; r.numCols = 6;
        r.cols[0] = DataValue(i);
        r.cols[1] = DataValue(rng_range(1, numCustomers));
        r.cols[2] = DataValue(statuses[rng_range(0,2)]);
        r.cols[3] = DataValue(rng_double(1000.0, 500000.0));
        char date[16]; int y = rng_range(1992,1998), m = rng_range(1,12), d = rng_range(1,28);
        snprintf(date, 16, "%04d-%02d-%02d", y, m, d);
        r.cols[4] = DataValue(date);
        r.cols[5] = DataValue(priorities[rng_range(0,4)]);
        rows.push_back(r);
    }
}

static void generateLineitems(DynamicArray<Row>& rows, int count, int numOrders) {
    for (int i = 0; i < count; i++) {
        Row r; r.numCols = 10;
        r.cols[0] = DataValue(rng_range(1, numOrders));
        r.cols[1] = DataValue(rng_range(1, 200000));
        r.cols[2] = DataValue(rng_range(1, 10000));
        r.cols[3] = DataValue(rng_range(1, 7));
        r.cols[4] = DataValue(rng_double(1.0, 50.0));
        double qty = rng_double(1.0, 50.0);
        double price = qty * rng_double(10.0, 900.0);
        r.cols[5] = DataValue(price);
        r.cols[6] = DataValue(rng_double(0.0, 0.10));
        r.cols[7] = DataValue(rng_double(0.0, 0.08));
        r.cols[8] = DataValue(flags[rng_range(0,2)]);
        char date[16]; int y = rng_range(1992,1998), m = rng_range(1,12), d = rng_range(1,28);
        snprintf(date, 16, "%04d-%02d-%02d", y, m, d);
        r.cols[9] = DataValue(date);
        rows.push_back(r);
    }
}

// ─── Benchmark helpers ──────────────────────────────────────────────────────
static double getTimeMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

// ─── Demo: Graph / Kruskal ──────────────────────────────────────────────────
static void demoGraph() {
    printf("=== Kruskal MST Demo ===\n");
    Graph g(6);
    g.addEdge(0,1,4); g.addEdge(0,2,3); g.addEdge(1,2,1);
    g.addEdge(1,3,2); g.addEdge(2,3,4); g.addEdge(3,4,2);
    g.addEdge(4,5,6);
    DynamicArray<Edge> mst = g.kruskalMST();
    double total = 0;
    for (int i = 0; i < mst.size(); i++) {
        printf("  Edge %d-%d weight=%.1f\n", mst[i].u, mst[i].v, mst[i].weight);
        total += mst[i].weight;
    }
    printf("  Total MST weight: %.1f\n\n", total);
    gLogger.info("Kruskal MST: %d edges, total weight %.1f", mst.size(), total);
}

// ─── Demo: Infix to Postfix ─────────────────────────────────────────────────
static void demoInfixPostfix() {
    printf("=== Infix to Postfix Demo ===\n");
    const char* exprs[] = {
        "3 + 4 * 2",
        "(3 + 4) * 2",
        "5 + (6 * 7) - 8 / 4"
    };
    char postfix[256];
    for (int i = 0; i < 3; i++) {
        Parser::infixToPostfix(exprs[i], postfix, sizeof(postfix));
        printf("  Infix:   %s\n  Postfix: %s\n\n", exprs[i], postfix);
    }
    gLogger.info("Infix-to-postfix demo complete");
}

// ─── Demo: Stack & Queue ────────────────────────────────────────────────────
static void demoStackQueue() {
    printf("=== Stack & Queue Demo ===\n");
    Stack<int> st;
    for (int i = 1; i <= 5; i++) st.push(i * 10);
    printf("  Stack pop: ");
    while (!st.empty()) { printf("%d ", st.top()); st.pop(); }
    printf("\n");

    Queue<int> q;
    for (int i = 1; i <= 5; i++) q.enqueue(i * 10);
    printf("  Queue dequeue: ");
    while (!q.empty()) { printf("%d ", q.front()); q.dequeue(); }
    printf("\n\n");

    PriorityQueue<int> pq;
    int vals[] = {5, 3, 8, 1, 9, 2};
    for (int v : vals) pq.push(v);
    printf("  PriorityQueue (min-heap): ");
    while (!pq.empty()) { printf("%d ", pq.top()); pq.pop(); }
    printf("\n\n");
}

// ─── Write queries.txt ───────────────────────────────────────────────────────
static void writeQueriesFile() {
    FILE* f = fopen("queries.txt", "w");
    if (!f) return;
    fprintf(f, "# NanoDB Query File - 50 Sample Queries\n\n");

    // SELECTs
    for (int i = 1; i <= 10; i++)
        fprintf(f, "SELECT * FROM customer WHERE c_custkey = %d\n", i * 1000);
    for (int i = 1; i <= 5; i++)
        fprintf(f, "SELECT * FROM customer WHERE c_nationkey = %d\n", i);
    for (int i = 1; i <= 5; i++)
        fprintf(f, "SELECT * FROM orders WHERE o_custkey = %d\n", i * 500);
    for (int i = 1; i <= 5; i++)
        fprintf(f, "SELECT * FROM lineitem WHERE l_orderkey = %d\n", i * 100);
    // JOINs
    for (int i = 1; i <= 5; i++)
        fprintf(f, "SELECT * FROM customer JOIN orders ON customer.c_custkey = orders.o_custkey\n");
    for (int i = 1; i <= 5; i++)
        fprintf(f, "SELECT * FROM orders JOIN lineitem ON orders.o_orderkey = lineitem.l_orderkey\n");
    // UPDATEs
    for (int i = 1; i <= 5; i++)
        fprintf(f, "UPDATE customer SET c_acctbal = 1000.00 WHERE c_custkey = %d\n", i * 2000);
    for (int i = 1; i <= 5; i++)
        fprintf(f, "UPDATE orders SET o_orderstatus = F WHERE o_orderkey = %d\n", i * 1000);
    // INSERTs
    for (int i = 1; i <= 5; i++)
        fprintf(f, "INSERT INTO customer VALUES %d Customer#NEW%d NewAddr%d %d 11-111-111-1111 500.00 BUILDING\n",
            90000+i, i, i, i%25);

    fclose(f);
    printf("queries.txt written (50 queries)\n");
}

// ─── Write benchmark CSV ─────────────────────────────────────────────────────
static void writeBenchmarkCSV(const char* path,
    double genTime, double loadTime, double selectTime,
    double joinTime, double updateTime, double indexBuildTime,
    int selectCount, int joinCount, int updateCount,
    double hitRate)
{
    FILE* f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "operation,time_ms,rows_affected,notes\n");
    fprintf(f, "data_generation,%.2f,100000,20k cust + 30k orders + 50k lineitem\n", genTime);
    fprintf(f, "data_load,%.2f,100000,load all tables from disk\n", loadTime);
    fprintf(f, "index_build,%.2f,%d,AVL index on c_custkey\n", indexBuildTime, selectCount);
    fprintf(f, "select_queries,%.2f,%d,20 SELECT queries\n", selectTime, selectCount);
    fprintf(f, "join_queries,%.2f,%d,5 JOIN queries\n", joinTime, joinCount);
    fprintf(f, "update_queries,%.2f,%d,5 UPDATE queries\n", updateTime, updateCount);
    fprintf(f, "buffer_hit_rate,%.2f,0,%.1f%%\n", 0.0, hitRate);
    fclose(f);
    printf("benchmark_results.csv written\n");
}

// ─── Write memory profile ────────────────────────────────────────────────────
static void writeMemoryProfile(const char* path, int custRows, int ordRows, int liRows) {
    FILE* f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "NanoDB Memory Profile\n");
    fprintf(f, "=====================\n");
    fprintf(f, "Row size (bytes):      %zu\n", sizeof(Row));
    fprintf(f, "DataValue size:        %zu\n", sizeof(DataValue));
    fprintf(f, "Schema size:           %zu\n", sizeof(Schema));
    size_t custMem = custRows * sizeof(Row);
    size_t ordMem  = ordRows  * sizeof(Row);
    size_t liMem   = liRows   * sizeof(Row);
    fprintf(f, "\ncustomer rows:         %d  (~%zu KB)\n", custRows, custMem/1024);
    fprintf(f, "orders rows:           %d  (~%zu KB)\n",  ordRows,  ordMem/1024);
    fprintf(f, "lineitem rows:         %d (~%zu KB)\n",   liRows,   liMem/1024);
    fprintf(f, "Total data memory:     ~%zu KB\n", (custMem+ordMem+liMem)/1024);
    fprintf(f, "\nBuffer pool frames:    64\n");
    fprintf(f, "Buffer pool memory:    ~%d KB\n", (int)(64*PAGE_SIZE/1024));
    fprintf(f, "\nAVL index (c_custkey): ~%d nodes\n", custRows);
    fclose(f);
    printf("memory_profile.txt written\n");
}

// ─── Main ────────────────────────────────────────────────────────────────────
int main() {
    // Setup directories
    system("mkdir -p data logs");

    // Setup logger
    gLogger.open("logs/nanodb_execution.log", LogLevel::DEBUG, false);
    gLogger.info("NanoDB starting up");
    printf("╔══════════════════════════════════════╗\n");
    printf("║          NanoDB Engine v1.0          ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    gFileManager.setDataDir("data");

    // ── Build schemas ──────────────────────────────────────────────────────
    Schema custSchema  = buildCustomerSchema();
    Schema ordSchema   = buildOrdersSchema();
    Schema liSchema    = buildLineitemSchema();

    gFileManager.saveSchema(custSchema);
    gFileManager.saveSchema(ordSchema);
    gFileManager.saveSchema(liSchema);

    // ── Generate data ──────────────────────────────────────────────────────
    printf("=== Generating TPC-H Style Dataset ===\n");
    double t0 = getTimeMs();

    const int NUM_CUST = 20000;
    const int NUM_ORD  = 30000;
    const int NUM_LI   = 50000;

    DynamicArray<Row> custRows(NUM_CUST + 10);
    DynamicArray<Row> ordRows(NUM_ORD + 10);
    DynamicArray<Row> liRows(NUM_LI + 10);

    printf("  Generating %d customers...\n", NUM_CUST);
    generateCustomers(custRows, NUM_CUST);

    printf("  Generating %d orders...\n", NUM_ORD);
    generateOrders(ordRows, NUM_ORD, NUM_CUST);

    printf("  Generating %d line items...\n", NUM_LI);
    generateLineitems(liRows, NUM_LI, NUM_ORD);

    double genTime = getTimeMs() - t0;
    printf("  Generation done in %.2f ms\n\n", genTime);
    gLogger.info("Data generation: %d cust, %d orders, %d lineitems in %.2f ms",
        NUM_CUST, NUM_ORD, NUM_LI, genTime);

    // ── Save to disk ──────────────────────────────────────────────────────
    printf("=== Saving to Disk ===\n");
    t0 = getTimeMs();
    gFileManager.saveRows("customer", custRows);
    gFileManager.saveRows("orders",   ordRows);
    gFileManager.saveRows("lineitem", liRows);
    printf("  Saved in %.2f ms\n\n", getTimeMs() - t0);

    // ── Setup executor ─────────────────────────────────────────────────────
    QueryExecutor executor;
    executor.registerTable(custSchema);
    executor.registerTable(ordSchema);
    executor.registerTable(liSchema);

    // ── Load tables ────────────────────────────────────────────────────────
    printf("=== Loading Tables ===\n");
    t0 = getTimeMs();
    executor.loadTable("customer");
    executor.loadTable("orders");
    executor.loadTable("lineitem");
    double loadTime = getTimeMs() - t0;
    printf("  Load done in %.2f ms\n\n", loadTime);

    // ── Index build time ──────────────────────────────────────────────────
    double indexBuildTime = 0; // already built during loadTable("customer")
    printf("  AVL index on c_custkey: %d entries\n\n",
        executor.getIndexManager().size());

    // ── Structural demos ───────────────────────────────────────────────────
    demoStackQueue();
    demoGraph();
    demoInfixPostfix();

    // ── Run SELECT queries ─────────────────────────────────────────────────
    printf("=== SELECT Queries ===\n");
    Parser parser;
    t0 = getTimeMs();
    int totalSelectRows = 0;

    char q[256];
    // Exact lookups by custkey (uses AVL index)
    int custKeys[] = {1000, 5000, 10000, 15000, 20000};
    for (int k : custKeys) {
        snprintf(q, sizeof(q), "SELECT * FROM customer WHERE c_custkey = %d", k);
        gLogger.info("QUERY: %s", q);
        ParsedQuery pq = parser.parse(q);
        totalSelectRows += executor.execute(pq, 5);
    }

    // Filter by nationkey
    snprintf(q, sizeof(q), "SELECT * FROM customer WHERE c_nationkey = 3");
    gLogger.info("QUERY: %s", q);
    totalSelectRows += executor.execute(parser.parse(q), 5);

    // Orders filter
    snprintf(q, sizeof(q), "SELECT * FROM orders WHERE o_orderstatus = O");
    gLogger.info("QUERY: %s", q);
    totalSelectRows += executor.execute(parser.parse(q), 5);

    // Lineitem
    snprintf(q, sizeof(q), "SELECT * FROM lineitem WHERE l_linenumber = 1");
    gLogger.info("QUERY: %s", q);
    totalSelectRows += executor.execute(parser.parse(q), 5);

    double selectTime = getTimeMs() - t0;
    printf("SELECT queries done in %.2f ms (%d total rows)\n\n", selectTime, totalSelectRows);

    // ── Run JOIN queries ───────────────────────────────────────────────────
    printf("=== JOIN Queries ===\n");
    t0 = getTimeMs();
    int totalJoinRows = 0;

    snprintf(q, sizeof(q),
        "SELECT * FROM customer JOIN orders ON customer.c_custkey = orders.o_custkey");
    gLogger.info("QUERY: %s", q);
    totalJoinRows += executor.execute(parser.parse(q), 10);

    snprintf(q, sizeof(q),
        "SELECT * FROM orders JOIN lineitem ON orders.o_orderkey = lineitem.l_orderkey");
    gLogger.info("QUERY: %s", q);
    totalJoinRows += executor.execute(parser.parse(q), 10);

    double joinTime = getTimeMs() - t0;
    printf("JOIN queries done in %.2f ms (%d total rows)\n\n", joinTime, totalJoinRows);

    // ── Run UPDATE queries ─────────────────────────────────────────────────
    printf("=== UPDATE Queries ===\n");
    t0 = getTimeMs();
    int totalUpdateRows = 0;

    snprintf(q, sizeof(q), "UPDATE customer SET c_acctbal = 9999.99 WHERE c_custkey = 1000");
    gLogger.info("QUERY: %s", q);
    totalUpdateRows += executor.execute(parser.parse(q));

    snprintf(q, sizeof(q), "UPDATE orders SET o_orderstatus = F WHERE o_custkey = 5000");
    gLogger.info("QUERY: %s", q);
    totalUpdateRows += executor.execute(parser.parse(q));

    snprintf(q, sizeof(q), "UPDATE lineitem SET l_discount = 0.05 WHERE l_linenumber = 1");
    gLogger.info("QUERY: %s", q);
    totalUpdateRows += executor.execute(parser.parse(q));

    double updateTime = getTimeMs() - t0;
    printf("UPDATE queries done in %.2f ms (%d rows affected)\n\n", updateTime, totalUpdateRows);

    // ── Run INSERT queries ──────────────────────────────────────────────────
    printf("=== INSERT Queries ===\n");
    for (int i = 1; i <= 3; i++) {
        snprintf(q, sizeof(q),
            "INSERT INTO customer VALUES %d Customer#INSERT%d InsertAddr %d 99-999-999-9999 0.00 BUILDING",
            90000+i, i, i%25);
        gLogger.info("QUERY: %s", q);
        executor.execute(parser.parse(q));
    }

    // ── Buffer pool stats ───────────────────────────────────────────────────
    BufferPool& bp = executor.getBufferPool();
    printf("=== Buffer Pool Stats ===\n");
    printf("  Pool size: %d pages\n", bp.getPoolSize());
    printf("  Hits: %d  Misses: %d  Hit rate: %.1f%%\n\n",
        bp.getHits(), bp.getMisses(), bp.hitRate());
    gLogger.info("Buffer pool: hits=%d misses=%d rate=%.1f%%",
        bp.getHits(), bp.getMisses(), bp.hitRate());

    // ── Write output files ─────────────────────────────────────────────────
    printf("=== Writing Output Files ===\n");
    writeQueriesFile();
    writeBenchmarkCSV("benchmark_results.csv",
        genTime, loadTime, selectTime, joinTime, updateTime, indexBuildTime,
        totalSelectRows, totalJoinRows, totalUpdateRows, bp.hitRate());
    writeMemoryProfile("memory_profile.txt", NUM_CUST, NUM_ORD, NUM_LI);

    // Copy log note
    gLogger.info("NanoDB shutdown complete");
    printf("\n=== NanoDB Execution Complete ===\n");
    printf("  Log:       logs/nanodb_execution.log\n");
    printf("  Benchmark: benchmark_results.csv\n");
    printf("  Memory:    memory_profile.txt\n");
    printf("  Queries:   queries.txt\n");

    return 0;
}
