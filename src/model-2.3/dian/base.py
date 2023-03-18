from abc import abstractmethod
import uuid
import attr
import socket
import re

import os
VERSION = '0.5'
root = 'testfile'

def setroot(value:str):
    global root
    root = value

def _to_str(value:bytes) -> str:
    return value.decode() if isinstance(value,bytes) else value

class StreamReader:
    @abstractmethod
    def readall(self):
        pass

class SocketStreamReader(StreamReader):
    def __init__(self,socket:socket.socket,max_read:int = -1) -> None:
        self._socket = socket
        self._max_read = int(max_read)
        self._have_read = 0

    def read_until(self,until:str,max:int = -1) -> bytes:
        ret = b""
        while True:
            data = self._socket.recv(1)
            if not data:
                break
            ret += data
            if max != -1 and len(ret) >= max:
                break
            if self._max_read != -1:self._have_read += 1
            if self._max_read != -1 and self._have_read >= self._max_read:
                break
            if ret.endswith(until.encode()):
                break
        return ret
    
    def at_eof(self):
        return self._max_read != -1 and self._have_read >= self._max_read
    
    def read(self,max:int = -1) -> bytes:
        if self._max_read == -1:
            self._have_read += max
            return self._socket.recv(max)
        else:
            to_read = self._max_read - self._have_read if max == -1 else min(self._max_read - self._have_read,max)
            self._have_read += to_read
            return self._socket.recv(to_read)
    
    def readall(self):
        if self._max_read == -1:
            return b''
        if self._have_read == self._max_read:
            return b''
        return self.read()
    
    def __del__(self):
        self.readall()
    
class FileStreamReader(StreamReader):
    def __init__(self,file,limit:tuple[int,int] = (-1,-1)) -> None:
        self._file = file
        if limit[0] != -1:
            self._file.seek(limit[0])
        self._limit = limit
        self._have_read = 0
    
    def readall(self):
        return self._file.read(self._limit[1] - self._limit[0]) if self._limit[1] != -1 else self._file.read()

RequestHead_re = re.compile("^(?P<method>[A-Z]+) (?P<url>.+) (?P<version>.+)(\r)?$")
BaseHeader_re = re.compile("^(?P<key>.+?) *: *(?P<value>.+)(\r)?$")
ResponseHead_re = re.compile("^(?P<version>.+) (?P<status>[0-9]+) (?P<status_text>.+)(\r)?$")

@attr.s(auto_attribs=True)
class Request:
    method:str = ''
    url:str = ''
    version:str = ''
    headers:dict[str,str] = {}
    body:StreamReader | bytes | str = ''

    sock:socket.socket = None

    @staticmethod
    def unparse(socket:socket.socket,max_read:int = -1):
        ret = Request()
        data = SocketStreamReader(socket,max_read).read_until("\r\n").decode()
        match = RequestHead_re.match(data)
        if not match:
            raise Exception("Invalid head")
        head = match.groupdict()
        ret.method = head["method"]
        ret.url = head["url"]
        ret.version = head["version"]
        headers = {}
        while True:
            data = SocketStreamReader(socket,max_read).read_until("\r\n").decode()
            match = BaseHeader_re.match(data)
            if not match:
                break
                ...
            header = match.groupdict()
            headers[header["key"].strip()] = header["value"].strip()
            if data == "\r\n":
                break
        ret.headers = headers
        ret.body = SocketStreamReader(socket)
        ret.sock = socket
        return ret

    def parse(self):
        return f"{self.method} {self.url} {self.version}\r\n" + "\r\n".join([f"{key}:{value}" for key,value in self.headers.items()]) + "\r\n\r\n" + (
            _to_str(self.body.readall()) if isinstance(self.body,StreamReader) else _to_str(self.body)
        )
    
