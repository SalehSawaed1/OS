#include "algo.hpp"
#include <queue>
#include <algorithm>
#include <functional>
#include <chrono>

using Clock = std::chrono::steady_clock;

struct Budget {
    Clock::time_point deadline{};
    size_t step_limit{0};
    size_t steps{0};
    bool timed_out() {
        ++steps;
        if (step_limit && steps >= step_limit) return true;
        if (deadline != Clock::time_point{} && Clock::now() >= deadline) return true;
        return false;
    }
};
static int get_timeout_ms(const KV& params, int def_ms=300) {
    auto it = params.find("timeout_ms");
    return (it==params.end()) ? def_ms : std::max(1, std::atoi(it->second.c_str()));
}
static size_t get_step_limit(const KV& params, size_t def_steps=500000) {
    auto it = params.find("step_limit");
    if (it==params.end()) return def_steps;
    unsigned long long v = std::strtoull(it->second.c_str(), nullptr, 10);
    return v ? (size_t)v : def_steps;
}

// ---------- Small helpers ----------
static std::vector<std::size_t> out_deg(const Graph& g){
    std::vector<std::size_t> d(g.n,0);
    for (std::size_t u=0; u<g.n; ++u) d[u] = g.adj[u].size();
    return d;
}
static std::vector<std::vector<int>> reverse_adj(const Graph& g){
    std::vector<std::vector<int>> radj(g.n);
    for (std::size_t u=0; u<g.n; ++u)
        for (int v : g.adj[u]) radj[v].push_back((int)u);
    return radj;
}

// ---------- (v) SCC count (Kosaraju) ----------
static int count_connected_undirected(const Graph& g){
    std::vector<char> vis(g.n,0);
    int comps=0;
    for (std::size_t s=0; s<g.n; ++s) if (!vis[s]) {
        ++comps;
        std::queue<int> q; q.push((int)s); vis[s]=1;
        while(!q.empty()){
            int u=q.front(); q.pop();
            for (int v : g.adj[u]) if(!vis[v]){ vis[v]=1; q.push(v); }
            if (g.directed) { // add reverse neighbors when graph is directed but we want CC
                for (std::size_t x=0; x<g.n; ++x)
                    for (int y : g.adj[x]) if (y==u && !vis[(int)x]){ vis[(int)x]=1; q.push((int)x); }
            }
        }
    }
    return comps;
}
static int count_scc_kosaraju(const Graph& g){
    if (!g.directed) return count_connected_undirected(g);
    std::size_t n = g.n;
    std::vector<char> vis(n,0);
    std::vector<int> order; order.reserve(n);
    std::function<void(int)> dfs1 = [&](int u){
        vis[u]=1;
        for (int v : g.adj[u]) if(!vis[v]) dfs1(v);
        order.push_back(u);
    };
    for (std::size_t i=0;i<n;++i) if(!vis[i]) dfs1((int)i);
    auto radj = reverse_adj(g);
    std::fill(vis.begin(), vis.end(), 0);
    int comps=0;
    std::function<void(int)> dfs2 = [&](int u){
        vis[u]=1;
        for (int v : radj[u]) if(!vis[v]) dfs2(v);
    };
    for (int i=(int)order.size()-1; i>=0; --i) {
        int u=order[i]; if(!vis[u]){ ++comps; dfs2(u); }
    }
    return comps;
}

struct SccCount : IAlgorithm {
    const char* name() const override { return "SCC_COUNT"; }
    AlgoResult run(const Graph& g, const KV&){
        int c = count_scc_kosaraju(g);
        if (g.directed) return {true, "SCC count="+std::to_string(c)};
        return {true, "Graph undirected; connected components="+std::to_string(c)};
    }
};

