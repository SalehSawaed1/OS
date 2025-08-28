#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static void usage(const char* p){
    std::cerr<<"Usage: "<<p<<" -p <port> \"REQUEST\"\n"
             <<"Examples:\n"
             <<"  "<<p<<" -p 5557 \"ALG SCC_COUNT RANDOM n=10 m=20 seed=1 directed=1\"\n"
             <<"  "<<p<<" -p 5557 \"ALG MAXCLIQUE RANDOM n=12 m=20 seed=7 directed=0\"\n"
             <<"  "<<p<<" -p 5557 \"ALG NUM_MAXCLIQUES RANDOM n=12 m=20 seed=7 directed=0\"\n"
             <<"  "<<p<<" -p 5557 \"ALG HAM_CYCLE RANDOM n=12 m=18 seed=3 directed=0 limit=16\"\n";
}

int main(int argc, char** argv){
    int port=5557; std::string req;
    for(int i=1;i<argc;++i){
        if(std::string(argv[i])=="-p" && i+1<argc) port=std::atoi(argv[++i]);
        else req = argv[i];
    }
    if(req.empty()){ usage(argv[0]); return 2; }

    int sfd=socket(AF_INET,SOCK_STREAM,0); if(sfd<0){ perror("socket"); return 1; }
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(port); inet_pton(AF_INET,"127.0.0.1",&a.sin_addr);
    if(connect(sfd,(sockaddr*)&a,sizeof(a))<0){ perror("connect"); return 1; }

    std::string line=req+"\n";
    if(send(sfd,line.c_str(),line.size(),0)<0){ perror("send"); return 1; }

    char buf[8192]; ssize_t r=recv(sfd,buf,sizeof(buf)-1,0);
    if(r>0){ buf[r]='\0'; std::cout<<buf; }
    close(sfd); return 0;
}
