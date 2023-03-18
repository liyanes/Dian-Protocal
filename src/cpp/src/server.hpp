#include "base.hpp"
#include <thread>
#include <functional>
#include <vector>
#include <mutex>

#ifdef _DEBUG
#define DIAN_DEBUG
#endif

namespace dian{
    namespace rtsp{
        struct RtspURI {
            std::string host;
            unsigned short port;
            std::string path;
        };
        class RtspRequest: public Request{
        public:
            RtspURI getURI();
        };
        class RtspResponse: public Response{
        public:
            using Response::Response;
        };
        struct Session {
            std::string id;
            std::string url;
            std::string ip;
            unsigned short port;
            std::string path;
            std::string user;
            std::string password;
            // std::string session;
            std::string transport;
            // std::string range;
            // std::string scale;
            // std::string speed;
            // std::string control;
            // std::string rtpinfo;
            // std::string contenttype;
            std::string contentlength;
            // std::string content;
        };
        class RtspServer;
        using RequestCallback = std::function<void(Request&,Response&,RtspServer&,Session*)>;
        class RtspServer{
        protected:
            dian::socket::socket socket_;
            std::vector<std::thread> threads;
            void _handle(RtspRequest& request, RtspResponse& response,dian::socket::socket& client);

            std::map<std::string,Session> sessions;
            std::mutex sessions_lock;
        public:
            bool enable_auth = false;

            RequestCallback *setupCallback = nullptr;
            RequestCallback *playCallback = nullptr;
            RequestCallback *teardownCallback = nullptr;

            /// @brief 验证函数
            /// @param user 用户名
            /// @param password 密码
            std::function<bool(std::string,std::string)> *authCallback = nullptr;

            RtspServer(const char* addr,unsigned short port);
            ~RtspServer();
            void start(unsigned thread_num = 1);
        };
    }
}
