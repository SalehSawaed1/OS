CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wshadow -Wpedantic -O0 -g

STAGE1_DIR := ../stage1
STAGE2_DIR := ../stage2
STAGE3_DIR := ../stage3

BIN := euler_reports
SRC := $(STAGE3_DIR)/main.cpp $(STAGE2_DIR)/euler.cpp $(STAGE1_DIR)/graph.cpp
INCLUDES := -I$(STAGE3_DIR) -I$(STAGE2_DIR) -I$(STAGE1_DIR)

# Default workloads (override on the command line)
ARGS           ?= -n 20 -m 40 -s 1
ARGS_MEMCHECK  ?= -n 50 -m 120 -s 1
ARGS_CALLGRIND ?= -n 2000 -m 4000 -s 1
ARGS_GPROF     ?= -n 50000 -m 120000 -s 1
ARGS_COV       ?= -n 20 -m 40 -s 1

.PHONY: all run clean memcheck callgrind coverage gprof asan ubsan reports

all: $(BIN)
$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN) $(ARGS)

memcheck: $(BIN)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(BIN) $(ARGS_MEMCHECK)

callgrind: $(BIN)
	valgrind --tool=callgrind ./$(BIN) $(ARGS_CALLGRIND)
	@echo "Open with: kcachegrind callgrind.out.*"

coverage: clean
	$(CXX) $(CXXFLAGS) --coverage $(INCLUDES) $(SRC) -o $(BIN) --coverage
	./$(BIN) $(ARGS_COV)
	mkdir -p coverage
	gcovr -r .. --filter '../.*' --html --html-details -o coverage/index.html || \
	gcovr -r .. --html --html-details -o coverage/index.html
	@echo "Coverage report at stage4/coverage/index.html"

gprof: clean
	$(CXX) $(CXXFLAGS) -pg $(INCLUDES) $(SRC) -o $(BIN) -pg
	./$(BIN) $(ARGS_GPROF)
	gprof $(BIN) gmon.out > gprof.txt
	@echo "gprof.txt written"

asan:  # quick sanitizer run
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined $(INCLUDES) $(SRC) -o $(BIN)
	./$(BIN) $(ARGS)

ubsan: # just UBSan
	$(CXX) $(CXXFLAGS) -fsanitize=undefined $(INCLUDES) $(SRC) -o $(BIN)
	./$(BIN) $(ARGS)

reports: all memcheck callgrind coverage gprof

clean:
	$(RM) $(BIN) gmon.out gprof.txt
	$(RM) -r coverage callgrind.out.*
