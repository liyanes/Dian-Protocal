# Dian 协议模型构建
import socket

class Server:
    def __init__(self,address) -> None:
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind(address)
    
    def listen(self):
        self.socket.listen(5)
        while True:
            client, addr = self.socket.accept()
            print(f"Connected by {addr}")
            while True:
                data = client.recv(1024)
                if not data:
                    break
                print("Received", str(data))
                client.sendall("Received your message".encode())
            client.close()

class Client:
    def __init__(self,address) -> None:
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect(address)
    
    def send(self,data):
        if isinstance(data,str):
            data = data.encode()
        self.socket.sendall(data)
        data = self.socket.recv(1024)
        print("Received", str(data))

if __name__ == "__main__":
    import sys
    if sys.argv[1] == "server":
        server = Server(("localhost", 8000))
        server.listen()
    elif sys.argv[1] == "client":
        client = Client(("localhost", 8000))
        client.send("Hello")
