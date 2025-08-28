#pragma once
#include <vector>
#include <string>
#include "graph.hpp"

struct EulerResult {
    bool exists{false};
    bool directed{false};
    std::vector<int> circuit;  // sequence of vertices: v0, v1, ..., v0
    std::string reason;        // if !exists, human-readable reason
};

// Decide and (if possible) construct an Euler circuit.
EulerResult euler_find(const Graph& g);
