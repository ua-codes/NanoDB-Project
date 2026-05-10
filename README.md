# NanoDB - A Minimal Database Engine in C++

NanoDB is a from-scratch database engine built entirely in C++ using **no STL containers** — only raw pointers, `new`/`delete`, fixed arrays, and linked lists.

## Features

| Component | Implementation |
|---|---|
| Dynamic Array | Raw pointer + resize |
| Stack | Array-based |
| Queue | Circular array |
| Priority Queue | Min-heap |
| Doubly Linked List | Pointer-based nodes |
| Hash Map | Separate chaining |
| AVL Tree | Self-balancing BST |
| Graph + MST | Kruskal's algorithm |
| Buffer Pool | LRU eviction via DLL |
| Binary File I/O | `fread`/`fwrite` |
| Query Parser | Tokenizer + recursive |
| Infix→Postfix | Shunting-yard |
| Query Executor | SELECT/INSERT/UPDATE/JOIN |
| AVL Index | On `c_custkey` |
| Logger | File + console |
| Test Runner | Custom assertion framework |

## Project Structure

```
NanoDB/
├── include/          # All headers (templates + classes)
├── src/              # .cpp source files
├── tests/            # Test runner
├── scripts/          # Python data preview generator
├── data/             # Binary .ndb files (generated at runtime)
├── logs/             # Execution logs
└── docs/             # Documentation
```

## Build & Run

```bash
# Build
make

# Run the database engine
./nanodb

# Run tests
make test

# Generate CSV previews
python3 scripts/generate_data.py
```

## Query Support

```sql
-- SELECT with WHERE (uses AVL index on c_custkey)
SELECT * FROM customer WHERE c_custkey = 1000

-- JOIN (hash join implementation)
SELECT * FROM customer JOIN orders ON customer.c_custkey = orders.o_custkey

-- UPDATE
UPDATE customer SET c_acctbal = 9999.99 WHERE c_custkey = 500

-- INSERT
INSERT INTO customer VALUES 99999 NewCust Addr1 5 11-111-222-3333 100.00 BUILDING
```

## Dataset

Generated at runtime (TPC-H style):
- **customer**: 20,000 rows
- **orders**: 30,000 rows
- **lineitem**: 50,000 rows

## Output Files

| File | Description |
|---|---|
| `logs/nanodb_execution.log` | Full execution log |
| `benchmark_results.csv` | Query timing benchmarks |
| `memory_profile.txt` | Memory usage analysis |
| `queries.txt` | 50 sample queries |

## Constraints

- **Zero STL containers**: no `std::vector`, `std::map`, `std::queue`, `std::set`, `std::list`
- All data structures built from scratch using raw pointers
- Fixed-size arrays where appropriate
- Manual memory management throughout
