#include "aetherwild/runtime.hpp"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <cerrno>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using Socket=SOCKET;
static void closeSocket(Socket s) {
    closesocket(s);
}
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
using Socket=int;
static void closeSocket(Socket s) {
    close(s);
}
#endif
using namespace aw;
namespace {
    volatile std::sig_atomic_t stopRequested=0;
    void requestStop(int){stopRequested=1;}
    struct Request {
        std::string method,path,body;
        std::map<std::string,std::string> headers;
    };
    std::string lower(std::string s) {
        for(char&c:s)if(c>='A'&&c<='Z')c+=32;
        return s;
    }
    bool nonblocking(Socket socket) {
#ifdef _WIN32
        u_long enabled=1;
        return ioctlsocket(socket,FIONBIO,&enabled)==0;
#else
        const int flags=fcntl(socket,F_GETFL,0);
        return flags>=0&&fcntl(socket,F_SETFL,flags|O_NONBLOCK)==0;
#endif
    }
    bool wouldBlock() {
#ifdef _WIN32
        const auto e=WSAGetLastError();
        return e==WSAEWOULDBLOCK||e==WSAEINTR;
#else
        return errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR;
#endif
    }
    std::optional<Request> receive(const std::string& data) {
        const auto end=data.find("\r\n\r\n");
        if(end==std::string::npos) {
            require(data.size()<32768,"標頭過大");
            return std::nullopt;
        }
        require(end<32768,"標頭過大");
        Request r;
        std::istringstream in(data.substr(0,end));
        std::string version;
        in>>r.method>>r.path>>version;
        require(r.path.size()<2048&&(r.method=="GET"||r.method=="POST"),"請求方法錯誤");
        std::string line;
        std::getline(in,line);
        while(std::getline(in,line)) {
            auto p=line.find(':');
            if(p==std::string::npos)continue;
            auto key=lower(line.substr(0,p));
            auto value=line.substr(p+1);
            while(!value.empty()&&(value.front()==' '||value.front()=='\t'))value.erase(value.begin());
            if(!value.empty()&&value.back()=='\r')value.pop_back();
            r.headers[key]=value;
        }
        std::size_t length=0;
        if(r.headers.contains("content-length")) {
            auto s=r.headers.at("content-length");
            require(!s.empty()&&s.find_first_not_of("0123456789")==std::string::npos,"長度標頭錯誤");
            length=std::stoull(s);
        }
        require(length<=16384,"內容過大");
        r.body=data.substr(end+4);
        if(r.body.size()<length)return std::nullopt;
        r.body.resize(length);
        auto q=r.path.find('?');
        if(q!=std::string::npos)r.path.resize(q);
        return r;
    }
    void response(std::string& wire,int code,const std::string&mime,const std::string&body) {
        wire="HTTP/1.1 "+std::to_string(code)+(code==200?" OK":code==401?" Unauthorized":" Bad Request")+"\r\nContent-Type: "+mime+"\r\nContent-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\nCache-Control: no-store\r\n\r\n"+body;
    }
    struct Client {
        Socket socket;
        std::string input,output;
        std::size_t sent=0;
        std::chrono::steady_clock::time_point progress;
    };
    std::string contentType(const std::filesystem::path&p) {
        auto e=p.extension().string();
        return e==".html"?"text/html; charset=utf-8":e==".js"?"text/javascript; charset=utf-8":e==".css"?"text/css; charset=utf-8":e==".svg"?"image/svg+xml":e==".webp"?"image/webp":e==".jpg"?"image/jpeg":e==".png"?"image/png":"application/octet-stream";
    }
}
int main(int argc,char**argv) {
    try {
#ifdef _WIN32
        WSADATA wsa;
        require(WSAStartup(MAKEWORD(2,2),&wsa)==0,"Winsock無法啟動");
#endif
        int port=std::getenv("PORT")?std::atoi(std::getenv("PORT")):4173;
        std::filesystem::path root=std::filesystem::current_path();
        for(int i=1;i<argc;i++) {
            std::string a=argv[i];
            if(a=="--port"&&i+1<argc)port=std::atoi(argv[++i]);
            else if(a=="--root"&&i+1<argc)root=argv[++i];
        }
        require(port>0&&port<=65535,"埠號錯誤");
        auto dataDir=std::getenv("AETHERWILD_DATA_DIR");
        GameRuntime runtime((dataDir?std::filesystem::path(dataDir):root/"runtime-data")/"world-v2.json");
        std::signal(SIGTERM,requestStop);std::signal(SIGINT,requestStop);
        auto server=socket(AF_INET,SOCK_STREAM,0);
        require(server!=static_cast<Socket>(-1),"無法建立監聽");
        int reuse=1;
        setsockopt(server,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&reuse),sizeof(reuse));
        sockaddr_in addr {};
        addr.sin_family=AF_INET;
        addr.sin_port=htons(static_cast<unsigned short>(port));
        addr.sin_addr.s_addr=htonl(INADDR_ANY);
        require(bind(server,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))==0,"工作站埠已被使用");
        require(listen(server,32)==0,"無法開始監聽");
        require(nonblocking(server),"無法設定非阻塞監聽");
        std::cout<<"Aetherwild C++ workstation ready\n  Local: http://localhost:"<<port<<"/\n"<<std::flush;
        auto last=std::chrono::steady_clock::now();
        double pending=0;
        const auto syncEpoch=std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        std::uint64_t syncSequence=0;
        auto handle=[&](Request request) {
            std::string wire;
            try {
                if(request.path.starts_with("/api/")) {
                    Json result;
                    if(request.path=="/api/health")result=Json {
                        {
                            "ok",true
                        },
                        {
                            "engine","C++20 native"
                        },
                        {
                            "profiles",runtime.profileCount()
                        },
                        {"inviteRequired",bool(std::getenv("GAME_OWNER_CODE")||std::getenv("GAME_INVITE_CODE"))
                        },
                        {"cloudTest",bool(std::getenv("AETHERWILD_PUBLIC")&&std::string(std::getenv("AETHERWILD_PUBLIC"))=="1")
                        }
                    };
                    else if(request.method=="GET"&&request.path=="/api/catalog")result=runtime.catalog();
                    else {
                        auto auth=request.headers.contains("authorization")?request.headers.at("authorization"):"";
                        auto token=auth.starts_with("Bearer ")?auth.substr(7):"";
                        if(request.method=="GET"&&request.path=="/api/state")result=runtime.snapshot(token);
                        else {
                            require(request.method=="POST"&&request.headers["x-aetherwild-client"]=="1","不接受跨站操作");
                            require(request.headers["content-type"].starts_with("application/json"),"需要JSON內容");
                            auto body=Json::parse(request.body);
                            if(request.path=="/api/session")result=runtime.createSession(body);
                            else if(request.path=="/api/input") {
                                runtime.input(token,body);
                                result=Json {
                                    {
                                        "ok",true
                                    }
                                };
                                if(body.flag("snapshot"))result=runtime.snapshot(token);
                            }
                            else if(request.path=="/api/command")result=runtime.command(token,body);
                            else throw RuleError("不存在的API");
                        }
                    }
                    if(result.contains("player"))result["_sync"]=Json{{"epoch",syncEpoch},{"sequence",++syncSequence}};
                    response(wire,200,"application/json; charset=utf-8",result.dump());
                }
                else {
                    require(request.method=="GET"&&request.path.find("..") ==std::string::npos&&request.path.find('%')==std::string::npos&&request.path.find('\\')==std::string::npos,"路徑錯誤");
                    auto base=request.path.starts_with("/art/")?root/"assets/reference":root/"web";
                    auto relative=request.path=="/"?"index.html":request.path.starts_with("/art/")?request.path.substr(5):request.path.substr(1);
                    auto path=base/relative;
                    require(std::filesystem::is_regular_file(path),"檔案不存在");
                    std::ifstream file(path,std::ios::binary);
                    const auto size=std::filesystem::file_size(path);
                    require(size<=16*1024*1024,"靜態資產過大");
                    std::string bytes(static_cast<std::size_t>(size),'\0');
                    file.read(bytes.data(),static_cast<std::streamsize>(size));
                    require(file.gcount()==static_cast<std::streamsize>(size),"無法完整讀取資產");
                    response(wire,200,contentType(path),bytes);
                }
            }
            catch(const RuleError&e) {
                auto text=std::string(e.what());
                response(wire,text.find("登入")!=std::string::npos?401:400,"application/json; charset=utf-8",Json {
                    {
                        "ok",false
                    },
                    {
                        "error",text
                    }
                }
                .dump());
            }
            catch(const std::exception&e) {
                response(wire,400,"application/json; charset=utf-8",Json {
                    {
                        "ok",false
                    },
                    {
                        "error",std::string(e.what())
                    }
                }
                .dump());
            }
            return wire;
        };
        std::vector<Client> clients;
        while(!stopRequested) {
            auto now=std::chrono::steady_clock::now();
            pending+=std::min(.25,std::chrono::duration<double>(now-last).count());
            last=now;
            while(pending>=.05) {
                runtime.tick(.05);
                pending-=.05;
            }
            fd_set set;
            FD_ZERO(&set);FD_SET(server,&set);
            timeval wait {0,5000};
            if(select(static_cast<int>(server)+1,&set,nullptr,nullptr,&wait)>0) {
                for(int accepted=0;accepted<8;accepted++) {
                    auto socket=accept(server,nullptr,nullptr);
                    if(socket==static_cast<Socket>(-1))break;
                    if(clients.size()>=48||!nonblocking(socket)){closeSocket(socket);continue;}
                    clients.push_back({socket,{}, {},0,now});
                }
            }
            for(auto it=clients.begin();it!=clients.end();) {
                auto& client=*it;
                bool closed=false;
                if(client.output.empty()) {
                    try {
                        char buffer[8192];
                        // A bounded amount per client keeps simulation and other sockets fair.
                        for(int reads=0;reads<4;reads++) {
                            const int n=recv(client.socket,buffer,sizeof(buffer),0);
                            if(n>0) {
                                client.input.append(buffer,n);client.progress=now;
                                require(client.input.size()<=49152,"請求過大");
                                if(auto request=receive(client.input)) {
                                    client.output=handle(std::move(*request));
                                    client.input.clear();break;
                                }
                            }else {
                                if(n==0||!wouldBlock())closed=true;
                                break;
                            }
                        }
                    }catch(const std::exception& e) {
                        response(client.output,400,"application/json; charset=utf-8",Json{{"ok",false},{"error",e.what()}}.dump());
                    }
                }
                if(!closed&&!client.output.empty()) {
                    const auto remaining=client.output.size()-client.sent;
                    const int n=send(client.socket,client.output.data()+client.sent,static_cast<int>(std::min<std::size_t>(65536,remaining)),
#ifdef MSG_NOSIGNAL
                        MSG_NOSIGNAL
#else
                        0
#endif
                    );
                    if(n>0){client.sent+=n;client.progress=now;closed=client.sent==client.output.size();}
                    else if(n==0||!wouldBlock())closed=true;
                }
                const double idle=std::chrono::duration<double>(now-client.progress).count();
                if(idle>(client.output.empty()?2.0:15.0))closed=true;
                if(closed){closeSocket(client.socket);it=clients.erase(it);}else ++it;
            }
        }
        for(auto& client:clients)closeSocket(client.socket);
        runtime.save();closeSocket(server);
    }
    catch(const std::exception&e) {
        std::cerr<<"Workstation failed: "<<e.what()<<'\n';
        return 1;
    }
}
