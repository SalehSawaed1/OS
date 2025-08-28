#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " -p <port> \"REQUEST LINE\"\n";
    std::cerr << "Example:\n  " << prog << " -p 5555 \"EULER RANDOM n=8 m=12 seed=42 directed=0\"\n";
}

int main(int argc, char** argv) {
    int port = 5555;
    std::string req;

    for (int i=1; i<argc; ++i) {
        if (std::string(argv[i]) == "-p" && i+1 < argc) {
            port = std::atoi(argv[++i]);
        } else {
            req = argv[i];
        }
    }
    if (req.empty()) { usage(argv[0]); return 2; }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { perror("socket"); return 1; }

    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sfd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("connect"); return 1; }

    std::string line = req + "\n";
    if (send(sfd, line.c_str(), line.size(), 0) < 0) { perror("send"); return 1; }

    char buf[4096];
    ssize_t r = recv(sfd, buf, sizeof(buf)-1, 0);
    if (r > 0) {
        buf[r] = '\0';
        std::cout << buf;
    }
    close(sfd);
    return 0;
}
