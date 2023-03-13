#include "base.h"

using namespace std;
using namespace dian;
using namespace dian::socket;
using namespace dian::rtsp;

dian::socket::socket::socket(int af, int type, int protocol) {
    socketvalue = ::socket(af, type, protocol);
}

dian::socket::socket::socket(SOCKET socketvalue) {
    this->socketvalue = socketvalue;
}

dian::socket::socket::~socket() {
    closesocket(socketvalue);
}

void dian::socket::socket::listen(const std::string& host, unsigned short port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.S_un.S_addr = inet_addr(host.c_str());
    if (SOCKET_ERROR == ::bind(socketvalue, (sockaddr*)&addr, sizeof(addr))){
        throw BindError("bind error");
    }
    if (SOCKET_ERROR == ::listen(socketvalue, 5)){
        throw ListenError("listen error");
    }
}

dian::socket::socket dian::socket::socket::accept() {
    sockaddr_in addr;
    int len = sizeof(addr);
    SOCKET socketvalue = ::accept(this->socketvalue, (sockaddr*)&addr, &len);
    if (socketvalue == INVALID_SOCKET){
        throw AcceptError("accept error");
    }
    return dian::socket::socket(socketvalue);
}

void dian::socket::socket::connect(const std::string& host, unsigned short port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.S_un.S_addr = inet_addr(host.c_str());
    if (SOCKET_ERROR == ::connect(socketvalue, (sockaddr*)&addr, sizeof(addr))){
        throw ConnectError("connect error");
    }
}

void dian::socket::socket::send(const buffer& data) {
    if (SOCKET_ERROR == ::send(socketvalue, (const char*)data.data(), ((buffer)data).size(), 0)){
        throw SendError("send error");
    }
}

buffer dian::socket::socket::recv(size_t length) {
    buffer data(length);
    if (SOCKET_ERROR == ::recv(socketvalue, (char*)data.data(), (const int)data.size(), 0)){
        throw RecvError("recv error");
    }
    return data;
}

void dian::socket::socket::close() {
    closesocket(socketvalue);
    this->closed = true;
}

void dian::socket::socket::shutdown(int how) {
    ::shutdown(socketvalue, how);
}

buffer dian::socket::socket::recvLine(int max){
    buffer data(max);
    int i = 0;
    while (true){
        if (SOCKET_ERROR == ::recv(socketvalue, (char*)data.data() + i, 1, 0)){
            throw RecvError("recv error");
        }
        if (((char*)data.data())[i] == '\r'){
            if (SOCKET_ERROR == ::recv(socketvalue, (char*)data.data() + i + 1, 1, 0)){
                throw RecvError("recv error");
            }
            if (((char*)data.data())[i + 1] == '\n'){
                break;
            }
        }
        i++;
    }
    return buffer(data.data(), i);
}

buffer dian::socket::socket::recvUntil(const std::string& str, int max){
    buffer data(max);
    int i = 0;
    while (true){
        if (SOCKET_ERROR == ::recv(socketvalue, (char*)data.data() + i, 1, 0)){
            throw RecvError("recv error");
        }
        if (((char*)data.data())[i] == str[0]){
            if (SOCKET_ERROR == ::recv(socketvalue, (char*)data.data() + i + 1, str.size() - 1, 0)){
                throw RecvError("recv error");
            }
            if (memcmp((char*)data.data() + i, str.c_str(), str.size()) == 0){
                break;
            }
        }
        i++;
    }
    return buffer(data.data(), i);
}

Base Base::parse(socket::socket &socket) {
    Base base;
    buffer line = socket.recvUntil("\r\n");
    char version[10];
    sscanf("%s %s %[^\r]\r\n", (char*)line.data(), base.method, base.url, version);
    int major, minor;
    sscanf("%d.%d",version, &major, &minor);
    base.version.major = major;
    base.version.minor = minor;
    while (true){
        line = socket.recvUntil("\r\n");
        if (line.size() == 0){
            break;
        }
        if (strcmp((char*)line.data(),"\r\n") == 0){
            break;
        }
        char key[100];
        char value[100];
        sscanf("%[^:]:%[^\r]\r\n", (char*)line.data(), key, value);
        base.headers[key] = value;
    }
    base.body = new SocketStreamReader(&socket);
    return base;
}

