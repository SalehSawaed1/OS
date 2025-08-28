#include <iostream>
#include <string>
#include <unordered_map>
#include "graph.hpp"
#include "algo.hpp"
#include <memory>


// Small helper: build KV from {{"k","v"}, ...}
static KV P(std::initializer_list<std::pair<std::string, std::string>> kv) {
    KV m;
    m.reserve(kv.size());
    for (const auto& e : kv) m.emplace(e.first, e.second);
    return m;
}


int main(){
    // 1) SCC_COUNT: directed and undirected
    {
        Graph gd(5, true);
        gd.add_edge(0,1); gd.add_edge(1,2); gd.add_edge(2,0); gd.add_edge(3,4);
        std::unique_ptr<IAlgorithm> A(make_algorithm("SCC_COUNT"));
        auto r1 = A->run(gd, P({})); std::cout << r1.text << "\n";

        Graph gu(5, false);
        gu.add_edge(0,1); gu.add_edge(1,2); gu.add_edge(3,4);
        std::unique_ptr<IAlgorithm> B(make_algorithm("SCC_COUNT"));
        auto r2 = B->run(gu, P({})); std::cout << r2.text << "\n";
    }

    // 2) HAM_CYCLE: a yes-case and a no-case with precheck
    {
        Graph gyes(4,false); // 4-cycle
        gyes.add_edge(0,1); gyes.add_edge(1,2); gyes.add_edge(2,3); gyes.add_edge(3,0);
        std::unique_ptr<IAlgorithm> H(make_algorithm("HAM_CYCLE"));
        auto r = H->run(gyes, P({{"limit","10"},{"timeout_ms","200"}}));
        std::cout << r.text << "\n";

        Graph gno(4,false); // degree-1 vertex -> precheck fail
        gno.add_edge(0,1); gno.add_edge(1,2);
        std::unique_ptr<IAlgorithm> H2(make_algorithm("HAM_CYCLE"));
        auto r2 = H2->run(gno, P({{"limit","10"},{"timeout_ms","200"}}));
        std::cout << r2.text << "\n";
    }

    // 3) MAXCLIQUE & NUM_MAXCLIQUES
    {
        Graph g(5,false);
        // Make a K3 on {0,1,2} and extra edges
        g.add_edge(0,1); g.add_edge(1,2); g.add_edge(0,2);
        g.add_edge(2,3); g.add_edge(3,4);
        std::unique_ptr<IAlgorithm> MC(make_algorithm("MAXCLIQUE"));
        auto r = MC->run(g, P({{"timeout_ms","200"}}));
        std::cout << r.text << "\n";

        std::unique_ptr<IAlgorithm> NM(make_algorithm("NUM_MAXCLIQUES"));
        auto r2 = NM->run(g, P({{"timeout_ms","200"}}));
        std::cout << r2.text << "\n";
    }

    return 0;
}
