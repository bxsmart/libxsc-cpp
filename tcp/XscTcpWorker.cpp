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

#include "XscTcpWorker.h"
#include "XscTcpServer.h"
#include "XscTcpCfg.h"
#include "XscTcpChannel.h"
#include "../core/Xsc.h"
#include "../core/XscTimerMgr.h"
#include "../core/XscWorkerStat.h"

XscTcpWorker::XscTcpWorker(shared_ptr<XscTcpServer> tcpServer, int maxFdSize) :
		XscWorker(tcpServer)
{
	this->maxFdSize = maxFdSize;
	this->efd = ::epoll_create(this->maxFdSize);
	this->sfd = 0;
	this->mtu = tcpServer->cfg->peerMtu;
}

bool XscTcpWorker::publish(const string& host, int port)
{
	this->sfd = Net::tcpListen(host.c_str(), port);
	if (this->sfd < 0)
	{
		LOG_FAULT("libxsc can not listen on: %s:%d, errno: %s(%d)", host.c_str(), port, ::strerror(errno), errno)
		return false;
	}
	Net::setNoBlocking(this->sfd);
	struct epoll_event ee = { 0 };
	ee.data.fd = this->sfd;
	ee.events = (EPOLLIN | EPOLLERR | EPOLLRDHUP | EPOLLET);
	if (::epoll_ctl(this->efd, EPOLL_CTL_ADD, this->sfd, &ee) == -1)
	{
		LOG_FAULT("call epoll_ctl (EPOLL_CTL_ADD) failed, worker-indx: 0, efd: %d, errno: %s(%d)", this->efd, ::strerror(errno), errno)
		return false;
	}
	LOG_INFO("libxsc already listen on %s:%d, and waiting for connection, server-name: %s, index: 0x%02X, epoll-fd: 0x%08X, sfd: 0x%08X", host.c_str(), port, this->server->ne.c_str(), this->wk, this->efd, this->sfd)
	return true;
}

void XscTcpWorker::loop()
{
	string threadName;
	SPRINTF_STRING(&threadName, "tcp-wk-%04X", this->wk)
	::prctl(PR_SET_NAME, threadName.c_str());
	::pthread_setspecific(Xsc::pkey, this);
	this->timerMgr.reset(new XscTimerMgr(this));
	this->server->waitWorker->set();
	struct epoll_event* evns = (struct epoll_event*) ::calloc(1, sizeof(struct epoll_event) * this->maxFdSize);
	this->addCfd4Read(this->evn);
	LOG_INFO("worker-thread start successful, server-name: %s, index: 0x%02X, epoll-fd: 0x%08X, evn: 0x%08X", this->server->ne.c_str(), this->wk, this->efd, this->evn)
	int i = 0;
	int count = 0;
	bool haveItcEvent = false;
	while (1)
	{
		this->busy = false;
		count = ::epoll_wait(this->efd, evns, this->maxFdSize, -1);
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
			if (evns[i].data.fd == this->sfd)
			{
				this->evnConn(); 
				continue;
			}
			this->evnRecv(evns + i, &haveItcEvent); 
		}
		if (haveItcEvent)
			this->doFuture(); 
	}
	::free(evns);
}

void XscTcpWorker::evnConn()
{
	static socklen_t socklen = sizeof(struct sockaddr_in);
	XscTcpServer* tcpServer = (XscTcpServer*) this->server;
	shared_ptr<XscTcpWorker> wk = static_pointer_cast<XscTcpWorker>(this->shared_from_this());
	struct sockaddr_in peer;
	int cfd;
	while (1) 
	{
		cfd = ::accept(this->sfd, (struct sockaddr*) &peer, &socklen);
		if (cfd == -1) 
			break;
		if (cfd >= (int) tcpServer->cfg->peerLimit)
		{
			LOG_FAULT("over the xsc tcp server max file description size: %d, we will close this connection, cfd: %d", tcpServer->cfg->peerLimit, cfd)
			Net::close(cfd);
			continue;
		}
		LOG_TRACE("got a connection from: %s, cfd: %d", Net::sockaddr2str(&peer).c_str(), cfd)
		shared_ptr<XscTcpChannel> tcpChannel = static_pointer_cast<XscTcpLog>(tcpServer->log)->newXscTcpChannel(wk, cfd, Net::sockaddr2str(&peer));
		this->setFdAtt(tcpServer, cfd);
		this->addTcpChannel(tcpChannel);
		this->addCfd4Read(cfd);
		this->stat->inc(XscWorkerStatItem::XSC_WORKER_N2H_TOTAL);
		this->checkZombie(tcpChannel, tcpServer->cfg->n2hZombie);
		this->checkHeartbeat(tcpChannel, tcpServer->cfg->heartbeat);
	}
}

