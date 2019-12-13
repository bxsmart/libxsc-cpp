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

#ifndef UDP_XSCUDPWORKER_H_
#define UDP_XSCUDPWORKER_H_

#if !defined (__LIBXSC_H__) && !defined (LIBXSC)
#error only libxsc.h can be included directly.
#endif

#include "../core/XscWorker.h"

class XscUdpServer;

class XscUdpWorker: public XscWorker
{
public:
	int efd; 
	int sfd; 
	uchar rbuf[0x10000]; 
public:
	XscUdpWorker(XscUdpServer* udpServer);
	void loop(); 
	bool publish(const string& host, int port); 
	string toString();
	virtual ~XscUdpWorker();
public:
	void addCfd4Read(int cfd); 
public:
	virtual void dida(ullong now); 
protected:
	virtual void evnRecv(struct epoll_event* evn, bool* isItcEvent); 
	void evnSend(struct epoll_event* evn); 
	void evnErro(struct epoll_event* evn); 
	void evnItc(); 
	void doFuture(); 
};

#endif 
