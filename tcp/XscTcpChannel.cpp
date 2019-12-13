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

#include "XscTcpChannel.h"
#include "XscTcpWorker.h"
#include "XscTcpCfg.h"
#include "XscTcpServer.h"
#include "../core/Xsc.h"
#include "../core/XscTimerMgr.h"
#include "../core/XscWorkerStat.h"

XscTcpChannel::XscTcpChannel(ActorType type, XscTcpWorker* wk, int mtu, int cfd, const string &peer) :
		XscChannel(XscProtocolType::XSC_PROTOCOL_TCP, type, wk, cfd, peer)
{
	this->est = false;
	this->dlen = 0;
	this->rbuf = (uchar*) ::malloc(mtu);
	this->wbuf = new queue<xsc_channel_wbuf*>();
}

void XscTcpChannel::send(uchar* dat, int len)
{
	XscTcpWorker* wk = (XscTcpWorker*) Xsc::getXscWorker();
	wk->stat->incv(XscWorkerStatItem::XSC_WORKER_TX_BYTES, len);
	wk->stat->inc(XscWorkerStatItem::XSC_WORKER_TX_MSGS);
	static_pointer_cast<XscTcpLog>(wk->server->log)->tx(this, dat, len);
	if (!this->est) 
	{
		LOG_DEBUG("tcp channel was lost, can not send any more, this: %s", this->toString().c_str())
		return;
	}
	if (this->wbuf != NULL)
	{
		this->sendBuf(dat, len);
		return;
	}
	if (::send(this->cfd, dat, len, MSG_DONTWAIT) == len) 
	{
		this->evnCanSend();
		return;
	}
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
		LOG_DEBUG("tcp channel buffer was full, can not send anymore, we will close this peer: %s, size: %08X", this->peer.c_str(), len)
	} else
	{
		LOG_DEBUG("client socket exception, peer: %s, cfd: %d, size: %08X, errno: %s(%d)", this->peer.c_str(), this->cfd, len, ::strerror(errno), errno)
	}
	wk->delChannel(this->cfd);
	this->est = false;
	this->cleanWbuf();
	this->clean();
	this->evnDisc();
}

void XscTcpChannel::sendBuf(uchar* dat, int len)
{
	if (!this->wbuf->empty()) 
	{
		xsc_channel_wbuf* wb = (xsc_channel_wbuf*) ::malloc(sizeof(xsc_channel_wbuf));
		wb->len = len;
		wb->pos = 0;
		wb->dat = (uchar*) ::malloc(wb->len);
		::memcpy(wb->dat, dat, wb->len);
		this->wbuf->push(wb);
		return;
	}
	loop:
	int w = ::send(this->cfd, dat, len, MSG_DONTWAIT);
	if (w == len) 
	{
		this->evnCanSend();
		return;
	}
	if (errno == EAGAIN || errno == EWOULDBLOCK) 
	{
		w = w < 0 ? 0 : w;
		xsc_channel_wbuf* wb = (xsc_channel_wbuf*) ::malloc(sizeof(xsc_channel_wbuf));
		wb->len = len - w;
		wb->pos = 0;
		wb->dat = (uchar*) ::malloc(wb->len);
		::memcpy(wb->dat, dat + w, wb->len);
		this->wbuf->push(wb);
		((XscTcpWorker*) this->worker)->addCfd4Write(this->cfd);
		return;
	}
	if (errno == 0 && w > 0)
	{
		dat += w;
		len -= w;
		goto loop;
	}
	LOG_DEBUG("client socket exception, peer: %s, cfd: %d, w: %d, len: %d, errno: %d", this->peer.c_str(), this->cfd, w, len, errno)
	((XscTcpWorker*) this->worker)->delChannel(this->cfd);
	this->est = false;
	this->cleanWbuf();
	this->clean();
	this->evnDisc();
}

void XscTcpChannel::evnSend()
{
	XscTcpWorker* wk = (XscTcpWorker*) this->worker;
	while (this->wbuf && !this->wbuf->empty())
	{
		xsc_channel_wbuf* wb = this->wbuf->front();
		int r = wb->len - wb->pos;
		int w = ::send(this->cfd, wb->dat + wb->pos, r, MSG_DONTWAIT);
		if (w == r) 
		{
			this->wbuf->pop();
			::free(wb->dat);
			::free(wb);
			wk->stat->incv(XscWorkerStatItem::XSC_WORKER_TX_BYTES, w);
			this->stat.incv(XscTcpChannelStatItem::XSC_TCP_CHANNEL_TX_BYTES, w);
			continue;
		}
		if (errno == EAGAIN || errno == EWOULDBLOCK) 
		{
			if (w > 0) 
			{
				wk->stat->incv(XscWorkerStatItem::XSC_WORKER_TX_BYTES, w);
				this->stat.incv(XscTcpChannelStatItem::XSC_TCP_CHANNEL_TX_BYTES, w);
				wb->pos += w;
			}
			return;
		}
		if (errno == 0 && w > 0)
		{
			wk->stat->incv(XscWorkerStatItem::XSC_WORKER_TX_BYTES, w);
			this->stat.incv(XscTcpChannelStatItem::XSC_TCP_CHANNEL_TX_BYTES, w);
			wb->pos += w;
			continue;
		}
		LOG_DEBUG("client socket exception, peer: %s, cfd: %d, errno: %s(%d)", this->peer.c_str(), this->cfd, strerror(errno), errno)
		wk->delChannel(this->cfd);
		this->est = false;
		this->cleanWbuf();
		this->clean();
		this->evnDisc();
		return;
	}
	wk->delCfd4Write(this->cfd); 
	this->evnCanSend();
}

void XscTcpChannel::close()
{
	if (!this->est)
		return;
	this->closeSlient();
	this->evnDisc();
}

void XscTcpChannel::closeSlient()
{
	this->cleanWbuf();
	this->clean();
	if (!this->est)
		return;
	((XscTcpWorker*) this->worker)->delChannel(this->cfd);
	this->est = false;
}

void XscTcpChannel::lazyClose()
{
	if (!this->est) 
	{
		LOG_TRACE("channel already lost: %s", this->toString().c_str())
		return;
	}
	shared_ptr<XscTcpChannel> channel = ((XscTcpWorker*) this->worker)->findChannel(this->cfd);
	if (channel == nullptr) 
	{
		LOG_FAULT("it`s a bug, this: %s", this->toString().c_str())
		return;
	}
	this->worker->timerMgr->addTimerOneTime(((XscTcpServer*) ((XscTcpWorker*) this->worker)->server)->cfg->lazyClose, [channel]
	{
		if(!channel->est) 
		{
			LOG_TRACE("channel already lost: %s", channel->toString().c_str())
			return;
		}
		channel->close();
	});
}

void XscTcpChannel::cleanWbuf()
{
	while (this->wbuf != NULL && !this->wbuf->empty())
	{
		xsc_channel_wbuf* wb = this->wbuf->front();
		::free(wb->dat);
		::free(wb);
		this->wbuf->pop();
	}
	if (this->wbuf != NULL)
		delete this->wbuf;
	this->wbuf = NULL;
}

void XscTcpChannel::evnCanSend()
{

}

XscTcpChannel::~XscTcpChannel()
{
	::free(this->rbuf);
	this->cleanWbuf();
}

