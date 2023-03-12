import socket
import re
import attr
from typing import Callable
import threading

class Server:
    def __init__(self,address) -> None:
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind(address)
        self.on_connect:Callable[[[socket.socket,...]]] = None

    def listen(self):
        self.socket.listen(5)
        while True:
            client, addr = self.socket.accept()
            def thread_load():
                self.on_connect(client,addr)
            thread = threading.Thread(target=thread_load)
            thread.start()

    def sendall(self,value:str|bytes):
        if isinstance(value,str):
            value = value.encode()
        self.socket.sendall(value)

class Client:
    def __init__(self,address) -> None:
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect(address)
    
    def send(self,data):
        if isinstance(data,str):
            data = data.encode()
        self.socket.sendall(data)

    def recv(self,size:int):
        return self.socket.recv(size)
    

@attr.s
class DianRequest:
    method:str
    url:str
    version:str
    headers:dict[str,str]
    body:socket.socket

    @staticmethod
    def unparse(socket:socket.socket):
        ret = DianRequest()
        data = read_until(socket,"\r\n").decode()
        match = RequestHead_re.match(data)
        if not match:
            raise Exception("Invalid head")
        head = match.groupdict()
        ret.method = head["method"]
        ret.url = head["url"]
        ret.version = head["version"]
        headers = {}
        while True:
            data = read_until(socket,"\r\n").decode()
            match = BaseHeader_re.match(data)
            if not match:
                break
                ...
            header = match.groupdict()
            headers.append(header["key"].strip(),header["value"].strip())
            if data == "\r\n":
                break
        ret.headers = headers
        ret.body = socket
        return ret
    
    @classmethod
    def parse(self):
        return f"{self.method} {self.url} {self.version}\r\n" + "\r\n".join([f"{key}:{value}" for key,value in self.headers.items()]) + "\r\n\r\n"

@attr.s
class DianResponse:
    version:str
    status:int
    status_text:str
    headers:dict[str,str]
    body:str|bytes

    @classmethod
    def parse(self):
        return f"{self.version} {self.status} {self.status_text}\r\n" + "\r\n".join([f"{key}:{value}" for key,value in self.headers.items()]) + "\r\n\r\n" + self.body
    
    @staticmethod
    def unparse(socket:socket.socket):
        ret = DianResponse()
        data = read_until(socket,"\r\n").decode()
        match = ResponseHead_re.match(data)
        if not match:
            raise Exception("Invalid head")
        head = match.groupdict()
        ret.version = head["version"]
        ret.status = int(head["status"])
        ret.status_text = head["status_text"]
        headers = {}
        while True:
            data = read_until(socket,"\r\n").decode()
            match = BaseHeader_re.match(data)
            if not match:
                break
                ...
            header = match.groupdict()
            headers.append(header["key"].strip(),header["value"].strip())
            if data == "\r\n":
                break
        ret.headers = headers
        ret.body = socket.recv(int(ret.headers["Content-Length"]))
        return ret
    
        
def read_until(socket:socket.socket,until:str,max:int=-1):
    data = b""
    while True:
        data += socket.recv(1)
        if max != -1 and len(data) > max:
            return data
        if data.endswith(until.encode()):
            return data

RequestHead_re = re.compile(r"^(?P<method>[A-Z]+) (?P<url>.+) (?P<version>.+)$")
ResponseHead_re = re.compile(r"^(?P<version>.+) (?P<status>[0-9]+) (?P<status_text>.+)$")
BaseHeader_re = re.compile(r"^(?P<key>.+):(?P<value>.+)$")

class DianServer:
    def __init__(self,server:Server) -> None:
        self.server = server
        self.on_request = None
        self.thread_local = threading.local()
        
    def listen(self):
        def on_connect(client:socket.socket,addr):
            while True:
                request = DianRequest.unparse(client)
                self.thread_local['client'] = client
                if self.on_request:
                    if self.on_request(request) == False:
                        client.close()
                        break
        self.server.on_connect = on_connect
        self.server.listen()

    def send(self,response:DianResponse):
        self.thread_local['client'].sendall(response.parse())

class DianClient:
    def __init__(self,client:Client) -> None:
        self.client = client

    def request(self,request:DianRequest):
        self.client.send(request.parse())
        return DianResponse.unparse(self.client.socket)