@attr.s(auto_attribs=True,slots=True)
class Response:
    version:str = VERSION
    status:int = 200
    status_text:str = 'OK'
    headers:dict[str,str] = {}
    body:StreamReader | bytes| str = ''

    req:Request = None

    def response(self):
        self.req.sock.send(self.parsehead().encode())
        if isinstance(self.body,StreamReader):
            self.req.sock.send(self.body.readall())
        else:
            self.req.sock.send(self.body.encode())

    def parse(self):
        return f"{self.version} {self.status} {self.status_text}\r\n" + "\r\n".join([f"{key}:{value}" for key,value in self.headers.items()]) + "\r\n\r\n" + (
            _to_str(self.body.readall()) if isinstance(self.body,StreamReader) else _to_str(self.body)
        )
    
    def parsehead(self):
        return f"{self.version} {self.status} {self.status_text}\r\n" + "\r\n".join([f"{key}:{value}" for key,value in self.headers.items()]) + "\r\n\r\n"
    
    @staticmethod
    def unparse(socket:socket.socket,max_read:int = -1):
        ret = Response()
        data = SocketStreamReader(socket,max_read).read_until("\r\n").decode()
        match = ResponseHead_re.match(data)
        if not match:
            raise Exception("Invalid head")
        head = match.groupdict()
        ret.version = head["version"]
        ret.status = int(head["status"])
        ret.status_text = head["status_text"]
        headers = {}
        while True:
            data = SocketStreamReader(socket,max_read).read_until("\r\n").decode()
            match = BaseHeader_re.match(data)
            if not match:
                break
                ...
            header = match.groupdict()
            headers[header["key"].strip()]=header["value"].strip()
            if data == "\r\n":
                break
        ret.headers = headers
        ret.body = SocketStreamReader(socket,headers['Content-Length'] if 'Content-Length' in headers else -1)
        return ret

class Base:
    def __init__(self) -> None:
        self._socket = None

Address = tuple[str,int]

class StopHandler(Exception):
    pass

class Session:
    def __init__(self,request:Request,socket:socket.socket,session_id:uuid.UUID) -> None:
        self.socket = socket
        self.session_id = session_id
        self.request = request

        match = re.compile(r'^diantp://(?P<host>[^/]+)/(?P<file>[^/]+)$').match(request.url)
        if not match:
            Response(version=VERSION,status=400,status_text='Bad Request',headers={},req=request,body='').response()
            raise StopHandler()
        
        import os
        
        file = match.groupdict()["file"]
        file = os.path.join(root,file)
        if not os.path.exists(file):
            print("File Not Found:",file)
            Response(version=VERSION,status=404,status_text='Not Found',headers={'Session':str(self.session_id)},req=request,body='').response()
            raise StopHandler()
        self.file = open(file,"rb")
        self.file.seek(0,2)
        self.file_size = self.file.tell()
        self.file.seek(0,0)

    def play(self,range = None):
        import re
        if not range:
            self.request.body = FileStreamReader(self.file,(0,self.file_size))
            self.request.headers["Range"] = f"bytes=0-{self.file_size}"
            self.request.headers["Content-Length"] = str(self.file_size)
            self.request.headers["Content-Range"] = f"bytes 0-{self.file_size-1}/{self.file_size}"
            self.request.headers["Accept-Ranges"] = "bytes"
            self.request.headers["Connection"] = "keep-alive"
            Response(version=VERSION,status=206,status_text='Partial Content',headers=self.request.headers,body=self.request.body,req=self.request).response()
            return
        match = re.compile('^npt *= *(?P<start>[0-9]+)-(?P<end>[0-9]+)? *$').match(range)
        if not match:
            raise Exception("Invalid range")
        start = int(match.groupdict()["start"])
        end = int(match.groupdict()["end"])
        if not end:
            end = self.file_size+1
        headers = {}

        headers["Range"] = f"bytes={start}-{end}"
        headers["Content-Length"] = str(end - start + 1)
        headers["Content-Range"] = f"bytes {start}-{end}/{self.file_size}"
        headers["Accept-Ranges"] = "bytes"
        headers["Connection"] = "keep-alive"

        Response(version=VERSION,status=206,status_text='Partial Content',headers=headers,req=self.request,body=
                FileStreamReader(self.file,(start,end+1))).response()
        
    def close(self):
        self.file.close()
        self.socket.close()
