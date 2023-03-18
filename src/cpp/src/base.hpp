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

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

namespace dian
{
	class NotImplementError : public std::exception
	{
		using std::exception::exception;
	};
	class bufferStream;
	class buffer
	{
	protected:
		char *_buffer;
		// void* _buffer;
		size_t _size;

	public:
		inline buffer()
		{
			_buffer = nullptr;
			_size = 0;
		}
		inline buffer(size_t size)
		{
			_buffer = new char[size];
			_size = size;
		};
		inline buffer(void *buffer, size_t size);
		inline buffer(const void *buffer, size_t size)
		{
			_buffer = new char[size];
			memcpy(_buffer, buffer, min(this->_size, size));
			_size = size;
		};
		inline buffer(buffer &&other) noexcept
		{
			_buffer = other._buffer;
			_size = other._size;
			other._buffer = nullptr;
			other._size = 0;
		};
		buffer &operator=(buffer &&other) noexcept;
		inline buffer(const buffer &other)
		{
			_buffer = new char[other._size];
			memcpy(_buffer, other._buffer, other._size);
			_size = other._size;
		};
		// inline buffer(void*&& buffer, size_t size) { _buffer = buffer; _size = size; buffer = nullptr; };
		inline ~buffer()
		{
			if (_buffer)
				delete[] _buffer;
		};
		inline void *data() { return _buffer; };
		inline size_t size() const { return _size; };
		inline const void *data() const { return _buffer; };

		void resize(size_t newsize);
	};

	class bufferStream : public buffer
	{
	protected:
		size_t _pos = 0;

	public:
		using buffer::buffer;
		void resize(size_t newsize);

		bufferStream &operator<<(const char *value);
		bufferStream &operator<<(const std::string &value);
		bufferStream &operator<<(const char chr);

		inline bufferStream &operator<<(const int value);
		inline bufferStream &operator<<(const unsigned value);
		inline bufferStream &operator<<(const long value);

		inline operator buffer() { return buffer(this->_buffer, _pos); }
		inline buffer to_buffer() { return buffer(this->_buffer, _pos); }
	};

	class StreamReader
	{
	public:
		StreamReader() = default;
		StreamReader(const StreamReader &) = delete;
		StreamReader &operator=(const StreamReader &) = delete;
		StreamReader(StreamReader &&) = delete;
		StreamReader &operator=(StreamReader &&) = delete;
		virtual ~StreamReader() = default;
		virtual buffer readLine() = 0;
		virtual buffer read(size_t length) = 0;
		virtual buffer readUntil(const std::string &delimiter) = 0;
		inline virtual size_t maxlen() { return -1; };
	};
	class FileStreamReader : public StreamReader
	{
	protected:
		std::string filepath;
		std::ifstream *file;
		size_t filesize;
		bool do_free;

	public:
		FileStreamReader(const std::string &filepath);
		FileStreamReader(const std::string &filepath, std::ifstream &&stream);
		FileStreamReader(const FileStreamReader &) = delete;
		FileStreamReader &operator=(const FileStreamReader &) = delete;
		FileStreamReader(FileStreamReader &&) = delete;
		FileStreamReader &operator=(FileStreamReader &&) = delete;
		virtual ~FileStreamReader();
		virtual buffer readLine() override;
		virtual buffer read(size_t length) override;
		virtual buffer readUntil(const std::string &delimiter) override;
		inline virtual size_t maxlen() override { return filesize; };

		inline size_t size() { return filesize; }
	};
	class RangeFileStreamReader : public FileStreamReader
	{
	protected:
		size_t start;
		size_t end;

	public:
		RangeFileStreamReader(const std::string &filepath, size_t start = 0, size_t end = -1);
		RangeFileStreamReader(const std::string &filepath, std::ifstream &&stream, size_t start = 0, size_t end = -1);
		RangeFileStreamReader(const RangeFileStreamReader &) = delete;
		RangeFileStreamReader &operator=(const RangeFileStreamReader &) = delete;
		RangeFileStreamReader(RangeFileStreamReader &&) = delete;
		RangeFileStreamReader &operator=(RangeFileStreamReader &&) = delete;
		virtual buffer readLine() override;
		virtual buffer read(size_t length) override;
		virtual buffer readUntil(const std::string &delimiter) override;
		inline virtual size_t maxlen() override { return end - start; };
	};
	class StringStreamReader : public StreamReader
	{
		std::string str;
		std::stringstream stream;

