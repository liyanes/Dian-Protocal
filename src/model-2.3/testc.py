import dian
import threading

# 多线程并发

def dian_test():
    client = dian.Client(('localhost',554),'diantp://localhost:554/testfile')
    client.connect()
    client.setup('bearer-key')
    res = client.play()
    print(res.body.readall())
    res = client.play()
    print(res.body.readall())
    client.teardown()
    client.disconnect()

    print('\n'.join([str(i) for i in client.play_record]) + '\n')

ths = [threading.Thread(target=dian_test) for i in range(10)]
for i in ths:
    i.start()
for i in ths:
    i.join()

