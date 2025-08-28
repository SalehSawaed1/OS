SHELL := /bin/bash
CXX := g++
# Coverage build: -O0 + --coverage
CXXFLAGS := -std=c++20 -Wall -Wextra -Wshadow -Wpedantic -O0 -g --coverage
LDFLAGS := --coverage -pthread

# Reuse sources from earlier stages (no duplication)
STAGE1 := ../stage1
STAGE2 := ../stage2
STAGE7 := ../stage7
STAGE8 := ../stage8
STAGE9 := ../stage9

# Binaries we’ll build with coverage
BIN_ALGO_TESTS := cov_algo_tests
BIN_EULER_TEST := cov_euler_test
BIN_LF_SERVER  := cov_server8
BIN_PIPE_SERVER:= cov_server9
BIN_CLIENT     := cov_client7 # client doesn't need coverage, but okay

PORT_LF   := 5578
PORT_PIPE := 5579

.PHONY: all clean deps run_servers run_tests coverage html open

all: coverage

deps:
	@echo "(re)building nothing; stage11 compiles its own coverage binaries"

# ---- Coverage builds ----
$(BIN_ALGO_TESTS): algo_tests.cpp $(STAGE1)/graph.cpp $(STAGE7)/algorithms.cpp
	$(CXX) $(CXXFLAGS) -I$(STAGE1) -I$(STAGE7) $^ -o $@ $(LDFLAGS)

$(BIN_EULER_TEST): euler_tests.cpp $(STAGE1)/graph.cpp $(STAGE2)/euler.cpp
	$(CXX) $(CXXFLAGS) -I$(STAGE1) -I$(STAGE2) $^ -o $@ $(LDFLAGS)

$(BIN_LF_SERVER): $(STAGE8)/server8.cpp $(STAGE1)/graph.cpp $(STAGE7)/algorithms.cpp
	$(CXX) $(CXXFLAGS) -I$(STAGE1) -I$(STAGE7) $^ -o $@ $(LDFLAGS)

$(BIN_PIPE_SERVER): $(STAGE9)/server9.cpp $(STAGE1)/graph.cpp $(STAGE7)/algorithms.cpp
	$(CXX) $(CXXFLAGS) -I$(STAGE1) -I$(STAGE7) $^ -o $@ $(LDFLAGS)

$(BIN_CLIENT): $(STAGE7)/client7.cpp
	$(CXX) -std=c++20 -O2 -g -I$(STAGE1) -I$(STAGE7) $^ -o $@ -pthread

# ---- Workloads ----
run_tests: $(BIN_ALGO_TESTS) $(BIN_EULER_TEST)
	./$(BIN_ALGO_TESTS)
	./$(BIN_EULER_TEST)

run_servers: $(BIN_LF_SERVER) $(BIN_PIPE_SERVER) $(BIN_CLIENT)
	@echo "[LF server under coverage]"
	@set -euo pipefail; \
	( timeout 8s ./$(BIN_LF_SERVER) -p $(PORT_LF) -t 4 & srv=$$!; \
	  sleep 0.6; \
	  bash ./workload.sh ./$(BIN_CLIENT) $(PORT_LF); \
	  wait $$srv || true )
	@echo "[Pipeline server under coverage]"
	@set -euo pipefail; \
	( timeout 8s ./$(BIN_PIPE_SERVER) -p $(PORT_PIPE) & srv=$$!; \
	  sleep 0.6; \
	  bash ./workload.sh ./$(BIN_CLIENT) $(PORT_PIPE); \
	  wait $$srv || true )

coverage: run_tests run_servers
	mkdir -p coverage
	# Generate HTML covering stage1/2/7/8/9 (we compiled those here with coverage)
	gcovr -r .. --filter '../stage1/.*' --filter '../stage2/.*' --filter '../stage7/.*' \
	      --filter '../stage8/.*' --filter '../stage9/.*' \
	      --html --html-details -o coverage/index.html || \
	gcovr -r .. --html --html-details -o coverage/index.html
	@echo "Coverage report -> stage11/coverage/index.html"

open:
	@echo "Open in VS Code: stage11/coverage/index.html"

clean:
	$(RM) $(BIN_ALGO_TESTS) $(BIN_EULER_TEST) $(BIN_LF_SERVER) $(BIN_PIPE_SERVER) $(BIN_CLIENT)
	$(RM) -r coverage *.gcda *.gcno *.gcov
