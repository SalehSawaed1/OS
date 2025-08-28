CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wshadow -Wpedantic -O0 -g
STAGE1_DIR := ../stage1

BIN := euler_stage2
SRC := main.cpp euler.cpp $(STAGE1_DIR)/graph.cpp
INCLUDES := -I. -I$(STAGE1_DIR)

.PHONY: all run clean asan
all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN)

asan:
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined $(INCLUDES) $(SRC) -o $(BIN)

clean:
	$(RM) $(BIN)
