# Dian-Protocal

## 安装

暂未发布到PyPI,直接复制粘贴

## 接口

```python
import dian
```

## 简要使用

### Server

```python
server = dian.Server(('localhost',554),2)
server.enable_auth()
server.add_auth('bearer-key')
server.start()
```

其中,`('localhost',554)`是地址,`2`是线程数(不包括当前线程)

`enable_auth`是验证方法,此处采取的是密匙验证(Authentication验证,现行C++版本是常规HTTP账号密码验证)

`add_auth`是添加密匙

`start`是开启服务器(阻塞)

### Client

```python
client = dian.Client(('localhost',554),'diantp://localhost:554/testfile')
client.connect()
client.setup('bearer-key')
res = client.play()
print(res.body.readall())
res = client.play()
print(res.body.readall())
client.teardown()
client.disconnect()
```

其中`('localhost',554)`是目标地址,`'diantp://localhost:554/testfile'`是URL

`connect` 连接

`setup` 安装密匙,发出SETUP报文

`play` 播放

`readall` 读取play返回的内容

`teardown` 发出teardown报文

`disconnect` 取消连接
