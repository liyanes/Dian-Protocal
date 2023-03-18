#define _CRT_SECURE_NO_WARNINGS
#include "base.hpp"
using namespace std;
using namespace dian;
using namespace dian::socket;
using namespace dian::rtsp;

static void trim(string &s)
{
    if (!s.empty())
    {
        s.erase(0, s.find_first_not_of(" "));
        s.erase(s.find_last_not_of(" ") + 1);
    }
}

dian::socket::socket::socket(int af, int type, int protocol)
{
    socketvalue = ::socket(af, type, protocol);
}

dian::socket::socket::socket(SOCKET socketvalue)
{
    this->socketvalue = socketvalue;
}

dian::socket::socket::~socket()
{
    closesocket(socketvalue);
}

void dian::socket::socket::listen(const char *host, unsigned short port)
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.S_un.S_addr = inet_addr(host);
    if (SOCKET_ERROR == ::bind(socketvalue, (sockaddr *)&addr, sizeof(addr)))
    {
        int lasterror = WSAGetLastError();
        throw BindError(string(string("bind error") + to_string(lasterror)).c_str());
    }
    if (SOCKET_ERROR == ::listen(socketvalue, 5))
    {
        throw ListenError("listen error");
    }
}

dian::socket::socket dian::socket::socket::accept()
{
    sockaddr_in addr;
    int len = sizeof(addr);
    SOCKET socketvalue = ::accept(this->socketvalue, (sockaddr *)&addr, &len);
    if (socketvalue == INVALID_SOCKET)
    {
        throw AcceptError("accept error");
    }
    return dian::socket::socket(socketvalue);
}

void dian::socket::socket::connect(const std::string &host, unsigned short port)
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.S_un.S_addr = inet_addr(host.c_str());
    if (SOCKET_ERROR == ::connect(socketvalue, (sockaddr *)&addr, sizeof(addr)))
    {
        throw ConnectError("connect error");
    }
}

void dian::socket::socket::send(const buffer &data)
{
    if (SOCKET_ERROR == ::send(socketvalue, (const char *)data.data(), (int)(const_cast<buffer *>(&data))->size(), 0))
    {
        int error = WSAGetLastError();
        throw SendError(string(string("send error") + to_string(error)).c_str());
    }
}

buffer dian::socket::socket::recv(size_t length)
{
    buffer data(length);
    if (SOCKET_ERROR == ::recv(socketvalue, (char *)data.data(), (const int)data.size(), 0))
    {
        throw RecvError("recv error");
    }
    return data;
}

void dian::socket::socket::close()
{
    closesocket(socketvalue);
    this->closed = true;
}

void dian::socket::socket::shutdown(int how)
{
    ::shutdown(socketvalue, how);
}

buffer dian::socket::socket::recvLine(int max)
{
    return recvUntil("\r\n", max, true);
}

buffer dian::socket::socket::recvUntil(const std::string &str, int max, bool set_text)
{
    bufferStream data(max == -1 ? 1024 : max);
    int recved = 0, strlen_ = str.size();
    while (recved < max || max == -1)
    {
        char value = 0;
        if (SOCKET_ERROR == ::recv(socketvalue, &value, 1, 0))
        {
            throw RecvError("recv error",WSAGetLastError());
        }
        data << value;
        recved++;
        if (memcmp((char *)data.data() + recved - strlen_, str.c_str(), strlen_) == 0)
        {
            break;
        }
    }
    if (set_text)
    {
        data << '\0';
    }
    return data;
}