// ---------- (iv) Hamiltonian cycle with prechecks + timeout ----------
static bool ham_cycle_backtrack(const Graph& g, int start, std::vector<int>& path,
                                std::vector<char>& used, int depth, Budget& B)
{
    if (B.timed_out()) return false;
    if (depth == (int)g.n) {
        // close the cycle
        int u = path.back(), v = start;
        return std::find(g.adj[u].begin(), g.adj[u].end(), v) != g.adj[u].end();
    }
    int u = path.back();
    // Order neighbors by smaller degree first (light heuristic)
    std::vector<int> nbr = g.adj[u];
    std::sort(nbr.begin(), nbr.end(), [&](int a, int b){ return g.adj[a].size() < g.adj[b].size(); });
    for (int v : nbr) if (!used[v]) {
        used[v]=1; path.push_back(v);
        if (ham_cycle_backtrack(g, start, path, used, depth+1, B)) return true;
        path.pop_back(); used[v]=0;
        if (B.timed_out()) return false;
    }
    return false;
}
static bool quick_ham_impossible(const Graph& g){
    if (!g.directed) {
        // necessary conditions: connected + all degrees >= 2 (Ore/Dirac are stronger but this is cheap)
        // quick connectivity (undirected)
        std::vector<char> vis(g.n,0);
        int s=-1; for (size_t i=0;i<g.n;++i) if(!g.adj[i].empty()){ s=(int)i; break; }
        if (s==-1) return true; // no edges
        std::queue<int> q; q.push(s); vis[s]=1; size_t seen=1;
        while(!q.empty()){ int u=q.front(); q.pop(); for(int v:g.adj[u]) if(!vis[v]){ vis[v]=1; q.push(v); ++seen; } }
        if (seen<g.n) return true;
        for (size_t i=0;i<g.n;++i) if (g.adj[i].size() < 2) return true;
        return false;
    } else {
        // necessary: every vertex has in>=1 and out>=1, and strongly connected
        auto out = out_deg(g);
        std::vector<std::size_t> in(g.n,0); for(size_t u=0;u<g.n;++u) for(int v:g.adj[u]) ++in[v];
        for (size_t i=0;i<g.n;++i) if (out[i]==0 || in[i]==0) return true;
        // strong connectivity
        // forward
        std::vector<char> vis(g.n,0); int s=0; vis[s]=1; std::vector<int> st{0};
        while(!st.empty()){ int u=st.back(); st.pop_back(); for(int v:g.adj[u]) if(!vis[v]){ vis[v]=1; st.push_back(v);} }
        for(size_t i=0;i<g.n;++i) if(!vis[i]) return true;
        // reverse
        auto radj = reverse_adj(g);
        std::fill(vis.begin(), vis.end(), 0); vis[0]=1; st={0};
        while(!st.empty()){ int u=st.back(); st.pop_back(); for(int v:radj[u]) if(!vis[v]){ vis[v]=1; st.push_back(v);} }
        for(size_t i=0;i<g.n;++i) if(!vis[i]) return true;
        return false;
    }
}
static AlgoResult ham_cycle(const Graph& g, const KV& params){
    int limit_n = 18; // cap search size
    if (auto it=params.find("limit"); it!=params.end()) limit_n = std::max(1, std::atoi(it->second.c_str()));
    if ((int)g.n > limit_n) return {true, "HAM: n="+std::to_string(g.n)+" exceeds limit="+std::to_string(limit_n)+" (skip)"};
    if (g.n == 0) return {true, "HAM: trivial YES (empty)"};
    if (quick_ham_impossible(g)) return {true, "NO Hamilton cycle (quick precheck)"};

    Budget B; B.deadline = Clock::now() + std::chrono::milliseconds(get_timeout_ms(params, 300));
    B.step_limit = get_step_limit(params, 800000); // recursion guard

    // start at min-degree vertex helps
    int start = 0; for (size_t i=1;i<g.n;++i) if (g.adj[i].size() < g.adj[start].size()) start=(int)i;

    std::vector<int> path; path.reserve(g.n); path.push_back(start);
    std::vector<char> used(g.n,0); used[start]=1;
    bool ok = ham_cycle_backtrack(g, start, path, used, 1, B);
    if (ok) {
        std::string out = "YES Hamilton cycle: ";
        for (size_t i=0;i<path.size();++i){ if(i) out+=" -> "; out+=std::to_string(path[i]); }
        out += " -> " + std::to_string(start);
        return {true, out};
    }
    if (B.timed_out()) return {true, "HAM: TIMEOUT"};
    return {true, "NO Hamilton cycle"};
}

struct Hamilton : IAlgorithm {
    const char* name() const override { return "HAM_CYCLE"; }
    AlgoResult run(const Graph& g, const KV& params){ return ham_cycle(g, params); }
};

// ---------- (i, ii) Bron–Kerbosch with pivot + pruning + timeout ----------
struct BKState {
    const std::vector<std::vector<int>>& adj; // undirected neighbor lists (sorted)
    Budget& B;
    int best=0;
    std::vector<int> bestR;
    long long countMaximal=0;
    bool aborted=false;
};

static inline bool is_adj(const std::vector<std::vector<int>>& A, int u, int v){
    return std::binary_search(A[u].begin(), A[u].end(), v);
}
// Compute N(v) ∩ S where S is sorted vector
static std::vector<int> inter_neighbors(const std::vector<std::vector<int>>& A, int v, const std::vector<int>& S){
    std::vector<int> out; out.reserve(std::min(A[v].size(), S.size()));
    auto &Nv = A[v];
    std::set_intersection(Nv.begin(), Nv.end(), S.begin(), S.end(), std::back_inserter(out));
    return out;
}

