#define _CRT_SECURE_NO_WARNINGS
#include "server.hpp"
#include <map>
#include <fstream>

using namespace std;
using namespace dian;
using namespace dian::rtsp;

int main(){
    WSADATA lpwsadata;
    WSAStartup(MAKEWORD(2, 2), &lpwsadata);
    srand((unsigned)time(NULL));
    char host[] = "127.0.0.1";
    RtspServer server(host, 664);
    /*server.enable_auth = true;

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
    server.authCallback = &acallback;*/

    auto pcallback = (dian::rtsp::RequestCallback)
    [](Request& request,Response& response,RtspServer& server,Session* session) -> void{
        auto uri = RtspRequest(request).getURI();

        // 判断文件存在
        // 暂时不做路径合法检测
        auto filepath = string(string("D:\\Studio\\Dian-Protocol\\src\\cpp\\testfile\\") + (uri.path.c_str() + 1));
#ifdef _WIN32
        char* tmppath = new char[filepath.length() + 1];
        strcpy(tmppath, filepath.c_str());
        for (int i = 0; i < filepath.length(); i++) {
			if (tmppath[i] == '/') tmppath[i] = '\\';
        }
        filepath = string(tmppath);
        delete[] tmppath;
#endif
        std::ifstream file = ifstream(filepath);
        if (!file.good()){
            response.status = 404;
            response.reason = "Not Found";
            response.send();
            return;
        }
        if (request.headers.find("Range") != request.headers.end()){
            // 暂不考虑npt针对时间的精确
            // 不考虑多Range情况
            auto _range = request.headers["Range"];
            if (!request.headers["Range"].starts_with("npt=")){
                response.status = 400;
                response.reason = "Bad Request";
                response.send();
                return;
            }
            const char* cur = request.headers["Range"].c_str() + 4,
                *tmp = cur;
            while (*tmp++ != '-' && *tmp != '\0');
            if (*tmp == '\0'){
                response.status = 400;
                response.reason = "Bad Request";
                response.send();
                return;
            }
            auto start = stoi(string(cur,tmp - 1));
            auto end = *tmp == '\0' ? -1 : stoi(string(tmp));
            if (end < start && end != -1){
                response.status = 400;
                response.reason = "Bad Request";
                response.send();
                return;
            }
            auto fsr = new RangeFileStreamReader(filepath,std::move(file),start,end + 1);
            if (start > fsr->size() || end > fsr->size()){
                response.status = 416;
                response.reason = "Requested Range Not Satisfiable";
                response.send();
                return;
            }
            response.body = std::move(fsr);
            response.headers["Range"] = request.headers["Range"];
            response.headers["Content-Range"] = "bytes " + to_string(start) + "-" + to_string(end == -1 ? fsr->size() : end) + "/" + to_string(fsr->size());
            response.status = 206;
            response.reason = "Partial Content";
            response.send();
            return;
        }
        auto fsr = new FileStreamReader(filepath,std::move(file));
        response.body = std::move(fsr);
        response.status = 200;
        response.reason = "OK";
        response.send();
    };
    
    server.playCallback = &pcallback;

    server.start(8);
    WSACleanup();
    return 0;
}