Base Base::parse(socket::socket &socket)
{
    Base base;
    buffer line = socket.recvUntil("\r\n");
    while (strcmp((char*)line.data(), "\r\n") == 0) {
        // 即只有空行
        line = socket.recvUntil("\r\n");
    }
    char *cur = (char *)line.data(), *tmp = cur;
    while (*tmp++ != ' ' && *tmp != '\0')
        ;
    if (*tmp == '\0')
    {
        throw exception("parse error");
    }
    base.method = string(cur, tmp - cur - 1);
    cur = tmp;
    while (*tmp++ != ' ' && *tmp != '\0')
        ;
    if (*tmp == '\0')
    {
        throw exception("parse error");
    }
    base.url = string(cur, tmp - cur - 1);
    cur = tmp;
    while (*tmp++ != '\r' && *tmp != '\0')
        ;
    if (*tmp == '\0')
    {
        throw exception("parse error");
    }
    string version = string(cur, tmp - cur - 1);

    int major, minor;
    if (sscanf("%d.%d", version.c_str(), &major, &minor) != 2)
    {
        throw exception("parse error");
    }
    base.version.major = major;
    base.version.minor = minor;
    while (true)
    {
        line = socket.recvUntil("\r\n");
        if (line.size() == 0)
        {
            break;
        }
        if (strcmp((char *)line.data(), "\r\n") == 0)
        {
            break;
        }
        string key, value;
        tmp = cur = (char *)line.data();
        while (*tmp++ != ':' && *tmp != '\0')
            ;
        if (*tmp == '\0')
        {
            throw exception("parse error");
        }
        key = string(cur, tmp - cur - 1);
        cur = tmp;
        while (*tmp++ != '\r' && *tmp != '\0')
            ;
        if (*tmp == '\0')
        {
            throw exception("parse error");
        }
        trim(key);
        trim(value);
        base.headers[key] = value;
    }
    base.body = new SocketStreamReader(&socket);
    return base;
}

void Base::parseThis(socket::socket &socket)
{
    buffer line = socket.recvUntil("\r\n");
    while (strcmp((char*)line.data(),"\r\n") == 0) {
        // 即只有空行
        line = socket.recvUntil("\r\n");
    }
    char *cur = (char *)line.data(), *tmp = cur;
    while (*tmp++ != ' ' && *tmp != '\0')
        ;
    if (*tmp == '\0')
    {
        throw exception("parse error");
    }
    this->method = string(cur, tmp - cur - 1);
    cur = tmp;
    while (*tmp++ != ' ' && *tmp != '\0')
        ;
    if (*tmp == '\0')
    {
        throw exception("parse error");
    }
    this->url = string(cur, tmp - cur - 1);
    cur = tmp;
    while (*tmp++ != '\r' && *tmp != '\0')
        ;
    if (*tmp == '\0')
    {
        throw exception("parse error");
    }
    string version = string(cur, tmp - cur - 1);

    int major, minor;
    if (sscanf(version.c_str(),"%d.%d", &major, &minor) != 2)
    {
        throw exception("parse error");
    }
    this->version.major = major;
    this->version.minor = minor;
    while (true)
    {
        line = socket.recvUntil("\r\n");
        if (line.size() == 0)
        {
            break;
        }
        if (strcmp((char *)line.data(), "\r\n") == 0)
        {
            break;
        }
        string key, value;
        tmp = cur = (char *)line.data();
        while (*tmp++ != ':' && *tmp != '\0')
            ;
        if (*tmp == '\0')
        {
            throw exception("parse error");
        }
        key = string(cur, tmp - cur - 1);
        cur = tmp;
        while (*tmp++ != '\r' && *tmp != '\0')
            ;
        if (*tmp == '\0')
        {
            throw exception("parse error");
        }
        value = string(cur, tmp - cur - 1);
        trim(key);
        trim(value);
        this->headers[key] = value;
    }
    this->body = new SocketStreamReader(&socket);
}

Request Request::parse(dian::socket::socket &socket)
{
    Request request;
    request.parseThis(socket);
    return request;
}

buffer Request::unparseHead()
{
    bufferStream _stream(1024);
    _stream << (std::string)method;
    _stream << " ";
    _stream << url;
    _stream << " HTTP/";
    _stream << version.major;
    _stream << ".";
    _stream << version.minor;
    _stream << "\r\n";

    for (auto &header : headers)
    {
        _stream << header.first;
        _stream << ": ";
        _stream << header.second;
        _stream << "\r\n";
    }
    _stream << "\r\n";
    return _stream;
}

FileStreamReader::FileStreamReader(const std::string& filepath) : filepath(filepath),do_free(true)
{
    this->file = new std::ifstream();
    this->file->open(filepath, std::ios::binary);
    this->file->seekg(0, std::ios::end);
    filesize = this->file->tellg();
    this->file->seekg(0, std::ios::beg);
}

FileStreamReader::~FileStreamReader()
{
    this->file->close();
    if (this->do_free)
        delete this->file;
}

