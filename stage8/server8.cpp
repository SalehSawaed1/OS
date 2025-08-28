#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <random>
#include <unordered_set>
#include <algorithm>
#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include "algo.hpp"   // from ../stage7
#include "graph.hpp"   // from ../stage1

// ========== tiny socket helpers ==========
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
    size_t left = s.size(); const char* p = s.c_str();
    while (left) { ssize_t w = send(fd, p, left, 0); if (w <= 0) return false; p += w; left -= w; }
    return send(fd, "\n", 1, 0) == 1;
}

// ========== G(n,m) generator (same mapping as stage7) ==========
static inline std::pair<int,int> id_to_pair_directed(std::size_t n, unsigned long long id){
    unsigned long long row=id/(n-1), col=id%(n-1);
    int u=(int)row, v=(int)col; if (v>=u) ++v; return {u,v};
}
static inline std::pair<int,int> id_to_pair_undirected(std::size_t n, unsigned long long id){
    unsigned long long u=0, rem=id, len=n-1;
    while (rem>=len){ rem-=len; ++u; --len; }
    unsigned long long v=u+1+rem; return {(int)u,(int)v};
}
static std::vector<unsigned long long> sample_ids(unsigned long long N, unsigned long long m, std::mt19937& rng){
    std::unordered_set<unsigned long long> S; S.reserve((size_t)m*2+16);
    std::uniform_int_distribution<unsigned long long> dist;
    for (unsigned long long j=N-m; j<N; ++j){
        unsigned long long t = dist(rng, decltype(dist)::param_type(0, j));
        if (!S.insert(t).second) S.insert(j);
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

// ========== request handling (same protocol as stage7) ==========
static void handle_request_line(int cfd, const std::string& line){
    // Syntax:
    // ALG <NAME> RANDOM n=.. m=.. seed=.. directed=0|1 [limit=..] [timeout_ms=..] [step_limit=..]
    // ALG <NAME> GRAPH  n=.. directed=0|1 m=.. [limit=..] [timeout_ms=..] [step_limit=..]  + m lines "u v"
    std::vector<std::string> tok; tok.reserve(16);
    { std::string cur; for(char ch: line){ if(ch==' '||ch=='\t'){ if(!cur.empty()){ tok.push_back(cur); cur.clear(); } } else cur.push_back(ch); } if(!cur.empty()) tok.push_back(cur); }

    if (tok.size() < 3 || tok[0] != "ALG"){ send_line(cfd, "ERR expected 'ALG <NAME> <MODE>'"); return; }
    std::string alg = tok[1], mode = tok[2];
    KV params = kv_from_tokens(std::vector<std::string>(tok.begin()+3, tok.end()));

    std::size_t n=0, m=0; unsigned seed=0; int directed=0;

    if (mode == "RANDOM"){
        if (!kv_get_size_t(params, "n", n)) { send_line(cfd, "ERR missing n"); return; }
        if (!kv_get_size_t(params, "m", m)) { send_line(cfd, "ERR missing m"); return; }
        kv_get_uint(params, "seed",   seed);
        kv_get_int (params, "directed", directed);

        Graph g(n, directed!=0);
        generate_Gnm(g, m, seed);

        std::unique_ptr<IAlgorithm> A(make_algorithm(alg));
        if (!A) { send_line(cfd, "ERR unknown algorithm"); return; }
        auto res = A->run(g, params);
        send_line(cfd, std::string("OK ")+alg+" "+res.text);
        return;
    }
    else if (mode == "GRAPH"){
        if (!kv_get_size_t(params, "n", n)) { send_line(cfd, "ERR missing n"); return; }
        if (!kv_get_size_t(params, "m", m)) { send_line(cfd, "ERR missing m"); return; }
        kv_get_int(params, "directed", directed);

        Graph g(n, directed!=0);
        for (std::size_t i=0;i<m;++i){
            std::string el; if (!read_line(cfd, el)) { send_line(cfd, "ERR premature end while reading edges"); return; }
            int u=-1, v=-1; if (std::sscanf(el.c_str(), "%d %d", &u, &v) != 2) { send_line(cfd, "ERR bad edge format"); return; }
            g.add_edge(u, v);
        }
        std::unique_ptr<IAlgorithm> A(make_algorithm(alg));
        if (!A) { send_line(cfd, "ERR unknown algorithm"); return; }
        auto res = A->run(g, params);
        send_line(cfd, std::string("OK ")+alg+" "+res.text);
        return;
    }
    else {
        send_line(cfd, "ERR mode must be RANDOM or GRAPH");
        return;
    }
}

// ========== Leader–Follower thread pool ==========
static std::atomic<bool> running(true);
static int listen_fd = -1;

struct LF {
    std::mutex m;
    std::condition_variable cv;
    int leader_id = -1;         // which worker is the leader
    int threads   = 0;
};

static void worker_loop(LF* lf, int id){
    while (running.load(std::memory_order_relaxed)) {
        // become leader
        {
            std::unique_lock<std::mutex> lk(lf->m);
            if (lf->leader_id == -1) lf->leader_id = id;
            lf->cv.wait(lk, [&]{ return !running.load() || lf->leader_id == id; });
            if (!running.load()) return;
        }

        // leader blocks in accept()
        sockaddr_in cli{}; socklen_t cl = sizeof(cli);
        int cfd = accept(listen_fd, (sockaddr*)&cli, &cl);
        if (cfd < 0) {
            if (!running.load()) return;
            // transient error (EINTR/EAGAIN/etc). yield leader role to another and retry
            std::unique_lock<std::mutex> lk(lf->m);
            lf->leader_id = (id + 1) % lf->threads;
            lf->cv.notify_all();
            lk.unlock();
            std::this_thread::yield();
            continue;
        }

        // promote a follower to leader BEFORE handling the client
        {
            std::unique_lock<std::mutex> lk(lf->m);
            lf->leader_id = (id + 1) % lf->threads;
            lf->cv.notify_all();
        }

        // Handle client (single request per connection)
        std::string line;
        if (read_line(cfd, line)) {
            handle_request_line(cfd, line);
        }
        close(cfd);
        // loop back; this worker will become leader again in turn
    }
}

static void usage(const char* p){
    std::cerr << "Usage: " << p << " -p <port> [-t <threads>]\n";
}

static void sigint_handler(int){ running.store(false); if (listen_fd>=0) close(listen_fd); }

int main(int argc, char** argv){
    int port = 5558;
    int nthreads = std::max(2, (int)std::thread::hardware_concurrency()); // default
    for (int i=1;i<argc;++i){
        if (std::string(argv[i])=="-p" && i+1<argc) port = std::atoi(argv[++i]);
        else if (std::string(argv[i])=="-t" && i+1<argc) nthreads = std::max(1, std::atoi(argv[++i]));
        else { usage(argv[0]); return 2; }
    }

    signal(SIGINT, sigint_handler);

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) { perror("socket"); return 1; }
    int yes=1; setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons((uint16_t)port); addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(listen_fd, 128) < 0) { perror("listen"); return 1; }

    std::cout << "Stage8 Leader–Follower server on port " << port
              << " with " << nthreads << " threads. Ctrl+C to stop.\n";

    LF lf; lf.threads = nthreads;
    std::vector<std::thread> pool;
    pool.reserve(nthreads);
    for (int i=0;i<nthreads;++i) pool.emplace_back(worker_loop, &lf, i);

    for (auto& th : pool) th.join();
    return 0;
}
