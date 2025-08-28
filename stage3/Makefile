CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wshadow -Wpedantic -O0 -g
STAGE1_DIR := ../stage1
STAGE2_DIR := ../stage2

BIN := euler_cli
SRC := main.cpp $(STAGE1_DIR)/graph.cpp $(STAGE2_DIR)/euler.cpp
INCLUDES := -I. -I$(STAGE1_DIR) -I$(STAGE2_DIR)

.PHONY: all run clean asan
all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN) -n 8 -m 12 -s 42

asan:
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined $(INCLUDES) $(SRC) -o $(BIN)

clean:
	$(RM) $(BIN)
