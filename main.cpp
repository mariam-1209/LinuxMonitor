#include "metrics.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <cctype>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
using namespace std;
using namespace chrono_literals;
struct Process {string pid,name; long rss;};
struct State {double cpu=-1,ram=-1,swap=-1,disk=-1; vector<Process> processes; size_t count=0;} state;
mutex guard;
void cpu_worker(){unsigned long long a=0,b=0,c,d; bool ready=cpu_ticks(&a,&b); for(;;){this_thread::sleep_for(1s); bool ok=cpu_ticks(&c,&d); double v=-1; if(ok&&ready&&c>a&&d>=b&&d-b<=c-a)v=100.0*(1.0-double(d-b)/(c-a)); {lock_guard<mutex> lock(guard);state.cpu=v;} if(ok){a=c;b=d;} ready=ok;}}
void memory_worker(){for(;;){double r=-1,s=-1;memory_info(&r,&s);{lock_guard<mutex> lock(guard);state.ram=r;state.swap=s;}this_thread::sleep_for(1s);}}
void disk_worker(){for(;;){double d=-1;disk_info(&d);{lock_guard<mutex> lock(guard);state.disk=d;}this_thread::sleep_for(2s);}}
void process_worker(){for(;;){vector<Process> p; error_code ec; for(filesystem::directory_iterator it("/proc",ec),end; !ec&&it!=end;it.increment(ec)){string id=it->path().filename().string(); if(id.empty()||!all_of(id.begin(),id.end(),[](unsigned char c){return isdigit(c);}))continue; ifstream f(it->path()/"status"); if(!f)continue; string line,name;long rss=0;while(getline(f,line)){if(line.rfind("Name:",0)==0){name=line.substr(5);name.erase(0,name.find_first_not_of(" \t"));}else if(line.rfind("VmRSS:",0)==0){istringstream(line.substr(6))>>rss;}}if(!name.empty())p.push_back({id,name,rss});} sort(p.begin(),p.end(),[](auto&a,auto&b){return a.rss>b.rss;});size_t count=p.size();if(p.size()>50)p.resize(50);{lock_guard<mutex> lock(guard);state.processes=move(p);state.count=count;}this_thread::sleep_for(2s);}}
string quote(const string&s){string r="\"";for(unsigned char c:s){if(c=='"'||c=='\\'){r+='\\';r+=c;}else if(c<32||c>=127){char b[7];snprintf(b,sizeof b,"\\u%04x",c);r+=b;}else r+=c;}return r+'"';}
string json(){State s;{lock_guard<mutex> lock(guard);s=state;}ostringstream o;o<<"{\"cpu\":"<<s.cpu<<",\"ram\":"<<s.ram<<",\"swap\":"<<s.swap<<",\"disk\":"<<s.disk<<",\"uptime\":"<<uptime_seconds()<<",\"count\":"<<s.count<<",\"processes\":[";bool first=true;for(auto&p:s.processes){if(!first)o<<',';first=false;o<<"{\"pid\":"<<p.pid<<",\"name\":"<<quote(p.name)<<",\"rss\":"<<p.rss<<'}';}o<<"]}";return o.str();}
int main(){int server=socket(AF_INET,SOCK_STREAM,0);if(server<0){perror("socket");return 1;}int yes=1;setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes);sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_port=htons(8765);addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(server,(sockaddr*)&addr,sizeof addr)<0||listen(server,16)<0){perror("bind/listen");close(server);return 1;}thread(cpu_worker).detach();thread(memory_worker).detach();thread(disk_worker).detach();thread(process_worker).detach();cout<<"Open http://127.0.0.1:8765 (Ctrl+C to stop)"<<endl;for(;;){int fd=accept(server,nullptr,nullptr);if(fd<0)continue;timeval tv{2,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof tv);setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof tv);string req;char buf[2048];while(req.size()<8192&&req.find("\r\n\r\n")==string::npos){ssize_t n=recv(fd,buf,sizeof buf,0);if(n<=0)break;req.append(buf,n);}istringstream in(req);string method,path;in>>method>>path;string body,type="text/plain",status="200 OK";if(method=="GET"&&path=="/api/metrics"){body=json();type="application/json";}else if(method=="GET"&&path=="/"){ifstream f("web/index.html");if(f){body=string(istreambuf_iterator<char>(f),{});type="text/html; charset=utf-8";}else{status="500 Internal Server Error";body="Run from project directory";}}else{status="404 Not Found";body="Not found";}string response="HTTP/1.1 "+status+"\r\nContent-Type: "+type+"\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: "+to_string(body.size())+"\r\n\r\n"+body;size_t sent=0;while(sent<response.size()){ssize_t n=send(fd,response.data()+sent,response.size()-sent,MSG_NOSIGNAL);if(n<=0)break;sent+=n;}close(fd);}}
