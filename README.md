# libxsc-cpp

* X server communication library

![img](http://www.dev5.cn/x_msg_im/start/xsc/img/libxsc-cpp-arch.svg)

* 这是一个基于`actor`模型的单进程多线程并发通信服务器底座. 它的目标是为上层应用提供一个并行无锁, 网络透明, 全异步的开发环境.

* 它支持多种转输层协议: `tcp`, `udp`, `websocket`, `http`和基于[kcp](https://github.com/skywind3000/kcp)的`rudp`. 换句话说, 你可以在同一进程内同时启动以上五种协议的并发服务器. 用来支持不同需求的客户端接入.

* 它不支持任何应用层协议. 

 在[X-MSG-IM](https://github.com/dev5cn/x-msg-im)系统中, 它与[libxsc-proto-cpp](https://github.com/dev5cn/libxsc-proto-cpp)和[libx-msg-im-xsc](https://github.com/dev5cn/libx-msg-im-xsc)一起为所有核心网元提供统一的网络编程框架.