void XscTcpWorker::evnSend(struct epoll_event* evn)
{
	auto it = this->channel.find(evn->data.fd);
	if (it == this->channel.end()) 
	{
		LOG_FAULT("it`s a bug, can not found connection for fd: %d", evn->data.fd)
		return;
	}
	it->second->evnSend();
}

void XscTcpWorker::evnRecv(struct epoll_event* evn, bool* isItcEvent)
{
	if (evn->data.fd == this->evn) 
	{
		this->evnItc();
		*isItcEvent = true;
		return;
	}
	auto it = this->channel.find(evn->data.fd);
	if (it == this->channel.end()) 
	{
		LOG_FAULT("it`s a bug, can not found connection for fd: %d", evn->data.fd)
		return;
	}
	shared_ptr<XscTcpChannel> channel = it->second;
	int len;
	bool flag = true;
	while (flag)
	{
		len = ::recv(evn->data.fd, channel->rbuf + channel->dlen, this->mtu - channel->dlen, MSG_DONTWAIT);
		if (len < 1)
		{
			if ((len == -1 && errno != EAGAIN) || len == 0)
				flag = false;
			break;
		}
		this->stat->incv(XscWorkerStatItem::XSC_WORKER_RX_BYTES, len);
		channel->stat.incv(XscTcpChannelStatItem::XSC_TCP_CONNECTION_RX_BYTES, len);
		static_pointer_cast<XscTcpLog>(this->server->log)->rx(channel.get(), channel->rbuf + channel->dlen, len);
		channel->dlen += len;
		len = channel->evnRecv(this, channel->rbuf, channel->dlen);
		if (len > 0) 
		{
			int remain = channel->dlen - len;
			if (remain == 0) 
			{
				channel->dlen = 0;
				continue;
			}
			if (remain < 0) 
			{
				LOG_FAULT("it`s a bug, len: %d, dlen: %d, channel: %s", len, channel->dlen, channel->toString().c_str())
				flag = false;
				break;
			}
			for (int i = 0; i < remain; ++i)
				channel->rbuf[i] = channel->rbuf[len + i];
			channel->dlen = remain;
			continue;
		}
		if (len == 0) 
			continue;
		flag = false; 
	}
	if (channel->dlen >= this->mtu) 
	{
		LOG_TRACE("tcp receive buffer full, can not read any more, we will close it, channel: %s", channel->toString().c_str())
		goto label;
	}
	if (flag) 
		return;
	label:
	LOG_TRACE("client disconnected: %s, errno(%d): %s", channel->toString().c_str(), errno, ::strerror(errno))
	if (!channel->est) 
	{
		LOG_TRACE("channel already lost: %s", channel->toString().c_str())
		return;
	}
	this->delTcpChannel(channel->cfd);
	channel->est = false;
	channel->cleanWbuf();
	channel->clean();
	channel->evnDisc();
}

void XscTcpWorker::evnErro(struct epoll_event* evn)
{
	auto it = this->channel.find(evn->data.fd);
	if (it == this->channel.end()) 
	{
		LOG_FAULT("it`s a bug, can not found tcp connection for fd: %d", evn->data.fd)
		return;
	}
	shared_ptr<XscTcpChannel> channel = it->second;
	LOG_TRACE("tcp connection disconnected: %s, errno: %s(%d)", channel->toString().c_str(), ::strerror(errno), errno)
	this->delTcpChannel(channel->cfd);
	channel->est = false;
	channel->cleanWbuf();
	channel->clean();
	channel->evnDisc();
}

void XscTcpWorker::evnItc()
{
	static ullong count;
	while (::read(this->evn, &count, sizeof(ullong)) > 0)
		;
}

void XscTcpWorker::doFuture()
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

void XscTcpWorker::addTcpChannel(shared_ptr<XscTcpChannel> channel)
{
	auto it = this->channel.find(channel->cfd);
	if (it != this->channel.end()) 
	{
		LOG_FAULT("it`s a bug, an: %s", channel->toString().c_str())
		return;
	}
	this->channel[channel->cfd] = channel;
}

shared_ptr<XscTcpChannel> XscTcpWorker::findTcpChannel(int cfd)
{
	auto it = this->channel.find(cfd);
	return it == this->channel.end() ? nullptr : it->second;
}

