import os
import dian_server as dian
import uuid
import re

server = dian.DianServer(dian.Server(("localhost", 8000)))

ALLOW_METHOD = ["OPTIONS","SETUP","PLAY","TEARDOWN"]

alive_session:list = []
ROOT = 'testfile/'
VERSION = "0.5"

def on_request(req:dian.DianRequest):
    if req.method not in ALLOW_METHOD:
        server.send(dian.DianResponse(VERSION,405,"Method Not Allowed",{"Allow":",".join(ALLOW_METHOD),"CSeq":req.headers["CSeq"]}).parse())
        return False
    if 'CSeq' not in req.headers:
        server.send(dian.DianResponse(VERSION,400,"Bad Request",{}).parse())
        return False
    if req.method == "OPTIONS":
        server.send(dian.DianResponse(VERSION,200,"OK",{"CSeq":req.headers["CSeq"]}).parse())
        return True
    if req.method == "SETUP":
        if "Session" in req.headers:
            if req.headers["Session"] in alive_session:
                server.send(dian.DianResponse(VERSION,200,"OK",{"CSeq":req.headers["CSeq"],"Session":req.headers["Session"]}).parse())
                return True
            else:
                server.send(dian.DianResponse(VERSION,454,"Session Not Found",{"CSeq":req.headers["CSeq"]}).parse())
                return False
        else:
            session = str(uuid.uuid4())
            alive_session.append(session)
            server.send(dian.DianResponse(VERSION,200,"OK",{"CSeq":req.headers["CSeq"],"Session":session}).parse())
            return True
    if req.method == "PLAY":
        if "Session" not in req.headers:
            server.send(dian.DianResponse(VERSION,400,"Bad Request",{}).parse())
            return False
        if req.headers["Session"] not in alive_session:
            server.send(dian.DianResponse(VERSION,454,"Session Not Found",{"CSeq":req.headers["CSeq"]}).parse())
            return False
        if '..' in req.url:
            server.send(dian.DianResponse(VERSION,400,"Bad Request",{}).parse())
            alive_session.remove(req.headers["Session"])
            return False
        if "Range" not in req.headers:
            server.send(dian.DianResponse(VERSION,400,"Bad Request",{}).parse())
            alive_session.remove(req.headers["Session"])
            return False
        
        path = os.path.join(ROOT,req.url)
        if not os.path.exists(path):
            server.send(dian.DianResponse(VERSION,404,"Not Found",{"CSeq":req.headers["CSeq"]}).parse())
            alive_session.remove(req.headers["Session"])
            return False
        if os.path.isdir(path):
            return False
        
        match = re.compile(r"npt=(\d+\.?\d*)-(\d+\.?\d*)").match(req.headers["Range"])
        if match == None:
            server.send(dian.DianResponse(VERSION,400,"Bad Request",{}).parse())
            alive_session.remove(req.headers["Session"])
            return False
        
        start = float(match.group(1))
        end = float(match.group(2))
        if start > end:
            server.send(dian.DianResponse(VERSION,400,"Bad Request",{}).parse())
            alive_session.remove(req.headers["Session"])
            return False
        if start < 0:
            server.send(dian.DianResponse(VERSION,400,"Bad Request",{}).parse())
            alive_session.remove(req.headers["Session"])
            return False
        
        file = open(path,'rb')
        file.seek(int(start))
        data = file.read(int(end-start))
        file.close()
        server.send(dian.DianResponse(VERSION,200,"OK",{"CSeq":req.headers["CSeq"],"Session":req.headers["Session"],"Content-Type":"application/octet-stream","Content-Length":str(len(data))}).parse())
        
        return True
    if req.method == "TEARDOWN":
        if "Session" not in req.headers:
            server.send(dian.DianResponse(VERSION,400,"Bad Request",{}).parse())
            return False
        if req.headers["Session"] not in alive_session:
            server.send(dian.DianResponse(VERSION,454,"Session Not Found",{"CSeq":req.headers["CSeq"]}).parse())
            return False
        server.send(dian.DianResponse(VERSION,200,"OK",{"CSeq":req.headers["CSeq"],"Session":req.headers["Session"]}).parse())
        alive_session.remove(req.headers["Session"])
        return True

server.on_request = on_request
server.listen()
