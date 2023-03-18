import dian
import threading
import time

# 多线程并发

def dian_test():
    client = dian.Client(('localhost',664),'diantp://localhost:664/testfile')
    client.connect()
    client.setup('bearer-key')
    res = client.play('npt=1-3')
    print(res.body.readall())
    res = client.play()
    print(res.body.readall())
    client.teardown()
    client.disconnect()

    print('\n'.join([str(i) for i in client.play_record]) + '\n')

def options_test():
    client = dian.Client(('localhost',554),'diantp://localhost:554/testfile')
    client.connect()
    print(client.options().parse())
    client.disconnect()

options_test()
dian_test()
#ths = [threading.Thread(target=dian_test) for i in range(10)]
#for i in ths:
#    i.start()
#for i in ths:
#    i.join()

