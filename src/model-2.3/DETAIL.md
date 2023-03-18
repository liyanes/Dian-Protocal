# Dian-Protocal(Python)详细

## 基础思路

- 利用socket连接
- 建立多线程,每个线程一个accept来实现异步处理
- 对报文进行解析(dian.Request类)
- 返回报文(dian.Response类)

## 对象化

### StreamReader

> 在文件base.py

抽象类,用于异步读取数据,来实现分块内存传输

在没有该类的情况下,socket读取内容和返回文件内容都会直接readall,读取大文件时会造成内存使用过大,在使用该类时,缓冲会在socket read/write时再进行读取,此时可以明显减少内存使用(对于大文件)

抽象方法:
- readall 读取流中的所有内容

### SocketStreamReader

> 在文件base.py
> 继承于StreamReader

对socket的进一步封装,用于读取body

是对socket的即时性读取,节省程序内存使用

方法:
- read_until 读到指定内容结束
- at_eof 判断读取是否结束(只有限制读取bytes有效)
- read 读取指定字节
- readall 读取全部内容(只有限制读取bytes有效)

### FileStreamReader

> 在文件base.py
> 继承于StreamReader

对FileIO的进一步封装,用于读取文件(或者读取文件Range)

为防止返回文件内容时内存过多使用而使用FileStreamReader,主要是为了Range的兼容性

方法:
- readall 读取全部内容(或者指定Range)

### Request

> 在文件base.py

请求的结构体

包含以下字段:
- method
- url
- version
- body
- headers
- sock

其中body是前文的StreamReader | str | bytes,sock是socket类

方法:
- unparse 从socket读取并分析请求
- parse 将当前Request写入bytes,即完整报文

parse方法暂未考虑报体回调传输

### Response

响应的结构体

包含以下字段:
- version
- status
- status_text
- headers
- body
- req

其中req是对应的请求

方法:
- unparse 从socket读取并分析请求
- parse 将当前Response写入bytes,即完整报文
- parseHead 将当前Response的报头写入bytes
- response 返回给对应socket

### Session

会话类

> 注:该会话类暂时未进行文件权限认证,如 /../../ 等URL路径攻击

字段:
- session_id
- socket
- request

方法:
- play
- close

### Server

服务器类

字段:
- _address 地址(含端口)
- _socket socket类
- _sessions 会话类
- _thread_num 线程数
- _task_queue 任务序列,任务内部包含了accept得到的值,将由多线程处理
- _use_auth 是否使用验证机制(密匙验证)
- _auths 密匙列表,如果是大项目,考虑写入mysql
- root 服务器映射根目录,文件将从根目录读取(类似于HTTP FollowSysPath),此处并没有设置检测路径合法性

方法:
- enable_auth 开始密匙验证
- add_auth 添加密匙
- _thread_boot 线程方法,用于处理socket读取数据并进行报文分析,并返回报文
- start 开启服务器

### Client

客户端类

字段:
- _address 地址
- _socket socket类
- _session 会话类
- _cseq CSeq 序列号
- _url 访问的url
- play_record play记录

方法:
- connect
- disconnect
- request
- setup
- play
- teardown

## 基本逻辑

- 服务器开启
- 绑定地址
- 开启线程,并开始处理队列
- 开始侦听
- 客户端开启
- 客户端setup
- 服务器单线程接收
- 线程获取队列信息,Request.unparse分析,判断auth,创建Session类到列表,返回session_id
- 客户端Response.unparse分析,读取Session
- ...
- 客户端play
- 服务端读取session,找到对应的列表,判断数据是否符合
- 返回对应文件片段
- 客户端接收
- ...
- 客户端teardown
- 服务端读取
