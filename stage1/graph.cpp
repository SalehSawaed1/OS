#include "graph.hpp"
#include <algorithm>
#include <sstream>

namespace {
    inline bool in_range(std::size_t n, int v){ return v>=0 && static_cast<std::size_t>(v)<n; }
}

Graph::Graph(std::size_t n_, bool directed_) : n(n_), directed(directed_), adj(n_), m(0) {}

void Graph::add_edge(int u, int v) {
    if (!in_range(n,u) || !in_range(n,v) || u==v) return;

    auto &Au = adj[u];
    if (std::find(Au.begin(), Au.end(), v) != Au.end()) return; // already present

    Au.push_back(v);
    if (!directed) {
        auto &Av = adj[v];
        if (std::find(Av.begin(), Av.end(), u) == Av.end()) Av.push_back(u);
    }
    m += 1;
}

bool Graph::remove_edge(int u, int v) {
    if (!in_range(n,u) || !in_range(n,v) || u==v) return false;

    bool removed = false;
    auto &Au = adj[u];
    auto itu = std::find(Au.begin(), Au.end(), v);
    if (itu != Au.end()) { Au.erase(itu); removed = true; }

    if (!directed) {
        auto &Av = adj[v];
        auto itv = std::find(Av.begin(), Av.end(), u);
        if (itv != Av.end()) Av.erase(itv);
    }

    if (removed) m -= 1;
    return removed;
}

std::vector<std::size_t> Graph::out_degrees() const {
    std::vector<std::size_t> d(n,0);
    for (std::size_t u=0; u<n; ++u) d[u] = adj[u].size();
    return d;
}

std::vector<std::size_t> Graph::in_degrees() const {
    std::vector<std::size_t> d(n,0);
    for (std::size_t u=0; u<n; ++u)
        for (int v : adj[u]) ++d[v];
    return d;
}

bool Graph::validate() const {
    if (directed) return true;
    for (std::size_t u=0; u<n; ++u) {
        for (int v : adj[u]) {
            if (std::find(adj[v].begin(), adj[v].end(), (int)u) == adj[v].end())
                return false;
        }
    }
    return true;
}

std::string Graph::to_string() const {
    std::ostringstream os;
    os << (directed? "Directed":"Undirected") << " Graph: n="<<n<<" m="<<m<<"\n";
    for (std::size_t u=0; u<n; ++u) {
        os << "  " << u << ":";
        for (int v: adj[u]) os << " " << v;
        os << "\n";
    }
    auto out = out_degrees(); os << "Degrees(out):";
    for (auto d: out) os << " " << d;
    if (directed) {
        auto in = in_degrees(); os << "\nDegrees(in):";
        for (auto d: in) os << " " << d;
    }
    os << "\nvalid=" << (validate() ? "true":"false") << "\n";
    return os.str();
}
