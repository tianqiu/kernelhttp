# kernelhttp
A simple http server in kernel.

内核socket接受浏览器的请求，加入一个工作队列处理。再通过socket转发给用户态程序处理url。

url通过外部程序处理，除了ssl外其他的功能基本和python服务器相同。

使用方法：
先打开url.py 再加载内核模块。
