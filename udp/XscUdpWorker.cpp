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

#include "XscUdpWorker.h"
#include "XscUdpServer.h"
#include "XscUdpChannel.h"
#include "XscUdpLog.h"
#include "../core/Xsc.h"
#include "../core/XscTimerMgr.h"

XscUdpWorker::XscUdpWorker(XscUdpServer* udpServer) :
		XscWorker(udpServer)
{
	this->efd = ::epoll_create(1); 
	this->sfd = 0;
	this->mtu = udpServer->cfg->peerMtu;
}

bool XscUdpWorker::publish(const string& host, int port)
{
	this->sfd = Net::udpBindNoBlocking(host.c_str(), port, NULL);
	if (this->sfd < 0)
	{
		LOG_FAULT("libxsc can not listen on: %s:%d, errno: %s(%d)", host.c_str(), port, ::strerror(errno), errno)
		return false;
	}
	Net::setRCVBUF(this->sfd, ((XscUdpServer*) this->server)->cfg->peerRcvBuf);
	Net::setSNDBUF(this->sfd, ((XscUdpServer*) this->server)->cfg->peerSndBuf);
	struct epoll_event ee = { 0 };
	ee.data.fd = this->sfd;
	ee.events = (EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLET);
	if (::epoll_ctl(this->efd, EPOLL_CTL_ADD, this->sfd, &ee) == -1)
	{
		LOG_FAULT("call epoll_ctl (EPOLL_CTL_ADD) failed, worker-indx: 0, efd: %d, errno: %s(%d)", this->efd, ::strerror(errno), errno)
		return false;
	}
	LOG_INFO("libxsc already listen on %s:%d, and waiting for datagram, server-name: %s, index: 0x%02X, epoll-fd: 0x%08X, sfd: 0x%08X", host.c_str(), port, this->server->ne.c_str(), this->wk, this->efd, this->sfd)
	return true;
}

void XscUdpWorker::loop()
{
	string threadName;
	SPRINTF_STRING(&threadName, "udp-wk-%04X", this->wk)
	::prctl(PR_SET_NAME, threadName.c_str());
	::pthread_setspecific(Xsc::pkey, this);
	this->timerMgr.reset(new XscTimerMgr(this));
	this->server->waitWorker->set();
	struct epoll_event* evns = (struct epoll_event*) ::calloc(1, sizeof(struct epoll_event) * 0x10000);
	this->addCfd4Read(this->evn);
	LOG_INFO("worker-thread start successful, server-name: %s, index: 0x%02X, epoll-fd: 0x%08X, evn: 0x%08X", this->server->ne.c_str(), this->wk, this->efd, this->evn)
	int i = 0;
	int count = 0;
	bool haveItcEvent = false;
	while (1)
	{
		this->busy = false;
		count = ::epoll_wait(this->efd, evns, 0x10000, -1);
		this->busy = true;
		haveItcEvent = false;
		for (i = 0; i < count; ++i)
		{
			if (evns[i].events & EPOLLOUT) 
			{
				this->evnSend(evns + i);
				continue;
			}
			if (!(evns[i].events & EPOLLIN))
			{
				this->evnErro(evns + i); 
				continue;
			}
			this->evnRecv(evns + i, &haveItcEvent); 
		}
		if (haveItcEvent)
			this->doFuture(); 
	}
	::free(evns);
}

void XscUdpWorker::evnSend(struct epoll_event* evn)
{

}

void XscUdpWorker::evnRecv(struct epoll_event* evn, bool* isItcEvent)
{
	if (evn->data.fd == this->evn) 
	{
		this->evnItc();
		*isItcEvent = true;
		return;
	}
	struct sockaddr_in peer;
	static socklen_t socklen = sizeof(struct sockaddr_in);
	XscUdpServer* udpServer = (XscUdpServer*) this->server;
	while (true)
	{
		ssize_t len = ::recvfrom(evn->data.fd, this->rbuf, sizeof(this->rbuf), MSG_DONTWAIT, (struct sockaddr*) &peer, &socklen);
		if (len < 1) 
			break;
		if (len > this->mtu)
		{
			LOG_DEBUG("over the xsc-udp server mtu: %d", this->mtu)
			continue;
		}
		shared_ptr<XscUdpChannel> channel = static_pointer_cast<XscUdpLog>(udpServer->log)->newXscUdpChannel(this, udpServer->genCfd(), Net::sockaddr2str(&peer));
		channel->addr = (struct sockaddr_in*) ::malloc(sizeof(struct sockaddr_in));
		::memcpy(channel->addr, &peer, sizeof(struct sockaddr_in));
		this->stat->incv(XscWorkerStatItem::XSC_WORKER_RX_BYTES, len);
		channel->evnRecv(this, this->rbuf, (int) len);
	}
}

void XscUdpWorker::evnErro(struct epoll_event* evn)
{
	LOG_ERROR("have a error occur on server fd, errno(%d): %s", errno, ::strerror(errno))
}

void XscUdpWorker::evnItc()
{
	static ullong count;
	while (::read(this->evn, &count, sizeof(ullong)) > 0)
		;
}

void XscUdpWorker::doFuture()
{
	this->busy = true;
	list<function<void()>> tmpAfs;
	::pthread_mutex_lock(&this->mutex);
	while (!this->afs.empty())
	{
		tmpAfs.push_back(this->afs.front());
		this->afs.pop();
	}
	::pthread_mutex_unlock(&this->mutex);
	for (auto& it : tmpAfs)
		it();
	this->busy = false;
}

void XscUdpWorker::dida(ullong now)
{

}

void XscUdpWorker::addCfd4Read(int cfd)
{
	struct epoll_event ce = { 0 };
	ce.data.fd = cfd;
	ce.events = EPOLLIN | EPOLLERR | EPOLLPRI | EPOLLRDHUP | EPOLLET;
	if (::epoll_ctl(this->efd, EPOLL_CTL_ADD, cfd, &ce) == -1)
		LOG_FAULT("add FD to epoll failed, cfd: %d, errno: %s(%d)", cfd, ::strerror(errno), errno)
}

string XscUdpWorker::toString()
{
	string str;
	SPRINTF_STRING(&str, "index: 0x%04X, epoll-fd: 0x%08X, sfd: 0x%08X", this->wk, this->efd, this->sfd)
	return str;
}

XscUdpWorker::~XscUdpWorker()
{

}

