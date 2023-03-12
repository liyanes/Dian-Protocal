import dian

server = dian.Server(('localhost',554),2)
server.enable_auth()
server.add_auth('bearer-key')
server.start()
