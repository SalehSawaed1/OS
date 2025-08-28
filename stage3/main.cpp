#include <iostream>
#include <getopt.h>
#include <cstdlib>
#include <random>
#include <unordered_set>
#include <vector>
#include <limits>
#include "graph.hpp"
#include "euler.hpp"

// --- usage ---
static void usage(const char* prog){
    std::cerr<<"Usage: "<<prog<<" -n <vertices> -m <edges> -s <seed> [-d]\n"
             <<"  -n, --nodes     number of vertices (>=1)\n"
             <<"  -m, --edges     number of edges (no self-loops, no duplicates)\n"
             <<"  -s, --seed      RNG seed (unsigned)\n"
             <<"  -d, --directed  directed graph (default undirected)\n";
}

// --- map id->[u,v] for all possible edges ---
static inline std::pair<int,int> id_to_pair_directed(std::size_t n, unsigned long long id){
    unsigned long long row = id / (n-1);
    unsigned long long col = id % (n-1);
    int u = (int)row;
    int v = (int)col;
    if (v >= u) ++v; // skip self
    return {u,v};
}

static inline std::pair<int,int> id_to_pair_undirected(std::size_t n, unsigned long long id){
    // triangular indexing inversion
    // rows: u=0..n-2, each with len = (n-1-u), columns v=u+1..n-1
    unsigned long long u = 0;
    unsigned long long remaining = id;
    unsigned long long len = n-1; // row length for u=0
    while (remaining >= len) { remaining -= len; ++u; --len; }
    unsigned long long v = u + 1 + remaining;
    return {(int)u, (int)v};
}

// --- Robert Floyd sampling: pick m unique ids in [0..N) without replacement ---
static std::vector<unsigned long long> sample_ids(unsigned long long N, unsigned long long m, std::mt19937& rng){
    std::unordered_set<unsigned long long> S;
    S.reserve((size_t)m*2+16);
    std::uniform_int_distribution<unsigned long long> dist;
    for (unsigned long long j = N - m; j < N; ++j) {
        unsigned long long t = dist(rng, decltype(dist)::param_type(0, j));
        if (!S.insert(t).second) S.insert(j);
    }
    return std::vector<unsigned long long>(S.begin(), S.end());
}

// --- build a true G(n,m) simple graph ---
static void generate_simple_graph_Gnm(Graph& g, std::size_t target_m, unsigned int seed){
    if (g.n == 0 || target_m == 0) return;

    unsigned long long N = g.directed ? (unsigned long long)g.n*(g.n-1)
                                      : (unsigned long long)g.n*(g.n-1)/2ULL;
    if (target_m > N) target_m = (std::size_t)N;

    std::mt19937 rng(seed);
    auto ids = sample_ids(N, target_m, rng);

    for (auto id : ids) {
        auto [u,v] = g.directed ? id_to_pair_directed(g.n, id)
                                : id_to_pair_undirected(g.n, id);
        g.add_edge(u, v);
    }
}

// --- pretty print result (limit long paths) ---
static void print_result(const EulerResult& r){
    if(r.exists){
        std::cout<<"Euler circuit exists ("<<(r.directed?"directed":"undirected")<<").\n";
        std::cout<<"Path ("<<r.circuit.size()<<" vertices): ";
        const std::size_t limit=200;
        for(std::size_t i=0;i<r.circuit.size();++i){
            if(i) std::cout<<" -> ";
            if(i<limit) std::cout<<r.circuit[i];
            else { std::cout<<"..."; break; }
        }
        std::cout<<"\n";
    } else {
        std::cout<<"NO Euler circuit. Reason: "<<r.reason<<"\n";
    }
}

int main(int argc, char** argv){
    std::size_t n=0,m=0; unsigned int seed=0; bool directed=false;

    const option long_opts[]={
        {"nodes",1,nullptr,'n'}, {"edges",1,nullptr,'m'},
        {"seed",1,nullptr,'s'},  {"directed",0,nullptr,'d'},
        {nullptr,0,nullptr,0}
    };

    int opt, idx;
    while((opt=getopt_long(argc, argv, "n:m:s:d", long_opts, &idx))!=-1){
        switch(opt){
            case 'n': n = std::strtoull(optarg,nullptr,10); break;
            case 'm': m = std::strtoull(optarg,nullptr,10); break;
            case 's': seed = (unsigned)std::strtoul(optarg,nullptr,10); break;
            case 'd': directed = true; break;
            default: usage(argv[0]); return 2;
        }
    }
    if (n == 0) { usage(argv[0]); return 2; }

    unsigned long long max_m = directed ? (unsigned long long)n*(n-1)
                                        : (unsigned long long)n*(n-1)/2ULL;
    if (m > max_m) {
        std::cerr << "[warn] requested edges " << m << " exceed maximum " << max_m
                  << " for n=" << n << (directed ? " (directed)":" (undirected)") << ". Clamping.\n";
        m = (std::size_t)max_m;
    }

    Graph g(n, directed);
    generate_simple_graph_Gnm(g, m, seed);

    std::cout<<"Graph generated: n="<<g.n<<", m="<<g.edges()<<", directed="<<(g.directed?1:0)<<"\n";
    auto res = euler_find(g);
    print_result(res);
    return 0;
}
