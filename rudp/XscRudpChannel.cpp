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

#include "XscRudpChannel.h"
#include "XscRudpLog.h"
#include "XscRudpCfg.h"
#include "XscRudpServer.h"
#include "../core/Xsc.h"
#include "../core/XscTimerMgr.h"

XscRudpChannel::XscRudpChannel(XscRudpWorker* wk, int mtu, int cfd, const string &peer) :
		XscUdpChannel(wk, mtu, cfd, peer)
{
	this->dlen = 0;
	this->rbuf = (uchar*) ::malloc(mtu);
}

void XscRudpChannel::send(uchar* dat, int len)
{
	XscRudpWorker* wk = (XscRudpWorker*) Xsc::getXscWorker();
	wk->stat->incv(XscWorkerStatItem::XSC_WORKER_TX_BYTES, len);
	wk->stat->inc(XscWorkerStatItem::XSC_WORKER_TX_MSGS);
	static_pointer_cast<XscRudpLog>(wk->server->log)->tx(this, dat, len);
	if (!this->est) 
	{
		LOG_DEBUG("rudp channel was lost, can not send any more, this: %s", this->toString().c_str())
		return;
	}
	if (this->kcp == NULL)
	{
		LOG_FAULT("it`s a bug, this: %s", this->toString().c_str())
		return;
	}
	ikcp_send(this->kcp, (char*) dat, len);
	ikcp_update(this->kcp, Xsc::clock & 0xFFFFFFFFULL);
}

void XscRudpChannel::close()
{
	if (!this->est)
		return;
	this->closeSlient();
	this->evnDisc();
}

void XscRudpChannel::closeSlient()
{
	if (!this->est)
		return;
	((XscRudpWorker*) this->worker)->delChannel(this->cid);
	this->est = false;
}

void XscRudpChannel::lazyClose()
{
	if (!this->est) 
	{
		LOG_TRACE("channel already lost: %s", this->toString().c_str())
		return;
	}
	shared_ptr<XscRudpChannel> channel = ((XscRudpWorker*) this->worker)->findChannel(this->cid);
	if (channel == nullptr) 
	{
		LOG_FAULT("it`s a bug, this: %s", this->toString().c_str())
		return;
	}
	shared_ptr<XscRudpCfg> cfg = static_pointer_cast<XscRudpCfg>((((XscRudpServer*) ((XscRudpWorker*) this->worker)->server)->cfg));
	this->worker->timerMgr->addTimerOneTime(cfg->lazyClose, [channel]
	{
		if (!channel->est) 
		{
			LOG_TRACE("channel already lost: %s", channel->toString().c_str())
			return;
		}
		channel->close();
	});
}

string XscRudpChannel::toString()
{
	string str;
	SPRINTF_STRING(&str, "cid: %s, peer: %s", this->cid.c_str(), Net::sockaddr2str(((struct sockaddr_in* ) this->kcp->user)).c_str())
	return str;
}

XscRudpChannel::~XscRudpChannel()
{
	if (this->rbuf != NULL)
	{
		::free(this->rbuf);
		this->rbuf = NULL;
	}
	if (this->kcp != NULL)
	{
		ikcp_release(this->kcp);
		this->kcp = NULL;
	}
}