void XscTcpWorker::delTcpChannel(int cfd)
{
	if (this->channel.erase(cfd) != 1)
	{
		LOG_FAULT("it`s a bug, can not found cfd: %d", cfd)
	}
	this->delCfd(cfd);
	Net::close(cfd);
	this->stat->inc(XscWorkerStatItem::XSC_WORKER_N2H_DESTORY);
}

void XscTcpWorker::addCfd4Read(int cfd)
{
	struct epoll_event ce = { 0 };
	ce.data.fd = cfd;
	ce.events = EPOLLIN | EPOLLERR | EPOLLPRI | EPOLLRDHUP | EPOLLET;
	if (::epoll_ctl(this->efd, EPOLL_CTL_ADD, cfd, &ce) == -1)
		LOG_FAULT("add FD to epoll failed, cfd: %d, errno: %s(%d)", cfd, ::strerror(errno), errno)
}

void XscTcpWorker::addCfd4Write(int cfd)
{
	struct epoll_event ce = { 0 };
	ce.data.fd = cfd;
	ce.events = EPOLLOUT | EPOLLIN | EPOLLERR | EPOLLPRI | EPOLLRDHUP | EPOLLET;
	if (::epoll_ctl(this->efd, EPOLL_CTL_MOD, cfd, &ce) == -1)
		LOG_FAULT("mod FD to epoll failed, cfd: %d, errno: %s(%d)", cfd, ::strerror(errno), errno)
}

void XscTcpWorker::delCfd4Write(int cfd)
{
	struct epoll_event ce = { 0 };
	ce.data.fd = cfd;
	ce.events = EPOLLIN | EPOLLERR | EPOLLPRI | EPOLLRDHUP | EPOLLET;
	if (::epoll_ctl(this->efd, EPOLL_CTL_MOD, cfd, &ce) == -1)
		LOG_FAULT("mod FD to epoll failed, cfd: %d, errno: %s(%d)", cfd, ::strerror(errno), errno)
}

void XscTcpWorker::delCfd(int cfd)
{
	if (::epoll_ctl(this->efd, EPOLL_CTL_DEL, cfd, NULL) == -1)
		LOG_FAULT("remove FD from epoll failed, cfd: %d, errno: %s(%d)", cfd, ::strerror(errno), errno)
}

void XscTcpWorker::setFdAtt(XscTcpServer* tcpServer, int cfd)
{
	Net::setNoBlocking(cfd);
	Net::setLinger(cfd);
	Net::setSNDBUF(cfd, tcpServer->cfg->peerSndBuf / 2);
	Net::setRCVBUF(cfd, tcpServer->cfg->peerRcvBuf / 2);
	Net::setNODELAY(cfd);
}

void XscTcpWorker::dida(ullong now)
{
	this->timerMgr->dida(now);
	for (auto& it : this->h2ns)
		it->dida(now);
}

void XscTcpWorker::checkZombie(shared_ptr<XscTcpChannel> channel, int zombie )
{
	this->timerMgr->addTimerOneTime(zombie, [zombie, channel]
	{
		if(!channel->est) 
		{
			LOG_DEBUG("n2h channel already lost: %s", channel->toString().c_str())
			return;
		}
		if(channel->lts == 0) 
		{
			LOG_DEBUG("got a zombie tcp connection, we will close it, channel: %s", channel->toString().c_str())
			channel->close(); 
		}
	});
}

void XscTcpWorker::checkHeartbeat(shared_ptr<XscTcpChannel> channel, int heartbeat )
{
	if (heartbeat < 1) 
		return;
	int hb = heartbeat * 3;
	this->timerMgr->addTimer(hb, [hb, channel]
	{
		if (!channel->est) 
		{
			LOG_DEBUG("n2h channel already lost: %s", channel->toString().c_str())
			return false;
		}
		if (Xsc::clock > channel->lts + (hb * 1000L))
		{
			LOG_DEBUG("n2h channel lost heart-beat, we will close it, channel: %s, elap: %dms", channel->toString().c_str(), (int)(Xsc::clock - channel->lts))
			channel->close();
			return false;
		}
		return true;
	});
}

string XscTcpWorker::toString()
{
	string str;
	SPRINTF_STRING(&str, "index: 0x%04X, epoll-fd: 0x%08X, sfd: 0x%08X", this->wk, this->efd, this->sfd)
	return str;
}

XscTcpWorker::~XscTcpWorker()
{

}

