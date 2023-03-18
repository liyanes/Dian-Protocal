#include "base.hpp"
#include "server.hpp"

using namespace std;
using namespace dian;
using namespace dian::rtsp;

RtspURI RtspRequest::getURI(){
    RtspURI uri;
    const std::string prefix = "diantp://";
    if (!url.starts_with(prefix)){
        throw exception("invalid url");
    }
    auto rest = url.substr(prefix.size());
    auto pos = min(rest.find('/'), rest.find(':'));
    if (pos == string::npos){
        throw exception("invalid url");
    }
    uri.host = rest.substr(0, pos);
    if (rest[pos] == ':'){
        auto pathpos = rest.find('/');
        if (pathpos == string::npos){
            throw exception("invalid url");
        }
        uri.port = stoi(rest.substr(pos + 1, pathpos - pos - 1));
        uri.path = rest.substr(pathpos);
    }else {
        uri.port = 554;
        uri.path = rest.substr(pos);
    }
    return uri;
}

RtspServer::RtspServer(const char* addr, unsigned short port):socket_(AF_INET, SOCK_STREAM, IPPROTO_TCP) {
    socket_.listen(addr, port);
}

RtspServer::~RtspServer() {
    socket_.close();
}

void RtspServer::start(unsigned thread_num){
    for (unsigned i = 0; i < thread_num; i++){
        threads.push_back(std::thread([this](int seed){
            srand(seed);
            while (true){
                dian::socket::socket client = socket_.accept();
#ifdef DIAN_DEBUG
                cout << "[Info] New connection <id=" << client.sysid() << ">" << endl;
#endif
                while (true){
                    RtspRequest request(&client);
                    try {
                        request.parseThis(client);
                    } catch (dian::socket::RecvError e){
                        if (e.errcode == 10054) {
                            // 远程主机关闭了连接
#ifdef DIAN_DEBUG
                            cout << "[Info] Socket <id=" << client.sysid() << "> closed by remote host" << endl;
#endif
                            break;
                        }
                    } catch (...){
                        RtspResponse response(&request);
                        response.status = 400;
                        response.reason = "Bad Request";
                        response.send();
                        response.source->socket->close();
#ifdef DIAN_DEBUG
                        cout << "[Info] Socket <id=" << client.sysid() << "> closed,for bad request" << endl;
#endif
                        break;
                    }
                    request.body = new SocketStreamReader(&client);
                    RtspResponse response(&request);
                    
                    try{
                        this->_handle(request, response, client);
                    }
                    catch (dian::socket::SendError e) {
                        if (e.errcode == 10054) {
                            // 远程主机关闭了连接
#ifdef DIAN_DEBUG
                            cout << "[Info] Socket <id=" << client.sysid() << "> closed by remote host" << endl;
#endif
                            break;
                        }
                    } catch (...) {
                        response.status = 500;
                        response.reason = "Internal Server Error";
                        response.send();
                        response.source->socket->close(); 
#ifdef DIAN_DEBUG
                        cout << "[info] Socket <id=" << client.sysid() << "> closed,for server crack." << endl;
#endif
                        break;
                    };
                }
            }
        },rand()));
    }
    for (auto& thread : threads){
        thread.join();
    }
}

