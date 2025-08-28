CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wshadow -Wpedantic -O2 -g
LDFLAGS := -pthread

STAGE1_DIR := ../stage1
STAGE7_DIR := ../stage7

BIN_SERVER := server8
BIN_CLIENT := client7   # we can reuse the Stage 7 client

SRC_SERVER := server8.cpp $(STAGE1_DIR)/graph.cpp $(STAGE7_DIR)/algorithms.cpp
SRC_CLIENT := $(STAGE7_DIR)/client7.cpp

INCLUDES := -I$(STAGE1_DIR) -I$(STAGE7_DIR)

.PHONY: all clean run

all: $(BIN_SERVER) $(BIN_CLIENT)

$(BIN_SERVER): $(SRC_SERVER)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC_SERVER) -o $@ $(LDFLAGS)

$(BIN_CLIENT): $(SRC_CLIENT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC_CLIENT) -o $@ $(LDFLAGS)

run: $(BIN_SERVER)
	./$(BIN_SERVER) -p 5558 -t 4

clean:
	$(RM) $(BIN_SERVER) $(BIN_CLIENT)
