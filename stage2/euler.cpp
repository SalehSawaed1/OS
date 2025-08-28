#include "euler.hpp"
#include <vector>
#include <queue>
#include <unordered_set>
#include <algorithm>

namespace {

// ---------- helpers: degrees ----------
std::vector<std::size_t> out_deg(const Graph& g) {
    std::vector<std::size_t> d(g.n, 0);
    for (std::size_t u = 0; u < g.n; ++u) d[u] = g.adj[u].size();
    return d;
}

std::vector<std::size_t> in_deg(const Graph& g) {
    std::vector<std::size_t> d(g.n, 0);
    for (std::size_t u = 0; u < g.n; ++u)
        for (int v : g.adj[u]) ++d[v];
    return d;
}

inline bool has_any_edges(const Graph& g) {
    for (std::size_t u = 0; u < g.n; ++u) if (!g.adj[u].empty()) return true;
    return false;
}

// ---------- connectivity checks ----------
bool undirected_connected_ignoring_isolated(const Graph& g) {
    if (g.n == 0) return true;

    // find a start with degree>0
    int start = -1;
    for (std::size_t i = 0; i < g.n; ++i) if (!g.adj[i].empty()) { start = (int)i; break; }
    if (start == -1) return true; // no edges -> trivially ok

    // BFS on the (already symmetric) adjacency
    std::vector<char> vis(g.n, 0);
    std::queue<int> q;
    q.push(start); vis[start]=1;

    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : g.adj[u]) if (!vis[v]) { vis[v]=1; q.push(v); }
    }

    for (std::size_t i = 0; i < g.n; ++i)
        if (!g.adj[i].empty() && !vis[i]) return false;
    return true;
}

// For directed: check strong connectivity on vertices with deg>0.
bool strongly_connected_on_non_isolated(const Graph& g) {
    const auto out = out_deg(g);
    const auto in  = in_deg(g);
    auto has_deg = [&](std::size_t i){ return out[i] + in[i] > 0; };

    int start = -1;
    for (std::size_t i = 0; i < g.n; ++i) if (has_deg(i)) { start = (int)i; break; }
    if (start == -1) return true; // no edges

    // forward reachability
    std::vector<char> vis(g.n, 0);
    std::vector<int> st{start};
    vis[start]=1;
    while (!st.empty()) {
        int u = st.back(); st.pop_back();
        for (int v : g.adj[u]) if (!vis[v]) { vis[v]=1; st.push_back(v); }
    }
    for (std::size_t i=0;i<g.n;++i) if (has_deg(i) && !vis[i]) return false;

    // reverse graph
    std::vector<std::vector<int>> radj(g.n);
    for (std::size_t u=0;u<g.n;++u) for (int v: g.adj[u]) radj[v].push_back((int)u);
    std::fill(vis.begin(), vis.end(), 0);
    st.assign(1,start);
    vis[start]=1;
    while (!st.empty()) {
        int u = st.back(); st.pop_back();
        for (int v : radj[u]) if (!vis[v]) { vis[v]=1; st.push_back(v); }
    }
    for (std::size_t i=0;i<g.n;++i) if (has_deg(i) && !vis[i]) return false;

    return true;
}

// ---------- Hierholzer (undirected) ----------
struct AdjEdge { int to; int id; };

