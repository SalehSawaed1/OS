CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wshadow -Wpedantic -O2 -g

STAGE1_DIR := ../stage1

BIN_SERVER := server7
BIN_CLIENT := client7

SRC_SERVER := server7.cpp algorithms.cpp $(STAGE1_DIR)/graph.cpp
SRC_CLIENT := client7.cpp

INCLUDES := -I. -I$(STAGE1_DIR)

.PHONY: all clean run

all: $(BIN_SERVER) $(BIN_CLIENT)

$(BIN_SERVER): $(SRC_SERVER)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC_SERVER) -o $@

$(BIN_CLIENT): $(SRC_CLIENT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC_CLIENT) -o $@

run: $(BIN_SERVER)
	./$(BIN_SERVER) -p 5557

clean:
	$(RM) $(BIN_SERVER) $(BIN_CLIENT)
