#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "algo.hpp"
#include "graph.hpp"
#include <memory>

// ---- socket line I/O ----
static bool read_line(int fd, std::string& out){
    out.clear(); char c; ssize_t r;
    while(true){
        r = recv(fd, &c, 1, 0);
        if (r==0) return false;
        if (r<0)  return false;
        if (c=='\n') break;
        if (c=='\r') continue;
        out.push_back(c);
        if (out.size() > 2'000'000) return false;
    }
    return true;
}
static bool send_line(int fd, const std::string& s){
    size_t left=s.size(); const char* p=s.c_str();
    while(left){ ssize_t w=send(fd,p,left,0); if(w<=0) return false; p+=w; left-=w; }
    return send(fd, "\n", 1, 0) == 1;
}

// ---- exact G(n,m) generator (Robert Floyd sampling) ----
static inline std::pair<int,int> id_to_pair_directed(std::size_t n, unsigned long long id){
    unsigned long long row=id/(n-1), col=id%(n-1);
    int u=(int)row, v=(int)col; if(v>=u) ++v; return {u,v};
}
static inline std::pair<int,int> id_to_pair_undirected(std::size_t n, unsigned long long id){
    unsigned long long u=0, rem=id, len=n-1;
    while(rem>=len){ rem-=len; ++u; --len; }
    unsigned long long v=u+1+rem; return {(int)u,(int)v};
}
static std::vector<unsigned long long> sample_ids(unsigned long long N, unsigned long long m, std::mt19937& rng){
    std::unordered_set<unsigned long long> S; S.reserve((size_t)m*2+16);
    std::uniform_int_distribution<unsigned long long> dist;
    for(unsigned long long j=N-m; j<N; ++j){
        unsigned long long t=dist(rng, decltype(dist)::param_type(0,j));
        if(!S.insert(t).second) S.insert(j);
    }
    return std::vector<unsigned long long>(S.begin(), S.end());
}
static void generate_Gnm(Graph& g, std::size_t target_m, unsigned seed){
    if(g.n==0 || target_m==0) return;
    unsigned long long N = g.directed ? (unsigned long long)g.n*(g.n-1)
                                      : (unsigned long long)g.n*(g.n-1)/2ULL;
    if(target_m>N) target_m=(std::size_t)N;
    std::mt19937 rng(seed);
    auto ids = sample_ids(N, target_m, rng);
    for(auto id: ids){
        auto [u,v] = g.directed ? id_to_pair_directed(g.n,id) : id_to_pair_undirected(g.n,id);
        g.add_edge(u,v);
    }
}

// ---- request handling ----
static void handle(int cfd, const std::string& line){
    // Syntax:
    // ALG <NAME> RANDOM n=.. m=.. seed=.. directed=0|1 [limit=..]
    // ALG <NAME> GRAPH  n=.. directed=0|1 m=.. [limit=..]  + m lines "u v"
    std::vector<std::string> tok;
    { std::string cur; for(char ch: line){ if(ch==' '||ch=='\t'){ if(!cur.empty()){ tok.push_back(cur); cur.clear(); } } else cur.push_back(ch); } if(!cur.empty()) tok.push_back(cur); }

    if (tok.size() < 3 || tok[0] != "ALG") { send_line(cfd, "ERR expected 'ALG <NAME> <MODE>'"); return; }
    std::string alg = tok[1];
    std::string mode = tok[2];
    KV params = kv_from_tokens(std::vector<std::string>(tok.begin()+3, tok.end()));

    std::size_t n=0, m=0; unsigned seed=0; int directed=0;
    if (mode == "RANDOM") {
        if (!kv_get_size_t(params, "n", n)) { send_line(cfd, "ERR missing n"); return; }
        if (!kv_get_size_t(params, "m", m)) { send_line(cfd, "ERR missing m"); return; }
        kv_get_uint(params, "seed", seed);
        kv_get_int(params, "directed", directed);

        Graph g(n, directed!=0);
        generate_Gnm(g, m, seed);

        std::unique_ptr<IAlgorithm> A(make_algorithm(alg));
        if (!A) { send_line(cfd, "ERR unknown algorithm"); return; }

        auto res = A->run(g, params);
        send_line(cfd, std::string("OK ")+alg+" "+res.text);
        return;
    } else if (mode == "GRAPH") {
        if (!kv_get_size_t(params, "n", n)) { send_line(cfd, "ERR missing n"); return; }
        if (!kv_get_size_t(params, "m", m)) { send_line(cfd, "ERR missing m"); return; }
        kv_get_int(params, "directed", directed);

        Graph g(n, directed!=0);
        for (std::size_t i=0; i<m; ++i) {
            std::string el; if (!read_line(cfd, el)) { send_line(cfd, "ERR premature end while reading edges"); return; }
            int u=-1, v=-1; if (std::sscanf(el.c_str(), "%d %d", &u, &v) != 2) { send_line(cfd, "ERR bad edge format"); return; }
            g.add_edge(u, v);
        }

        std::unique_ptr<IAlgorithm> A(make_algorithm(alg));
        if (!A) { send_line(cfd, "ERR unknown algorithm"); return; }

        auto res = A->run(g, params);
        send_line(cfd, std::string("OK ")+alg+" "+res.text);
        return;
    } else {
        send_line(cfd, "ERR mode must be RANDOM or GRAPH");
        return;
    }
}

static void usage(const char* p){ std::cerr<<"Usage: "<<p<<" -p <port>\n"; }

int main(int argc, char** argv){
    int port=5557;
    for (int i=1; i<argc; ++i) {
        if (std::string(argv[i])=="-p" && i+1<argc) port = std::atoi(argv[++i]);
        else { usage(argv[0]); return 2; }
    }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); return 1; }
    int yes=1; setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons((uint16_t)port); addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if (bind(sfd,(sockaddr*)&addr,sizeof(addr))<0){ perror("bind"); return 1; }
    if (listen(sfd,16)<0){ perror("listen"); return 1; }

    std::cout<<"Stage7 server listening on port "<<port<<" ...\n";
    while (true) {
        sockaddr_in cli{}; socklen_t cl=sizeof(cli);
        int cfd = accept(sfd, (sockaddr*)&cli, &cl);
        if (cfd < 0) { perror("accept"); continue; }
        std::string line;
        if (read_line(cfd, line)) handle(cfd, line);
        close(cfd);
    }
    close(sfd); return 0;
}
