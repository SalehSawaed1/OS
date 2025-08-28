CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wshadow -Wpedantic -O0 -g

# Reuse your previous stages (no duplication)
STAGE1_DIR := ../stage1
STAGE2_DIR := ../stage2

BIN_SERVER := server
BIN_CLIENT := client

SRC_SERVER := server.cpp $(STAGE1_DIR)/graph.cpp $(STAGE2_DIR)/euler.cpp
SRC_CLIENT := client.cpp

INCLUDES := -I$(STAGE1_DIR) -I$(STAGE2_DIR)

.PHONY: all clean run

all: $(BIN_SERVER) $(BIN_CLIENT)

$(BIN_SERVER): $(SRC_SERVER)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC_SERVER) -o $@

$(BIN_CLIENT): $(SRC_CLIENT)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC_CLIENT) -o $@

run: $(BIN_SERVER)
	./$(BIN_SERVER) -p 5555

clean:
	$(RM) $(BIN_SERVER) $(BIN_CLIENT)
