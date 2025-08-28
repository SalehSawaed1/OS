#include <iostream>
#include "graph.hpp"
#include "euler.hpp"

int main(){
    // Euler: yes-case (cycle) & no-case (odd degree)
    {
        Graph g(4,false);
        g.add_edge(0,1); g.add_edge(1,2); g.add_edge(2,3); g.add_edge(3,0);
        auto r = euler_find(g);
        std::cout << (r.exists ? "EULER YES\n" : "EULER NO\n");
    }
    {
        Graph g(3,false);
        g.add_edge(0,1); g.add_edge(1,2); // degrees: 0->1, 1->2, 2->1 (two odd)
        auto r = euler_find(g);
        std::cout << (r.exists ? "EULER YES\n" : "EULER NO\n");
    }
    return 0;
}
