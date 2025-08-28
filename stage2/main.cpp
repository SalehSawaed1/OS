#include <iostream>
#include <vector>
#include "graph.hpp"   // from ../stage1 (Makefile adds -I../stage1)
#include "euler.hpp"

static void print_result(const EulerResult& r) {
    if (r.exists) {
        std::cout << "Euler circuit exists (" << (r.directed ? "directed" : "undirected") << ").\nPath: ";
        for (std::size_t i = 0; i < r.circuit.size(); ++i) {
            if (i) std::cout << " -> ";
            std::cout << r.circuit[i];
        }
        std::cout << "\n";
    } else {
        std::cout << "NO Euler circuit. Reason: " << r.reason << "\n";
    }
}

int main() {
    {
        // Undirected Eulerian example: 0-1-2-3-0 (cycle)
        Graph g(4, /*directed=*/false);
        g.add_edge(0,1); g.add_edge(1,2); g.add_edge(2,3); g.add_edge(3,0);
        auto r = euler_find(g);
        std::cout << "[Undirected example]\n";
        print_result(r);
    }
    {
        // Undirected NON-Eulerian example: a path 0-1-2 (two odd-degree vertices)
        Graph g(3, /*directed=*/false);
        g.add_edge(0,1); g.add_edge(1,2);
        auto r = euler_find(g);
        std::cout << "\n[Undirected non-euler example]\n";
        print_result(r);
    }
    {
        // Directed Eulerian example: 0->1->2->0 (balanced and strongly connected)
        Graph g(3, /*directed=*/true);
        g.add_edge(0,1); g.add_edge(1,2); g.add_edge(2,0);
        auto r = euler_find(g);
        std::cout << "\n[Directed example]\n";
        print_result(r);
    }
    return 0;
}
