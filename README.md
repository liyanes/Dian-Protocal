# Dian-Protocal

Dian团队招新题目库

使用编程语言 Python
(因为是小项目,而且不是特别注重运行效率)

环境
Python 3.10.6
对版本要求不大

文件
- src/model-2.3/test.py 服务器
- src/model-2.3/testc.py 客户端

库
- src/model-2.3/dian

支持多线程(服务器和客户端),有每次的请求和响应记录

现在在排除bug,等我调试

运行

```shell
pip -i requirements.txt
python3 src/model-2.3/test.py &
python3 src/model-2.3/testc.py &
```