buffer FileStreamReader::read(size_t size)
{
    size_t maxsize = filesize - this->file->tellg();
    maxsize = maxsize > size ? size : maxsize;
    buffer data(maxsize);
    this->file->read((char *)data.data(), maxsize);
    return data;
}

buffer FileStreamReader::readLine()
{
    std::string line;
    this->file->getline((char *)line.c_str(), line.size(), '\n');
    if (line[line.size() - 1] == '\r')
    {
        line.pop_back();
    }
    return buffer((const void *)line.c_str(), line.size());
}

buffer FileStreamReader::readUntil(const std::string &str)
{
    std::stringstream ss;
    char c;
    int strlen = str.size();
    while (true)
    {
        this->file->get(c);
        ss << c;
        if (str[strlen - 1] == c)
        {
            if (strlen == 1)
            {
                break;
            }
            if (ss.str().ends_with(str))
            {
                break;
            }
        }
        if (this->file->eof()) {
            break;
        }
    }
    return buffer(ss.str().c_str(), ss.str().size());
}

StringStreamReader::StringStreamReader(const std::string &str) : str(str)
{
    stream << str;
}

buffer StringStreamReader::read(size_t size)
{
    buffer data(size);
    stream.read((char *)data.data(), size);
    return data;
}

buffer StringStreamReader::readLine()
{
    std::string line;
    stream.getline((char *)line.c_str(), line.size(), '\n');
    if (line[line.size() - 1] == '\r')
    {
        line.pop_back();
    }
    if (line[line.size() - 1] == '\r')
    {
        line.pop_back();
    }
    return buffer((const void *)line.c_str(), line.size());
}

buffer StringStreamReader::readUntil(const std::string &str)
{
    std::stringstream ss;
    char c;
    int strlen = (int)str.size();
    while (true)
    {
        stream.get(c);
        ss << c;
        if (str[strlen - 1] == c)
        {
            if (strlen == 1)
            {
                break;
            }
            if (ss.str().ends_with(str))
            {
                break;
            }
        }
    }
    return buffer(ss.str().c_str(), ss.str().size());
}

Response Response::parse(const Request *source, dian::socket::socket socket)
{
    Response response(source);
    response.parseThis(socket);
    return response;
}

buffer Response::unparseHead()
{
    bufferStream _stream(1024);
    _stream << "HTTP/";
    _stream << version.major;
    _stream << ".";
    _stream << version.minor;
    _stream << " ";
    _stream << status;
    _stream << " ";
    _stream << reason;
    _stream << "\r\n";

    for (auto &header : headers)
    {
        _stream << header.first;
        _stream << ": ";
        _stream << header.second;
        _stream << "\r\n";
    }
    _stream << "\r\n";
    return _stream.to_buffer();
}

void Response::send()
{
    if (this->sended)
    {
        throw std::runtime_error("Response already sended");
    }
    if (this->body) {
        size_t con_len = this->body->maxlen();
        if (con_len == -1) {
            throw NotImplementError("Thunked Transport Not Implemented");
        }
        this->headers["Content-Length"] = std::to_string(con_len);
    }
    this->source->socket->send(this->unparseHead());
    if (this->body) this->source->socket->send(*this->body);
    this->sended = true;
}

void dian::socket::socket::send(StreamReader &data)
{
    while (true)
    {
        auto buffer = data.read(1024);
        if (buffer.size() == 0)
        {
            break;
        }
        this->send(buffer);
    }
}

void Response::_apply_res()
{
    this->method = this->source->method;
    this->url = this->source->url;
    this->version = this->source->version;
}

void Request::parseThis(dian::socket::socket &socket)
{
    Base::parseThis(socket);
    this->socket = &socket;
    return;
}

dian::socket::socket::socket(socket &&other) noexcept
{
    this->socketvalue = other.socketvalue;
    other.closed = true;
}

dian::socket::socket &dian::socket::socket::operator=(socket &&other) noexcept
{
    this->socketvalue = other.socketvalue;
    other.closed = true;
    return *this;
}

FileStreamReader::FileStreamReader(const std::string &filepath, std::ifstream &&stream) : filepath(filepath), file(&stream), do_free(false) {
    this->file->seekg(0, std::ios::end);
    filesize = this->file->tellg();
    this->file->seekg(0, std::ios::beg);
}