static void bk_recurse(BKState& st, std::vector<int>& R, std::vector<int>& P, std::vector<int>& X, bool recordBest){
    if (st.B.timed_out()) { st.aborted=true; return; }

    // Branch & bound: if we cannot beat current best, prune
    if (recordBest && (int)R.size() + (int)P.size() <= st.best) return;

    if (P.empty() && X.empty()) {
        // R is maximal clique
        ++st.countMaximal;
        if (recordBest && (int)R.size() > st.best) { st.best=(int)R.size(); st.bestR=R; }
        return;
    }

    // Choose pivot u from P ∪ X with max neighbors in P
    int u=-1, maxN=-1;
    {
        std::vector<int> U = P; U.insert(U.end(), X.begin(), X.end());
        for (int cand : U) {
            int cnt=0;
            // count neighbors of cand that are in P (two-pointer)
            auto &Nv = st.adj[cand];
            auto itP = P.begin();
            for (int w : Nv) {
                while (itP!=P.end() && *itP < w) ++itP;
                if (itP!=P.end() && *itP==w) ++cnt;
            }
            if (cnt > maxN) { maxN=cnt; u=cand; }
        }
    }

    // Candidates = P \ N(u)
    std::vector<char> isNbr(st.adj.size(), 0);
    if (u!=-1) for (int v : st.adj[u]) isNbr[v]=1;

    std::vector<int> cand;
    cand.reserve(P.size());
    for (int v : P) if (!(u!=-1 && isNbr[v])) cand.push_back(v);

    // Iterate candidates (smallest degree first often helps)
    std::sort(cand.begin(), cand.end(), [&](int a,int b){ return st.adj[a].size() < st.adj[b].size(); });

    for (int v : cand) {
        if (st.B.timed_out()) { st.aborted=true; return; }
        R.push_back(v);
        auto P2 = inter_neighbors(st.adj, v, P);
        auto X2 = inter_neighbors(st.adj, v, X);
        bk_recurse(st, R, P2, X2, recordBest);
        R.pop_back();
        // move v from P to X
        P.erase(std::find(P.begin(), P.end(), v));
        X.push_back(v);
        if (st.aborted) return;
    }
}

static std::vector<std::vector<int>> make_adj_undirected(const Graph& g){
    // Treat edges as undirected (needed for clique problems)
    std::vector<std::vector<int>> A(g.n);
    for (std::size_t u=0; u<g.n; ++u) {
        for (int v : g.adj[u]) {
            A[u].push_back(v);
            if (g.directed) A[v].push_back((int)u);
        }
    }
    for (auto& row : A) {
        std::sort(row.begin(), row.end());
        row.erase(std::unique(row.begin(), row.end()), row.end());
    }
    return A;
}

struct MaxClique : IAlgorithm {
    const char* name() const override { return "MAXCLIQUE"; }
    AlgoResult run(const Graph& g, const KV& params){
        auto A = make_adj_undirected(g);
        // initial P = all vertices sorted by ascending deg (degeneracy-ish)
        std::vector<int> P(g.n), X, R; for (size_t i=0;i<g.n;++i) P[i]=(int)i;
        std::sort(P.begin(), P.end(), [&](int a,int b){ return A[a].size() < A[b].size(); });

        Budget B; B.deadline = Clock::now() + std::chrono::milliseconds(get_timeout_ms(params, 300));
        B.step_limit = get_step_limit(params, 800000);

        BKState st{A, B};
        bk_recurse(st, R, P, X, /*recordBest=*/true);

        if (st.aborted) return {true, "MAXCLIQUE: TIMEOUT (current best="+std::to_string(st.best)+")"};
        std::string out = "MaxClique size=" + std::to_string(st.best) + " example:";
        for (size_t i=0;i<st.bestR.size();++i){ out += (i? " ":" "); out += std::to_string(st.bestR[i]); }
        return {true, out};
    }
};

struct NumMaxCliques : IAlgorithm {
    const char* name() const override { return "NUM_MAXCLIQUES"; }
    AlgoResult run(const Graph& g, const KV& params){
        auto A = make_adj_undirected(g);
        std::vector<int> P(g.n), X, R; for (size_t i=0;i<g.n;++i) P[i]=(int)i;
        std::sort(P.begin(), P.end(), [&](int a,int b){ return A[a].size() < A[b].size(); });

        Budget B; B.deadline = Clock::now() + std::chrono::milliseconds(get_timeout_ms(params, 300));
        B.step_limit = get_step_limit(params, 800000);

        BKState st{A, B};
        bk_recurse(st, R, P, X, /*recordBest=*/false);

        if (st.aborted) return {true, "NUM_MAXCLIQUES: TIMEOUT (count so far="+std::to_string(st.countMaximal)+")"};
        return {true, "Maximal cliques count="+std::to_string(st.countMaximal)};
    }
};

IAlgorithm* make_algorithm(const std::string& name){
    if (name == "SCC_COUNT")      return new SccCount();
    if (name == "HAM_CYCLE")      return new Hamilton();
    if (name == "MAXCLIQUE")      return new MaxClique();
    if (name == "NUM_MAXCLIQUES") return new NumMaxCliques();
    return nullptr;
}