void Base::parseThis(socket::socket &socket) {
    buffer line = socket.recvUntil("\r\n");
    char version[10];
    sscanf("%s %s %[^\r]\r\n", (char*)line.data(), method, url, version);
    int major, minor;
    sscanf("%d.%d",version, &major, &minor);
    this->version.major = major;
    this->version.minor = minor;
    while (true){
        line = socket.recvUntil("\r\n");
        if (line.size() == 0){
            break;
        }
        if (strcmp((char*)line.data(),"\r\n") == 0){
            break;
        }
        char key[100];
        char value[100];
        sscanf("%[^:]:%[^\r]\r\n", (char*)line.data(), key, value);
        headers[key] = value;
    }
    body = new SocketStreamReader(&socket);
}

Request Request::parse(dian::socket::socket &socket) {
    Request request;
    request.parseThis(socket);
    request.socket = &socket;
    return request;
}

void bufferStream::resize(size_t size){
    if (size != _buffer.size()){
        auto new_buffer = new char[size];
        memcpy(new_buffer, _buffer.data(), min(size,_buffer.size()));
        delete[] _buffer.data();
        _buffer._buffer = new_buffer;
        _buffer._size = size;
    }
}

void bufferStream::operator<<(const buffer& data){
    if (_buffer.size() - ((char*)pos - _buffer.data()) < data.size()){
        resize(_buffer.size() + data.size());
    }
    memcpy(pos, data.data(), data.size());
    this->pos = (void*)((char*)pos + data.size());
}

void bufferStream::operator<<(const char* data){
    if (_buffer.size() - ((char*)pos - _buffer.data()) < strlen(data)){
        resize(_buffer.size() + strlen(data));
    }
    memcpy(pos, data, strlen(data));
    this->pos = (void*)((char*)pos + strlen(data));
}

buffer bufferStream::to_buffer(){
    return this->_buffer;
}

buffer Request::unparseHead(){
    bufferStream _stream(1024);
    _stream << (std::string)method;
    _stream << " ";
    _stream << url;
    _stream << " HTTP/";
    _stream << version.major;
    _stream << ".";
    _stream << version.minor;
    _stream << "\r\n";

    for (auto& header : headers){
        _stream << header.first;
        _stream << ": ";
        _stream << header.second;
        _stream << "\r\n";
    }
    _stream << "\r\n";
    return _stream.to_buffer();
}

FileStreamReader::FileStreamReader(std::string filepath):
    filepath(filepath) {
    this->file.open(filepath, std::ios::binary);
}

FileStreamReader::~FileStreamReader() {
    this->file.close();
}

buffer FileStreamReader::read(size_t size) {
    buffer data(size);
    this->file.read((char*)data.data(), size);
    return data;
}

buffer FileStreamReader::readLine() {
    std::string line;
    this->file.getline((char*)line.c_str(), line.size(), '\n');
    if (line[line.size() - 1] == '\r'){
        line.pop_back();
    }
    return buffer((const void*)line.c_str(), line.size());
}

buffer FileStreamReader::readUntil(const std::string &str) {
    std::stringstream ss;
    char c;
    int strlen = str.size();
    while (true){
        this->file.get(c);
        ss << c;
        if (str[strlen - 1] == c){
            if (strlen == 1){
                break;
            }
            if (ss.str().ends_with(str)){
                break;
            }
        }
    }
    return buffer(ss.str().c_str(), ss.str().size());
}

StringStreamReader::StringStreamReader(const std::string &&str):
    str(str) {
        stream<<str;
}

buffer StringStreamReader::read(size_t size) {
    buffer data(size);
    stream.read((char*)data.data(), size);
    return data;
}

buffer StringStreamReader::readLine() {
    std::string line;
    stream.getline((char*)line.c_str(), line.size(), '\n');
    if (line[line.size() - 1] == '\r'){
        line.pop_back();
    }
    if (line[line.size() - 1] == '\r'){
        line.pop_back();
    }
    return buffer((const void*)line.c_str(), line.size());
}

buffer StringStreamReader::readUntil(const std::string &str) {
    std::stringstream ss;
    char c;
    int strlen = str.size();
    while (true){
        stream.get(c);
        ss << c;
        if (str[strlen - 1] == c){
            if (strlen == 1){
                break;
            }
            if (ss.str().ends_with(str)){
                break;
            }
        }
    }
    return buffer(ss.str().c_str(), ss.str().size());
}