Request::~Request() {}

SocketStreamReader::SocketStreamReader(dian::socket::socket *socket) : socket(socket) {}

SocketStreamReader::~SocketStreamReader()
{
    this->socket->close();
}

buffer SocketStreamReader::read(size_t size)
{
    buffer data(size);
    this->socket->recv(size);
    return data;
}

buffer SocketStreamReader::readLine()
{
    std::string line;
    line = (char *)this->socket->recvLine().data();
    if (line[line.size() - 1] == '\r')
    {
        line.pop_back();
    }
    return buffer((const void *)line.c_str(), line.size());
}

buffer SocketStreamReader::readUntil(const std::string &str)
{
    std::stringstream ss;
    char c;
    int strlen = (int)str.size();
    while (true)
    {
        c = ((char *)this->socket->recv(1).data())[0];
        ss << c;
        if (str[strlen - 1] == c)
        {
            if (strlen == 1)
            {
                break;
            }
            if (ss.str().ends_with(str))
            {
                break;
            }
        }
    }
    return buffer(ss.str().c_str(), ss.str().size());
}

void bufferStream::resize(size_t size)
{
    char* new_buffer = new char[size];
    memcpy(new_buffer, _buffer, min(_size,size));
    delete[] _buffer;
    _buffer = new_buffer;
    _size = size;
}

bufferStream& bufferStream::operator<<(const char* value){
    size_t len = strlen(value);
    if(_size - _pos < len){
        resize(_size + len);
    }
    memcpy(_buffer + _pos, value, len);
    this->_pos += len;
    return *this;
}

bufferStream& bufferStream::operator<<(const std::string& value){
    size_t len = value.size();
    if(_size - _pos < len){
        resize(_size + len);
    }
    memcpy(_buffer + _pos, value.c_str(), len);
    this->_pos += len;
    return *this;
}

bufferStream& bufferStream::operator<<(const char chr){
    if(_size - _pos < 1){
        resize(_size + 1);
    }
    memcpy(_buffer + _pos, &chr, 1);
    this->_pos += + 1;
    return *this;
}

buffer& buffer::operator=(buffer&& other) noexcept{
    this->~buffer();
    new (this) buffer(other);
    return *this;
}

RangeFileStreamReader::RangeFileStreamReader(const std::string& filepath, size_t start, size_t end) : 
    FileStreamReader(filepath), start(start), end(end)
{
    this->file->seekg(start);
}

RangeFileStreamReader::RangeFileStreamReader(const std::string& filepath, std::ifstream&& stream, size_t start, size_t end) : 
    FileStreamReader(filepath, std::move(stream)), start(start), end(end)
{
    this->file->seekg(start);
}

buffer RangeFileStreamReader::read(size_t size)
{
    if (this->end == -1) return FileStreamReader::read(size);
    if (this->file->tellg() >= this->end) {
        return buffer();
    }
    size_t max_size = this->end - this->file->tellg();
    if (size > max_size) {
        size = max_size;
    }
    return FileStreamReader::read(size);
}

buffer RangeFileStreamReader::readLine()
{
    if (this->end == -1) return FileStreamReader::readLine();
    std::streampos pos  = this->file->tellg();
    if (pos >= this->end) {
        return buffer();
    }
    buffer ret = FileStreamReader::readLine();
    if (ret.size() > this->end - pos) {
        ret.resize(this->end - pos);
    }
    return ret;
}

buffer RangeFileStreamReader::readUntil(const std::string& str)
{
    if (this->end == -1) return FileStreamReader::readUntil(str);
    std::streampos pos  = this->file->tellg();
    if (pos >= this->end) {
        return buffer();
    }
    buffer ret = FileStreamReader::readUntil(str);
    if (ret.size() > this->end - pos) {
        ret.resize(this->end - pos);
    }
    return ret;
}

void buffer::resize(size_t newsize){
    if(newsize == this->size()){
        return;
    }
    char* new_buffer = new char[newsize];
    memcpy(new_buffer, this->_buffer, min(this->size(), newsize));
    delete[] this->_buffer;
    this->_buffer = new_buffer;
    this->_size = newsize;
}


