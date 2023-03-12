from socket import AF_INET, socket, SOCK_STREAM
import uuid

from . import base
from .base import Address
from .ulits import Local
from queue import Queue
import threading
import attr

ALLOW_METHODS = ['OPTIONS','SETUP','PLAY','TEARDOWN']
VERSION = '0.5'

class Server(base.Base):
    def __init__(self,address:Address,thread_num:int = 1):
        super().__init__()
        self._address = address
        self._socket = socket(AF_INET,SOCK_STREAM)
        self._sessions:dict[str,base.Session] = {}
        self._thread_num = thread_num
        self._task_queue:Queue[tuple[socket,Address]] = Queue()
        self._use_auth = False
        self._auths = []
        self.root = 'testfile'

    def enable_auth(self):
        self._use_auth = True
        
    def add_auth(self,auth:str):
        self._auths.append(auth)

    def _thread_boot(self):
        while True:
            socket, address = self._task_queue.get()
            
            while True:
                # 处理请求
                try:
                    req = base.Request.unparse(socket)
                except Exception as e:
                    socket.close()
                    break
                try:

                    if req.method not in ALLOW_METHODS:
                        base.Response(version=VERSION,status=400,status_text='Bad Request',headers={},req=req,body='').response()
                        continue
                    if req.method == 'OPTIONS':
                        base.Response(version=VERSION,status=200,status_text='OK',headers={},req=req,body='').response()
                        continue
                    if req.method == 'SETUP':
                        if 'Transport' not in req.headers:
                            base.Response(version=VERSION,status=461,status_text='Transport Not Found',req=req,headers={},body='').response()
                            continue
                        if self._use_auth:
                            if 'Authorization' not in req.headers:
                                base.Response(version=VERSION,status=401,status_text='Unauthorized',req=req,headers={'WWW-Authenticate':'Basic realm="dian"'},body='').response()
                                continue
                            if req.headers['Authorization'] not in ["Bearer " + i for i in self._auths]:
                                base.Response(version=VERSION,status=403,status_text='Forbidden',req=req,headers={},body='').response()
                                continue
                        session = uuid.uuid4()
                        self._sessions[session] = base.Session(req,socket,session)
                        base.Response(version=VERSION,status=200,status_text='OK',headers={'Session':str(session),'Transport':req.headers['Transport']},req=req,body='').response()
                        continue
                    if req.method == 'PLAY':
                        if 'Session' not in req.headers:
                            base.Response(version=VERSION,status=453,status_text='Session Not Found',req=req,headers={},body='').response()
                            continue
                        session = uuid.UUID(req.headers['Session'])
                        if session not in self._sessions:
                            base.Response(version=VERSION,status=454,status_text='Session Not Found',req=req,headers={},body='').response()
                            continue
                        if self._sessions[session].socket != socket:
                            base.Response(version=VERSION,status=455,status_text='Session Not Current',req=req,headers={},body='').response()
                            continue
                        if 'Range' not in req.headers:
                            self._sessions[session].play()
                        else:
                            self._sessions[session].play(req.headers['Range'])
                        # base.Response(version=VERSION,status=200,status_text='OK',req=req,headers={},body='').response()
                        continue
                    if req.method == 'TEARDOWN':
                        if 'Session' not in req.headers:
                            base.Response(version=VERSION,status=453,status_text='Session Not Found',req=req,headers={},body='').response()
                            continue
                        session = uuid.UUID(req.headers['Session'])
                        if session not in self._sessions:
                            base.Response(version=VERSION,status=454,status_text='Session Not Found',req=req,headers={},body='').response()
                            continue
                        if self._sessions[session].socket != socket:
                            base.Response(version=VERSION,status=455,status_text='Session Not Current',req=req,headers={},body='').response()
                            continue
                        base.Response(version=VERSION,status=200,status_text='OK',req=req,headers={},body='').response()
                        self._sessions[session].close()
                        self._sessions.pop(session)
                        break
                except base.StopHandler:
                    continue


    def start(self):
        for i in range(self._thread_num):
            threading.Thread(target=self._thread_boot,daemon=True).start()
        self._socket.bind(self._address)
        self._socket.listen(self._thread_num)
        while True:
            socket, address = self._socket.accept()
            self._task_queue.put((socket,address))
