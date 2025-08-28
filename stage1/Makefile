CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wshadow -Wpedantic -O0 -g
BIN := graph_demo
SRC := main.cpp graph.cpp

.PHONY: all run clean asan
all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN)

asan:  # optional: address/undefined sanitizers
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined $(SRC) -o $(BIN)

clean:
	$(RM) $(BIN)