std::vector<int> hierholzer_undirected(const Graph& g) {
    // Build edge-indexed adjacency, each undirected edge has one id used on both sides.
    std::vector<std::vector<AdjEdge>> adj2(g.n);
    int eid = 0;
    std::unordered_set<long long> seen; seen.reserve(g.m*2+16);
    auto key = [](int u,int v){ if (u>v) std::swap(u,v); return ( (long long)u<<32) | (unsigned int)v; };

    for (int u=0; u<(int)g.n; ++u) {
        for (int v : g.adj[u]) {
            long long k = key(u,v);
            if (seen.insert(k).second) {
                // add once, to both sides
                adj2[u].push_back({v, eid});
                adj2[v].push_back({u, eid});
                ++eid;
            }
        }
    }
    std::vector<char> used(eid, 0);
    std::vector<std::size_t> it(g.n, 0);

    // find start with degree>0
    int start = -1;
    for (int i=0;i<(int)g.n;++i) if (!adj2[i].empty()) { start = i; break; }
    if (start == -1) return {0}; // no edges: degenerate circuit at vertex 0 (if exists)

    std::vector<int> st; st.push_back(start);
    std::vector<int> circuit;
    while (!st.empty()) {
        int u = st.back();
        auto &lu = adj2[u];
        while (it[u] < lu.size() && used[lu[it[u]].id]) ++it[u];
        if (it[u] == lu.size()) {
            circuit.push_back(u);
            st.pop_back();
        } else {
            auto e = lu[it[u]];
            used[e.id] = 1;
            st.push_back(e.to);
        }
    }
    std::reverse(circuit.begin(), circuit.end());
    return circuit;
}

// ---------- Hierholzer (directed) ----------
std::vector<int> hierholzer_directed(const Graph& g) {
    std::vector<std::vector<AdjEdge>> adj2(g.n);
    int eid = 0;
    for (int u=0; u<(int)g.n; ++u) {
        for (int v : g.adj[u]) {
            adj2[u].push_back({v, eid++});
        }
    }
    std::vector<char> used(eid, 0);
    std::vector<std::size_t> it(g.n, 0);

    int start = -1;
    auto out = out_deg(g);
    auto in  = in_deg(g);
    for (int i=0;i<(int)g.n;++i) if (out[i]+in[i] > 0) { start = i; break; }
    if (start == -1) return {0}; // no edges

    std::vector<int> st; st.push_back(start);
    std::vector<int> circuit;
    while (!st.empty()) {
        int u = st.back();
        auto &lu = adj2[u];
        while (it[u] < lu.size() && used[lu[it[u]].id]) ++it[u];
        if (it[u] == lu.size()) {
            circuit.push_back(u);
            st.pop_back();
        } else {
            auto e = lu[it[u]];
            used[e.id] = 1;
            st.push_back(e.to);
        }
    }
    std::reverse(circuit.begin(), circuit.end());
    return circuit;
}

} // namespace

// ---------- public API ----------
EulerResult euler_find(const Graph& g) {
    EulerResult res;
    res.directed = g.directed;

    if (!has_any_edges(g)) {
        // Trivial graph: no edges — typically considered Eulerian.
        res.exists = true;
        res.circuit = { 0 }; // or empty; using {0} if vertex 0 exists
        return res;
    }

    if (!g.directed) {
        // Undirected conditions: connected (ignoring isolated) + all degrees even
        if (!undirected_connected_ignoring_isolated(g)) {
            res.exists = false;
            res.reason = "Graph is not connected on its non-isolated vertices.";
            return res;
        }
        auto deg = out_deg(g);
        for (auto d : deg) if (d % 2 != 0) {
            res.exists = false;
            res.reason = "A vertex has odd degree (all degrees must be even).";
            return res;
        }
        res.exists = true;
        res.circuit = hierholzer_undirected(g);
        return res;
    }

    // Directed conditions: for every non-isolated vertex, in==out and strongly connected
    auto out = out_deg(g);
    auto in  = in_deg(g);
    for (std::size_t i=0;i<g.n;++i)
        if (out[i] + in[i] > 0 && out[i] != in[i]) {
            res.exists = false;
            res.reason = "In-degree != Out-degree for at least one vertex.";
            return res;
        }
    if (!strongly_connected_on_non_isolated(g)) {
        res.exists = false;
        res.reason = "Graph is not strongly connected on its non-isolated vertices.";
        return res;
    }
    res.exists = true;
    res.circuit = hierholzer_directed(g);
    return res;
}
