#include "server.hpp"
#include <map>
#include <fstream>

using namespace std;
using namespace dian;
using namespace dian::rtsp;

int main(){
    WSADATA lpwsadata;
    WSAStartup(MAKEWORD(2, 2), &lpwsadata);
    char host[] = "127.0.0.1";
    RtspServer server(host, 554);
    server.enable_auth = true;

    map<string,string> auths{
        {"user","password"}
    };

    auto acallback = (std::function<bool (std::string, std::string)>)
    [&auths](string user,string password) -> bool{
        if (auths.find(user) == auths.end()){
            return false;
        }
        return auths[user] == password;
    };
    server.authCallback = &acallback;

    auto pcallback = (dian::rtsp::RequestCallback)
    [](Request& request,Response& response,RtspServer& server,Session* session) -> void{
        auto uri = RtspRequest(request).getURI();
        
        // 判断文件存在
        // 暂时不做路径合法检测
        std::ifstream file = ifstream(string(string("./testfile/" + uri.path)));
        if (!file.good()){
            response.status = 404;
            response.reason = "Not Found";
            response.send();
            return;
        }
        FileStreamReader fsr(string("./testfile/") + uri.path,std::move(file));
        response.body = &fsr;
        response.status = 200;
        response.reason = "OK";
        response.send();
    };
    
    server.playCallback = &pcallback;

    server.start(1);
    WSACleanup();
    return 0;
}