	public:
		StringStreamReader(const std::string &str);
		StringStreamReader(const StringStreamReader &) = delete;
		StringStreamReader &operator=(const StringStreamReader &) = delete;
		StringStreamReader(StringStreamReader &&) = delete;
		StringStreamReader &operator=(StringStreamReader &&) = delete;
		virtual ~StringStreamReader() = default;
		virtual buffer readLine() override;
		virtual buffer read(size_t length) override;
		virtual buffer readUntil(const std::string &delimiter) override;
		inline virtual size_t maxlen() override { return str.size(); };
	};
	namespace socket
	{
		class SocketError : public std::exception
		{
		public:
			int errcode;
			inline SocketError(int errcode = -1) : std::exception(), errcode(errcode){};
			inline SocketError(const char *errmsg, int errcode = -1) : std::exception(errmsg), errcode(errcode) {}
		};
		class ListenError : public SocketError
		{
			using SocketError::SocketError;
		};
		class AcceptError : public SocketError
		{
			using SocketError::SocketError;
		};
		class ConnectError : public SocketError
		{
			using SocketError::SocketError;
		};
		class SendError : public SocketError
		{
			using SocketError::SocketError;
		};
		class RecvError : public SocketError
		{
			using SocketError::SocketError;
		};
		class BindError : public SocketError
		{
			using SocketError::SocketError;
		};
		class socket
		{
			SOCKET socketvalue;
			bool closed = false;

		public:
			socket(SOCKET socketvalue);
			socket(int af = AF_INET, int type = SOCK_STREAM, int protocol = 0);
			socket(const socket &) = delete;
			socket &operator=(const socket &) = delete;
			socket(socket &&) noexcept;
			socket &operator=(socket &&) noexcept;
			~socket();
			void listen(const char *host, unsigned short port);
			socket accept();
			void connect(const std::string &host, unsigned short port);
			void close();
			void send(const buffer &data);
			void send(StreamReader &data);
			void shutdown(int how);
			buffer recv(size_t length);
			buffer recvUntil(const std::string &delimiter, int max = 1024, bool set_text = true);
			buffer recvLine(int max = 1024);

			inline SOCKET sysid() { return socketvalue; };
		};
	}

	class SocketStreamReader : public StreamReader
	{
		socket::socket *socket;

	public:
		SocketStreamReader(socket::socket *socket);
		SocketStreamReader(const SocketStreamReader &) = delete;
		SocketStreamReader &operator=(const SocketStreamReader &) = delete;
		SocketStreamReader(SocketStreamReader &&) = delete;
		SocketStreamReader &operator=(SocketStreamReader &&) = delete;
		virtual ~SocketStreamReader();
		virtual buffer readLine() override;
		virtual buffer read(size_t length) override;
		virtual buffer readUntil(const std::string &delimiter) override;
	};
	namespace rtsp
	{
		using method = std::string;
		using url = std::string;
		struct Version
		{
			unsigned major : 4 = 0;
			unsigned minor : 4 = 5;
		};
		struct Headers : std::map<std::string, std::string>
		{
			using std::map<std::string, std::string>::map;
			using std::map<std::string, std::string>::operator[];
		};
		struct Base
		{
			method method;
			url url;
			Version version;
			Headers headers;
			StreamReader *body = nullptr;

			/// @brief parse head of rtsp message(Request)
			/// @param socket socket to read from
			/// @return Base object
			static Base parse(socket::socket &socket);
			void parseThis(socket::socket &socket);
		};
		class Request : public Base
		{
		public:
			dian::socket::socket *socket = nullptr;
			inline Request(){};
			inline Request(dian::socket::socket *socket) : socket(socket){};

			~Request();
			/// @brief Unparse head of rtsp message
			/// @return Unparsed buffer
			dian::buffer unparseHead();
			static Request parse(dian::socket::socket &socket);
			void parseThis(dian::socket::socket &socket);
		};
		class Response : public Base
		{
			void _apply_res();

		protected:
			bool sended = false;

		public:
			int status = 200;
			std::string reason = "OK";

			const Request *source;
			inline Response(const Request *source) : source(source) { _apply_res(); };
			inline Response(const Request *source, const std::string &body) : source(source)
			{
				this->body = new StringStreamReader(body);
				_apply_res();
			};
			inline Response(const Request *source, StreamReader *body) : source(source)
			{
				this->body = body;
				_apply_res();
			};

			inline ~Response()
			{
				if (body)
					delete body;
			};

			dian::buffer unparseHead();
			static Response parse(const Request *source, dian::socket::socket socket);

			void send();
			inline bool has_sended() { return sended; };
		};
	}
}

inline dian::buffer::buffer(void *data, size_t size) : _buffer(new char[size]), _size(size)
{
	memcpy(_buffer, data, size);
}

inline dian::bufferStream &dian::bufferStream::operator<<(const int value)
{
	return *this << std::to_string(value);
}

inline dian::bufferStream &dian::bufferStream::operator<<(const unsigned value)
{
	return *this << std::to_string(value);
}

inline dian::bufferStream &dian::bufferStream::operator<<(const long value)
{
	return *this << std::to_string(value);
}
