CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wshadow -Wpedantic -O2 -g
LDFLAGS := -pthread

STAGE1_DIR := ../stage1
STAGE7_DIR := ../stage7

BIN_SERVER := server9
BIN_CLIENT := client7   # reuse Stage 7 client

SRC_SERVER := server9.cpp active.hpp $(STAGE1_DIR)/graph.cpp $(STAGE7_DIR)/algorithms.cpp
SRC_CLIENT := $(STAGE7_DIR)/client7.cpp

INCLUDES := -I. -I$(STAGE1_DIR) -I$(STAGE7_DIR)

.PHONY: all clean run

all: $(BIN_SERVER) $(BIN_CLIENT)

$(BIN_SERVER): $(SRC_SERVER)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(STAGE1_DIR)/graph.cpp $(STAGE7_DIR)/algorithms.cpp server9.cpp -o $@ $(LDFLAGS)

$(BIN_CLIENT): $(SRC_CLIENT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC_CLIENT) -o $@ $(LDFLAGS)

run: $(BIN_SERVER)
	./$(BIN_SERVER) -p 5559

clean:
	$(RM) $(BIN_SERVER) $(BIN_CLIENT)
