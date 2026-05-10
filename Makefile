CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
LDFLAGS  =

SRC_DIR  = src
OBJ_DIR  = build
BIN_DIR  = .

SRCS     = src/Logger.cpp src/FileManager.cpp src/QueryExecutor.cpp src/main.cpp
OBJS     = $(patsubst src/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
TARGET   = nanodb

TEST_SRC = tests/test_runner.cpp
TEST_OBJ = $(OBJ_DIR)/test_runner.o
TEST_BIN = nanodb_tests

.PHONY: all clean test dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJ_DIR) data logs

$(OBJ_DIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built: $(TARGET)"

$(OBJ_DIR)/test_runner.o: tests/test_runner.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: dirs $(OBJ_DIR)/Logger.o $(OBJ_DIR)/FileManager.o $(OBJ_DIR)/QueryExecutor.o $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(TEST_BIN) $(OBJ_DIR)/Logger.o $(OBJ_DIR)/FileManager.o $(OBJ_DIR)/QueryExecutor.o $(TEST_OBJ) $(LDFLAGS)
	@echo "Running tests..."
	./$(TEST_BIN)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST_BIN) data/*.ndb data/*.schema logs/*.log \
	       benchmark_results.csv memory_profile.txt queries.txt

run: all
	./$(TARGET)
