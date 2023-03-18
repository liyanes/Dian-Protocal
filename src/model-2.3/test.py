import dian
import os
import sys

dian.setroot(os.path.dirname(os.path.abspath(sys.argv[0])).split('src')[0] + 'testfile')

server = dian.Server(('localhost',664),4)
server.enable_auth()
server.add_auth('bearer-key')
server.start()

