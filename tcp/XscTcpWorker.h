/*
  Copyright 2019 www.dev5.cn, Inc. dev5@qq.com
 
  This file is part of X-MSG-IM.
 
  X-MSG-IM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  X-MSG-IM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU Affero General Public License
  along with X-MSG-IM.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TCP_XSCTCPWORKER_H_
#define TCP_XSCTCPWORKER_H_

#if !defined (__LIBXSC_H__) && !defined (LIBXSC)
#error only libxsc.h can be included directly.
#endif

#include "../core/XscWorker.h"

class XscTcpServer;
class XscTcpChannel;

class XscTcpWorker: public XscWorker
{
public:
	int efd; 
	int sfd; 
	unordered_map<int , shared_ptr<XscTcpChannel>> channel; 
	list<shared_ptr<XscTcpChannel>> h2ns; 
public:
	XscTcpWorker(shared_ptr<XscTcpServer> tcpServer, int maxFdSize);
	void loop(); 
	bool publish(const string& host, int port); 
	string toString();
	virtual ~XscTcpWorker();
public:
	void addTcpChannel(shared_ptr<XscTcpChannel> channel); 
	shared_ptr<XscTcpChannel> findTcpChannel(int cfd); 
	void delTcpChannel(int cfd); 
	void addCfd4Read(int cfd); 
	void addCfd4Write(int cfd); 
	void delCfd4Write(int cfd); 
	void setFdAtt(XscTcpServer* tcpServer, int cfd); 
	void delCfd(int cfd); 
public:
	void dida(ullong now); 
private:
	void evnConn(); 
	void evnSend(struct epoll_event* evn); 
	void evnRecv(struct epoll_event* evn, bool* isItcEvent); 
	void evnErro(struct epoll_event* evn); 
	void evnItc(); 
	void doFuture(); 
private:
	void checkZombie(shared_ptr<XscTcpChannel> channel, int zombie ); 
	void checkHeartbeat(shared_ptr<XscTcpChannel> channel, int heartbeat ); 
};

#endif 
