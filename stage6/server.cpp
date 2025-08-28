#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "graph.hpp"   // from ../stage1 (included via -I)
#include "euler.hpp"   // from ../stage2 (included via -I)
#include <random>

// ----------- tiny line I/O over sockets -----------
static bool read_line(int fd, std::string& out) {
    out.clear();
    char c;
    ssize_t r;
    while (true) {
        r = recv(fd, &c, 1, 0);
        if (r == 0) return false;        // peer closed
        if (r < 0)  return false;        // error
        if (c == '\n') break;
        if (c == '\r') continue;
        out.push_back(c);
        if (out.size() > 1'000'000) return false; // sanity
    }
    return true;
}
static bool send_str(int fd, const std::string& s) {
    const char* p = s.c_str();
    size_t left = s.size();
    while (left) {
        ssize_t w = send(fd, p, left, 0);
        if (w <= 0) return false;
        p += w; left -= (size_t)w;
    }
    return true;
}
static bool send_line(int fd, const std::string& s) {
    return send_str(fd, s) && send_str(fd, std::string("\n"));
}

// ----------- helpers: parse "k=v" tokens -----------
struct Params {
    std::size_t n = 0, m = 0;
    unsigned int seed = 0;
    bool directed = false;
};
static void parse_kv_tokens(const std::vector<std::string>& toks, Params& P) {
    for (auto& t : toks) {
        auto eq = t.find('=');
        if (eq == std::string::npos) continue;
        std::string k = t.substr(0, eq);
        std::string v = t.substr(eq+1);
        if (k == "n")      P.n = std::strtoull(v.c_str(), nullptr, 10);
        else if (k == "m") P.m = std::strtoull(v.c_str(), nullptr, 10);
        else if (k == "seed") P.seed = (unsigned)std::strtoul(v.c_str(), nullptr, 10);
        else if (k == "directed") P.directed = (v=="1" || v=="true" || v=="True");
    }
}

// ----------- G(n,m) generator (Robert Floyd sampling) -----------
static inline std::pair<int,int> id_to_pair_directed(std::size_t n, unsigned long long id) {
    unsigned long long row = id / (n-1);
    unsigned long long col = id % (n-1);
    int u = (int)row;
    int v = (int)col;
    if (v >= u) ++v; // skip self
    return {u,v};
}
static inline std::pair<int,int> id_to_pair_undirected(std::size_t n, unsigned long long id) {
    unsigned long long u = 0;
    unsigned long long remaining = id;
    unsigned long long len = n - 1;
    while (remaining >= len) { remaining -= len; ++u; --len; }
    unsigned long long v = u + 1 + remaining;
    return {(int)u, (int)v};
}
static std::vector<unsigned long long> sample_ids(unsigned long long N, unsigned long long m, std::mt19937& rng) {
    std::unordered_set<unsigned long long> S;
    S.reserve((size_t)m*2 + 16);
    std::uniform_int_distribution<unsigned long long> dist;
    for (unsigned long long j = N - m; j < N; ++j) {
        unsigned long long t = dist(rng, decltype(dist)::param_type(0, j));
        if (!S.insert(t).second) S.insert(j);
    }
    return std::vector<unsigned long long>(S.begin(), S.end());
}
static void generate_Gnm(Graph& g, std::size_t target_m, unsigned int seed) {
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

// ----------- request handling -----------
static void handle_request_line(int cfd, const std::string& line) {
    // Protocol:
    // 1) "EULER RANDOM n=.. m=.. seed=.. directed=0|1"
    // 2) "EULER GRAPH n=.. directed=0|1 m=.."  then we read m lines "u v"
    std::vector<std::string> tok;
    {
        std::string cur;
        for (char ch : line) {
            if (ch == ' ' || ch == '\t') {
                if (!cur.empty()) { tok.push_back(cur); cur.clear(); }
            } else cur.push_back(ch);
        }
        if (!cur.empty()) tok.push_back(cur);
    }
    if (tok.size() < 2 || tok[0] != "EULER") {
        send_line(cfd, "ERR expected 'EULER ...'"); return;
    }
    std::string mode = tok[1];
    Params P;
    if (mode == "RANDOM") {
        // parse remaining tokens as k=v
        std::vector<std::string> kv(tok.begin() + 2, tok.end());
        parse_kv_tokens(kv, P);
        if (P.n == 0) { send_line(cfd, "ERR n must be > 0"); return; }
        unsigned long long max_m = P.directed ? (unsigned long long)P.n*(P.n-1)
                                              : (unsigned long long)P.n*(P.n-1)/2ULL;
        if (P.m > max_m) P.m = (std::size_t)max_m;

        Graph g(P.n, P.directed);
        generate_Gnm(g, P.m, P.seed);
        auto res = euler_find(g);

        if (res.exists) {
            std::string out = "OK YES path:";
            // guard against huge prints
            size_t limit = std::min<size_t>(res.circuit.size(), 4000);
            for (size_t i=0; i<limit; ++i) { out.push_back(' '); out += std::to_string(res.circuit[i]); }
            if (limit < res.circuit.size()) out += " ...";
            send_line(cfd, out);
        } else {
            send_line(cfd, "OK NO reason: " + res.reason);
        }
        return;
    }
    else if (mode == "GRAPH") {
        // first line includes n, directed, m
        std::vector<std::string> kv(tok.begin() + 2, tok.end());
        parse_kv_tokens(kv, P);
        if (P.n == 0) { send_line(cfd, "ERR n must be > 0"); return; }
        Graph g(P.n, P.directed);

        // read m lines "u v"
        for (std::size_t i=0; i<P.m; ++i) {
            std::string eline;
            if (!read_line(cfd, eline)) { send_line(cfd, "ERR premature end while reading edges"); return; }
            int u=-1, v=-1;
            if (std::sscanf(eline.c_str(), "%d %d", &u, &v) != 2) { send_line(cfd, "ERR bad edge format"); return; }
            g.add_edge(u, v);
        }
        auto res = euler_find(g);
        if (res.exists) {
            std::string out = "OK YES path:";
            size_t limit = std::min<size_t>(res.circuit.size(), 4000);
            for (size_t i=0; i<limit; ++i) { out.push_back(' '); out += std::to_string(res.circuit[i]); }
            if (limit < res.circuit.size()) out += " ...";
            send_line(cfd, out);
        } else {
            send_line(cfd, "OK NO reason: " + res.reason);
        }
        return;
    }
    else {
        send_line(cfd, "ERR unknown mode (use RANDOM or GRAPH)");
    }
}

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " -p <port>\n";
}

int main(int argc, char** argv) {
    int port = 5555;
    // parse args
    for (int i=1; i<argc; ++i) {
        if (std::string(argv[i]) == "-p" && i+1 < argc) {
            port = std::atoi(argv[++i]);
        } else {
            usage(argv[0]); return 2;
        }
    }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); return 1; }

    int yes = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{}; addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(sfd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(sfd, 16) < 0) { perror("listen"); return 1; }

    std::cout << "Euler server listening on port " << port << " ...\n";

    while (true) {
        sockaddr_in cli{}; socklen_t clilen = sizeof(cli);
        int cfd = accept(sfd, (sockaddr*)&cli, &clilen);
        if (cfd < 0) { perror("accept"); continue; }

        std::string line;
        if (read_line(cfd, line)) {
            handle_request_line(cfd, line);
        } else {
            // client closed without sending
        }
        close(cfd);
    }

    close(sfd);
    return 0;
}
