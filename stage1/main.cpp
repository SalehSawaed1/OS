#include <iostream>
#include "graph.hpp"

int main() {
    // Stage 1 demo: build a small undirected graph and print its adjacency/degree info.
    Graph g(5, /*directed=*/false);
    g.add_edge(0,1);
    g.add_edge(0,2);
    g.add_edge(1,2);
    g.add_edge(3,4);
    // duplicate/self-loop attempts (ignored):
    g.add_edge(1,2);
    g.add_edge(2,2);

    std::cout << g.to_string();

    // Try removing an edge:
    g.remove_edge(3,4);
    std::cout << "\nAfter removing (3,4):\n" << g.to_string();
    return 0;
}
