import dian
import socket
import threading

ALLOW_METHODS = ['OPTIONS','SETUP','PLAY','TEARDOWN']
HandlerClass = type()

def handler(req:dian.DianRequest,server:dian.DianServer):
    if 'CSeq' not in req.headers:
        server.server.sendall(dian.DianResponse("1.0",400,"Bad Request",{}).parse())
        return
    def thread_handler():
        if req.method not in "OPTIONS":
            server.server.sendall(dian.DianResponse("1.0",405,"Method Not Allowed",{"Allow":",".join(ALLOW_METHODS),"CSeq":req.headers["CSeq"]}).parse())
            server.server.socket.close()
            return None
        if hasattr(HandlerClass,req.method):
            handler = getattr(HandlerClass,req.method)
            handler(req,server)
            server.server.socket.close()
            return None
        if hasattr(HandlerClass,"DEFAULT"):
            handler = getattr(HandlerClass,"DEFAULT")
            handler(req,server)
            server.server.socket.close()
            return None
        server.server.sendall(dian.DianResponse("1.0",501,"Not Implemented",{"CSeq":req.headers["CSeq"]}).parse())
        server.server.socket.close()
        return None
    thread = threading.Thread(target=thread_handler)
    thread.start()

server = dian.DianServer(("localhost", 8000))
server.on_handle = handler

