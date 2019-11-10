# libxsc-cpp

* X server communication library

* 这是一个基于`actor`模型的单进程多线程并发通信服务器底座. 它的目标是为上层应用提供一个并行无锁, 网络透明, 全异步的开发环境.

* 它支持多种协议同时接入: `tcp`, `udp`, `rudp`, `websocket`, `http`, etc.

* 它不支持任何应用层协议. 

* 只支持linux 3.x kernel or later.

* 在`X-MSG-IM`系统中, 它将与[libxsc-proto-cpp](https://github.com/dev5cn/libxsc-proto-cpp)和[libx-msg-im-xsc](https://github.com/dev5cn/libx-msg-im-xsc)一起为所有核心网元提供统一的网络编程框架.
