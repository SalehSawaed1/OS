#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <memory>
#include <csignal>
#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "active.hpp"
#include "algo.hpp"      // Stage 7: IAlgorithm, make_algorithm, KV helpers
#include "graph.hpp"      // Stage 1

// -------- socket helpers --------
static bool read_line(int fd, std::string& out){
    out.clear(); char c; ssize_t r;
    while (true){
        r = recv(fd, &c, 1, 0);
        if (r == 0) return false;
        if (r < 0)  return false;
        if (c == '\n') break;
        if (c == '\r') continue;
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

// -------- exact G(n,m) generator (Robert Floyd) --------
static inline std::pair<int,int> id_to_pair_directed(std::size_t n, unsigned long long id){
    unsigned long long row=id/(n-1), col=id%(n-1);
    int u=(int)row, v=(int)col; if (v>=u) ++v; return {u,v};
}
static inline std::pair<int,int> id_to_pair_undirected(std::size_t n, unsigned long long id){
    unsigned long long u=0, rem=id, len=n-1;
    while(rem>=len){ rem-=len; ++u; --len; }
    unsigned long long v=u+1+rem; return {(int)u,(int)v};
}
static std::vector<unsigned long long> sample_ids(unsigned long long N, unsigned long long m, std::mt19937& rng){
    std::unordered_set<unsigned long long> S; S.reserve((size_t)m*2+16);
    std::uniform_int_distribution<unsigned long long> dist;
    for (unsigned long long j=N-m; j<N; ++j){
        unsigned long long t=dist(rng, decltype(dist)::param_type(0,j));
        if(!S.insert(t).second) S.insert(j);
    }
    return std::vector<unsigned long long>(S.begin(), S.end());
}
static void generate_Gnm(Graph& g, std::size_t target_m, unsigned seed){
    if (g.n==0 || target_m==0) return;
    unsigned long long N = g.directed ? (unsigned long long)g.n*(g.n-1)
                                      : (unsigned long long)g.n*(g.n-1)/2ULL;
    if (target_m > N) target_m = (std::size_t)N;
    std::mt19937 rng(seed);
    auto ids = sample_ids(N, target_m, rng);
    for (auto id : ids){
        auto [u,v] = g.directed ? id_to_pair_directed(g.n,id) : id_to_pair_undirected(g.n,id);
        g.add_edge(u, v);
    }
}

// -------- jobs through the pipeline --------
struct Request {
    int client_fd{-1};
    std::string alg;     // "SCC_COUNT" | "HAM_CYCLE" | ...
    Graph g{0,false};
    KV params;           // includes directed/seed/timeout_ms/etc
};

struct Response {
    int client_fd{-1};
    std::string text;    // final line to send (e.g., "OK ...")
};

// Forward declarations of handlers
struct Pipeline;
static void dispatch_handle(Request&& r, void* ctx);
static void scc_handle   (Request&& r, void* ctx);
static void ham_handle   (Request&& r, void* ctx);
static void maxclq_handle(Request&& r, void* ctx);
static void numclq_handle(Request&& r, void* ctx);
static void respond_handle(Response&& resp, void* ctx);

// -------- pipeline object holding all active objects --------
struct Pipeline {
    // 1 dispatcher, 4 algo workers, 1 responder
    ActiveObject<Request> dispatcher;
    ActiveObject<Request> scc_ao, ham_ao, maxclq_ao, numclq_ao;
    ActiveObject<Response> responder;
};

// -------- handlers --------
static void dispatch_handle(Request&& r, void* ctx){
    auto* P = static_cast<Pipeline*>(ctx);
    // route by algorithm name
    if (r.alg == "SCC_COUNT")          { P->scc_ao.post(std::move(r)); return; }
    if (r.alg == "HAM_CYCLE")          { P->ham_ao.post(std::move(r)); return; }
    if (r.alg == "MAXCLIQUE")          { P->maxclq_ao.post(std::move(r)); return; }
    if (r.alg == "NUM_MAXCLIQUES")     { P->numclq_ao.post(std::move(r)); return; }
    // unknown algorithm
    Response resp{r.client_fd, "ERR unknown algorithm"};
    P->responder.post(std::move(resp));
}

static void algorithm_run(const char* tag, Request&& r, void* ctx,
                          const char* alg_name){
    auto* P = static_cast<Pipeline*>(ctx);
    std::unique_ptr<IAlgorithm> A(make_algorithm(alg_name));
    if (!A) {
        P->responder.post(Response{r.client_fd, std::string("ERR unknown algorithm")});
        return;
    }
    auto res = A->run(r.g, r.params);
    std::string line = std::string("OK ") + alg_name + " " + res.text;
    (void)tag; // tag useful if you want logging
    P->responder.post(Response{r.client_fd, std::move(line)});
}
static void scc_handle(Request&& r, void* ctx)    { algorithm_run("SCC", std::move(r), ctx, "SCC_COUNT"); }
static void ham_handle(Request&& r, void* ctx)    { algorithm_run("HAM", std::move(r), ctx, "HAM_CYCLE"); }
static void maxclq_handle(Request&& r, void* ctx) { algorithm_run("MCQ", std::move(r), ctx, "MAXCLIQUE"); }
static void numclq_handle(Request&& r, void* ctx) { algorithm_run("NCQ", std::move(r), ctx, "NUM_MAXCLIQUES"); }

static void respond_handle(Response&& resp, void*){
    if (resp.client_fd >= 0) {
        send_line(resp.client_fd, resp.text);
        close(resp.client_fd);
    }
}

// -------- request parsing (Stage 7 protocol) --------
static bool parse_and_build(int cfd, const std::string& firstLine, Request& out){
    // First tokenized line: "ALG <NAME> RANDOM ..." or "ALG <NAME> GRAPH ..."
    std::vector<std::string> tok; tok.reserve(16);
    { std::string cur;
      for (char ch: firstLine) { if(ch==' '||ch=='\t'){ if(!cur.empty()){ tok.push_back(cur); cur.clear(); } } else cur.push_back(ch); }
      if(!cur.empty()) tok.push_back(cur);
    }
    if (tok.size()<3 || tok[0]!="ALG") { send_line(cfd,"ERR expected 'ALG <NAME> <MODE>'"); return false; }

    out.client_fd = cfd;
    out.alg = tok[1];
    std::string mode = tok[2];
    KV params = kv_from_tokens(std::vector<std::string>(tok.begin()+3, tok.end()));

    std::size_t n=0, m=0; int directed=0; unsigned seed=0;
    if (mode=="RANDOM"){
        if (!kv_get_size_t(params,"n",n)) { send_line(cfd,"ERR missing n"); return false; }
        if (!kv_get_size_t(params,"m",m)) { send_line(cfd,"ERR missing m"); return false; }
        kv_get_int (params,"directed",directed);
        kv_get_uint(params,"seed",seed);
        Graph g(n, directed!=0);
        generate_Gnm(g, m, seed);
        out.g = std::move(g);
        out.params = std::move(params);
        return true;
    } else if (mode=="GRAPH"){
        if (!kv_get_size_t(params,"n",n)) { send_line(cfd,"ERR missing n"); return false; }
        if (!kv_get_size_t(params,"m",m)) { send_line(cfd,"ERR missing m"); return false; }
        kv_get_int(params,"directed",directed);
        Graph g(n, directed!=0);
        for (std::size_t i=0;i<m;++i){
            std::string el;
            if (!read_line(cfd, el)) { send_line(cfd,"ERR premature end while reading edges"); return false; }
            int u=-1,v=-1; if (std::sscanf(el.c_str(), "%d %d", &u, &v) != 2) { send_line(cfd,"ERR bad edge format"); return false; }
            g.add_edge(u, v);
        }
        out.g = std::move(g);
        out.params = std::move(params);
        return true;
    } else {
        send_line(cfd,"ERR mode must be RANDOM or GRAPH");
        return false;
    }
}

// -------- main: acceptor + pipeline wiring --------
static std::atomic<bool> running(true);
static int listen_fd = -1;
static void on_sigint(int){ running.store(false); if(listen_fd>=0) close(listen_fd); }

static void usage(const char* p){
    std::cerr << "Usage: " << p << " -p <port>\n";
}

int main(int argc, char** argv){
    int port = 5559;
    for (int i=1;i<argc;++i){
        if (std::string(argv[i])=="-p" && i+1<argc) port = std::atoi(argv[++i]);
        else { usage(argv[0]); return 2; }
    }
    signal(SIGINT, on_sigint);

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return 1; }
    int yes=1; setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons((uint16_t)port); addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if (bind(listen_fd,(sockaddr*)&addr,sizeof(addr))<0){ perror("bind"); return 1; }
    if (listen(listen_fd, 128)<0){ perror("listen"); return 1; }

    Pipeline P;
    P.dispatcher.start(&dispatch_handle, &P, "dispatcher");
    P.scc_ao.start   (&scc_handle,    &P, "scc");
    P.ham_ao.start   (&ham_handle,    &P, "ham");
    P.maxclq_ao.start(&maxclq_handle, &P, "maxclique");
    P.numclq_ao.start(&numclq_handle, &P, "nummaxcliques");
    P.responder.start(&respond_handle,&P, "responder");

    std::cout << "Stage9 Pipeline server listening on port " << port
              << " (Active Objects: dispatcher + 4 algos + responder)\n";

    // single acceptor (can be extended to multiple if you like)
    while (running.load()) {
        sockaddr_in cli{}; socklen_t cl = sizeof(cli);
        int cfd = accept(listen_fd, (sockaddr*)&cli, &cl);
        if (cfd < 0) {
            if (!running.load()) break;
            continue;
        }
        // Read single-line request, then possibly m edge lines (GRAPH)
        std::string first;
        if (!read_line(cfd, first)) { close(cfd); continue; }

        Request r;
        if (!parse_and_build(cfd, first, r)) {
            // parse function already sent error line
            close(cfd);
            continue;
        }
        // push into pipeline at the dispatcher
        P.dispatcher.post(std::move(r));
        // responder will close cfd
    }

    // graceful stop
    P.dispatcher.stop();
    P.scc_ao.stop();
    P.ham_ao.stop();
    P.maxclq_ao.stop();
    P.numclq_ao.stop();
    P.responder.stop();

    return 0;
}
