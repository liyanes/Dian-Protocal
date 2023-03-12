from . import base
from socket import socket,AF_INET,SOCK_STREAM
import uuid

class Client(base.Base):
    def __init__(self,address:base.Address,url:str):
        super().__init__()
        self._address = address
        self._socket = socket(AF_INET,SOCK_STREAM)
        self._session:base.Session = None
        self._cseq = 0
        self._url = url
        self.play_record:list[base.Request] = []

    def connect(self):
        self._socket.connect(self._address)

    def disconnect(self):
        self._socket.close()

    def request(self,req:base.Request):
        self._socket.send(req.parse().encode())
        return base.Response.unparse(self._socket)

    def setup(self,auth:str|None = None):
        headers = {'CSeq':str(self._cseq),'Transport':'TCP'}
        if auth:
            headers['Authorization'] = "Bearer " + auth
        req = base.Request(method='SETUP',url=self._url,version=base.VERSION,headers=headers,body='')
        self._cseq += 1
        res = self.request(req)
        if res.status != 200:
            raise Exception(res.status_text)
        self._session = base.Session(req,self._socket,uuid.UUID(res.headers['Session']))

    def play(self,range:str|None = None):
        headers = {'CSeq':str(self._cseq),'Session':str(self._session.session_id)}
        if range:
            headers['Range'] = range
        req = base.Request(version=base.VERSION,method='PLAY',url=self._url,headers=headers,body='')
        self.play_record.append(req)
        self._cseq += 1
        res = self.request(req)
        if res.status != 200 and res.status != 206:
            raise Exception(res.status_text)
        return res

    def teardown(self):
        req = base.Request(version=base.VERSION,method='TEARDOWN',url=self._url,headers={'CSeq':str(self._cseq),'Session':str(self._session.session_id)},body='')
        self._cseq += 1
        res = self.request(req)
        if res.status != 200:
            raise Exception(res.status_text)

    def __del__(self):
        self.disconnect()
