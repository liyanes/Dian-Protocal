#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <winsock.h>
#include <fstream>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")

namespace dian {
	class buffer {
		void* _buffer;
		size_t _size;
	public:
		inline buffer(size_t size) { _buffer = new char[size]; _size = size; };
		inline buffer(void* buffer, size_t size) { _buffer = new char[size]; memcpy(_buffer, buffer, min(this->_size, size)); _size = size; };
		inline buffer(const void*buffer,size_t size){ _buffer = new char[size]; memcpy(_buffer, buffer, min(this->_size, size)); _size = size; };
		//inline buffer(void*&& buffer, size_t size) { _buffer = buffer; _size = size; buffer = nullptr; };
		inline ~buffer() { delete[] _buffer; };
		inline void* data() { return _buffer; };
		inline size_t size() const { return _size; };
		inline const void* data() const { return _buffer; };

		friend class bufferStream;
	};

	class bufferStream {
		void* pos;
		buffer _buffer;
	public:
		inline bufferStream(size_t size) :_buffer(size) { pos = _buffer.data(); };
		void resize(size_t size);
		void operator<<(const buffer& data);
		void operator<<(const char* data);
		inline void operator<<(const std::string& data) { operator<<(data.c_str()); };
		inline void operator<<(const int data) { operator<<(std::to_string(data).c_str()); };
		inline size_t size() { return _buffer.size(); };
		inline void* data() { return _buffer.data(); };
		buffer to_buffer();
	};

	class StreamReader {
	public:
		StreamReader() = default;
		StreamReader(const StreamReader&) = delete;
		StreamReader& operator=(const StreamReader&) = delete;
		StreamReader(StreamReader&&) = delete;
		StreamReader& operator=(StreamReader&&) = delete;
		virtual ~StreamReader() = default;
		virtual buffer readLine() = 0;
		virtual buffer read(size_t length) = 0;
		virtual buffer readUntil(const std::string& delimiter) = 0;
	};
	class FileStreamReader : public StreamReader {
		std::string filepath;
		std::ifstream file;
	public:
		FileStreamReader(std::string filepath);
		FileStreamReader(const FileStreamReader&) = delete;
		FileStreamReader& operator=(const FileStreamReader&) = delete;
		FileStreamReader(FileStreamReader&&) = delete;
		FileStreamReader& operator=(FileStreamReader&&) = delete;
		virtual ~FileStreamReader() = default;
		virtual buffer readLine() override;
		virtual buffer read(size_t length) override;
		virtual buffer readUntil(const std::string& delimiter) override;
	};
	class StringStreamReader : public StreamReader {
		std::string str;
		std::stringstream stream;
	public:
		StringStreamReader(std::string str);
		StringStreamReader(const std::string&& str);
		StringStreamReader(const StringStreamReader&) = delete;
		StringStreamReader& operator=(const StringStreamReader&) = delete;
		StringStreamReader(StringStreamReader&&) = delete;
		StringStreamReader& operator=(StringStreamReader&&) = delete;
		virtual ~StringStreamReader() = default;
		virtual buffer readLine() override;
		virtual buffer read(size_t length) override;
		virtual buffer readUntil(const std::string& delimiter) override;
	};
	namespace socket{
		class SocketError : public std::exception {
			using std::exception::exception;
		};
		class ListenError : public SocketError {
			using SocketError::SocketError;
		};
		class AcceptError : public SocketError {
			using SocketError::SocketError;
		};
		class ConnectError : public SocketError {
			using SocketError::SocketError;
		};
		class SendError : public SocketError {
			using SocketError::SocketError;
		};
		class RecvError : public SocketError {
			using SocketError::SocketError;
		};
		class BindError : public SocketError {
			using SocketError::SocketError;
		};
		class socket {
			SOCKET socketvalue;
			bool closed = false;
		public:
			socket(SOCKET socketvalue);
			socket(int af = AF_INET, int type = SOCK_STREAM, int protocol = 0);
			socket(const socket&) = delete;
			socket& operator=(const socket&) = delete;
			socket(socket&&) = delete;
			socket& operator=(socket&&) = delete;
			~socket() = default;
			void listen(const std::string& host, unsigned short port);
			socket accept();
			void connect(const std::string& host, unsigned short port);
			void close();
			void send(const buffer& data);
			void shutdown(int how);
			buffer recv(size_t length);
			buffer recvUntil(const std::string& delimiter,int max = 1024);
			buffer recvLine(int max = 1024);

		};
	}

	class SocketStreamReader : public StreamReader {
		socket::socket* socket;
	public:
		SocketStreamReader(socket::socket* socket);
		SocketStreamReader(const SocketStreamReader&) = delete;
		SocketStreamReader& operator=(const SocketStreamReader&) = delete;
		SocketStreamReader(SocketStreamReader&&) = delete;
		SocketStreamReader& operator=(SocketStreamReader&&) = delete;
		virtual ~SocketStreamReader() = default;
		virtual buffer readLine() override;
		virtual buffer read(size_t length) override;
		virtual buffer readUntil(const std::string& delimiter) override;
	};
	namespace rtsp {
		using method = std::string;
		using url = std::string;
		struct Version {
			unsigned major:4 = 0;
			unsigned minor:4 = 5;
		};
		struct Headers : std::map<std::string,std::string> {
			using std::map<std::string, std::string>::map;
			using std::map<std::string, std::string>::operator[];
		};
		struct Base {
			method method;
			url url;
			Version version;
			Headers headers;
			StreamReader *body;

			static Base parse(socket::socket &socket);
			void parseThis(socket::socket &socket);
		};
		class Request : public Base {
		public:
			dian::socket::socket* socket;
			inline Request(){};
			inline Request(dian::socket::socket* socket) : socket(socket) {};

			dian::buffer unparseHead();
			static Request parse(dian::socket::socket &socket);
		};
        class Response : public Base {
		public:
			const Request *source;
			inline Response(const Request* source) : source(source) {};
			inline Response(const Request* source, const std::string& body) : source(source) { this->body = new StringStreamReader(body); };
			inline Response(const Request* source, StreamReader* body) : source(source) { this->body = body; };

			dian::buffer unparseHead();
			static Response parse(const Request* source, dian::socket::socket socket);

			void send();
		};
	}
}