void RtspServer::_handle(RtspRequest& request, RtspResponse& response, dian::socket::socket& client) {
    // 处理RTSP请求
    if (request.method == "OPTIONS"){
        response.status = 200;
        response.reason = "OK";
        response.headers["Public"] = "SETUP, TEARDOWN, PLAY, OPTIONS";
        if (response.headers.find("Content-Length") != response.headers.end()){
            response.status = 400;
            response.reason = "Bad Request";
            response.send();
            return;
        }
        response.send();
#ifdef DIAN_DEBUG
        cout << "OPTIONS 200 OK" << endl;
#endif
        return;
    }else if (request.method == "SETUP"){
        auto uri = request.getURI();
        Session session;
        // 生成唯一id
        sessions_lock.lock();
        while (true) {
            session.id = std::to_string(rand()) + std::to_string(rand());
            if (sessions.find(session.id) == sessions.end()) {
                sessions_lock.unlock();
				break;
			}
            cout << "Generate Session ID Failed:" << session.id << endl;
        }
        session.url = request.url;
        session.ip = uri.host;
        session.port = uri.port;
        session.path = uri.path;
        if (request.headers.find("User-Agent") == request.headers.end()){
            if (this->enable_auth){
                response.status = 401;
                response.reason = "Unauthorized";
                response.send();
                return;
            }
            session.user = "";
            session.password = "";
        } else {
            session.user = request.headers["User-Agent"];
            session.password = request.headers["Authorization"];
            if (this->enable_auth && !(*this->authCallback)(session.user, session.password)){
                response.status = 401;
                response.reason = "Unauthorized";
                response.send();
                return;
            }
        }
        session.transport = request.headers["Transport"];
        if (session.transport != "TCP"){
            throw exception("unsupported transport");
        }
        session.contentlength = request.headers["Content-Length"];
        if (this->setupCallback != nullptr){
            (*this->setupCallback)(request,response,*this,&session);
        }
        sessions_lock.lock();
        sessions[session.id] = session;
#ifdef DIAN_DEBUG
        cout << "Setuped.Add Session:" << session.id << endl;
#endif
        sessions_lock.unlock();
        if (!response.has_sended()){
            response.status = 200;
            response.reason = "OK";
            response.headers["Transport"] = session.transport;
            response.headers["Session"] = session.id;
            // response.headers["Content-Length"] = "0";
            response.send();
        }
        return;
    }else if (request.method == "TEARDOWN"){
        auto session_id = request.headers["Session"];
        sessions_lock.lock();
        if (sessions.find(request.headers["Session"]) == sessions.end()){
            response.status = 454;
            response.reason = "Session Not Found";
            response.send();
            sessions_lock.unlock();
            return;
        }
        auto& session = sessions[request.headers["Session"]];
        sessions_lock.unlock();
        if (this->teardownCallback != nullptr){
            (*this->teardownCallback)(request,response,*this,&session);
        }
        if (!response.has_sended()){
            response.status = 200;
            response.reason = "OK";
            response.headers["Session"] = request.headers["Session"];
            // response.headers["Content-Length"] = "0";
            response.send();
        }
        sessions_lock.lock();
        sessions.erase(request.headers["Session"]);
#ifdef DIAN_DEBUG
        cout << "Teardowned.Remove Session:" << request.headers["Session"] << endl;
#endif
        sessions_lock.unlock();
        return;
    }else if (request.method == "PLAY"){
        sessions_lock.lock();
        if (sessions.find(request.headers["Session"]) == sessions.end()){
            response.status = 454;
            response.reason = "Session Not Found";
            response.send();
            sessions_lock.unlock();
            return;
        }
        sessions_lock.unlock();
        if (request.headers.find("CSeq") == request.headers.end()){
            response.status = 400;
            response.reason = "Bad Request";
            response.send();
            return;
        }
        sessions_lock.lock();
        auto& session = sessions[request.headers["Session"]];
        sessions_lock.unlock();
        response.headers["CSeq"] = request.headers["CSeq"];
        response.headers["Session"] = request.headers["Session"];
        if (this->playCallback != nullptr){
            (*this->playCallback)(request,response,*this,&session);
        }
        if (!response.has_sended()){
            response.status = 200;
            response.reason = "OK";
            response.headers["Session"] = request.headers["Session"];
            response.headers["Range"] = "npt=0.000-";
            response.headers["RTP-Info"] = string("url=") + request.url;
            // response.headers["Content-Length"] = "0";
            response.send();
        }
    }else {
        response.status = 405;
        response.reason = "Method Not Allowed";
        response.headers["Public"] = "SETUP, TEARDOWN, PLAY, OPTIONS";
        response.send();
        return;
    }
}
