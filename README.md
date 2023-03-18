# Dian-Protocal

Dian团队招新题目库(RTSP协议部分功能复现)

## StartUp

### Python 版本

环境
Python 3.10.6
对版本要求不大

文件
- src/model-2.3/test.py 服务器示例
- src/model-2.3/testc.py 客户端示例

库
- src/model-2.3/dian

该库暂时没有打包

支持多线程(服务器和客户端),有每次的请求和响应记录

运行

```shell
pip install -i requirements.txt
python3 src/model-2.3/test.py &
python3 src/model-2.3/testc.py
```

### C++版本

目前对C++只开发服务器版本(懒得调bug了)

要求C++ std:20 / std:23, 因为有暂时未应用的C++协程库

由于网络库使用,目前仅支持Windows

文件
- /src/cpp/src

编译文件
- /src/cpp/projects/vc22/Dian-Protocol/Dian-Protocol.sln


