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

#ifndef RUDP_XSCRUDPWORKER_H_
#define RUDP_XSCRUDPWORKER_H_

#if !defined (__LIBXSC_H__) && !defined (LIBXSC)
#error only libxsc.h can be included directly.
#endif

#include "../udp/XscUdpWorker.h"

class XscRudpChannel;

class XscRudpWorker: public XscUdpWorker
{
public:
	unordered_map<string , shared_ptr<XscRudpChannel>> channel; 
public:
	void dida(ullong now); 
	void checkChannelRt(ullong now); 
	XscRudpWorker(XscUdpServer* server);
	virtual ~XscRudpWorker();
public:
	shared_ptr<XscRudpChannel> findChannel(const string& cid); 
	void delChannel(const string& cid); 
protected:
	void evnRecv(struct epoll_event* evn, bool* isItcEvent); 
private:
	void evnConn(const string& cid, uint conv, struct sockaddr_in& addr, uchar* dat, int size); 
	void evnRead(const string& cid, shared_ptr<XscRudpChannel> channel); 
	void checkHeartbeat(shared_ptr<XscRudpChannel> channel, int heartbeat ); 
	static int kcpOutput(const char* buf, int len, void* kcp, void* user); 
};

#endif 
