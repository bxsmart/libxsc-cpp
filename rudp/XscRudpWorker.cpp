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

#include "XscRudpWorker.h"
#include "XscRudpCfg.h"
#include "XscRudpChannel.h"
#include "XscRudpServer.h"
#include "ikcp.h"
#include "../core/Xsc.h"
#include "../core/XscTimerMgr.h"

XscRudpWorker::XscRudpWorker(XscUdpServer* server) :
		XscUdpWorker(server)
{

}

void XscRudpWorker::evnRecv(struct epoll_event* evn, bool* isItcEvent)
{
	if (evn->data.fd == this->evn) 
	{
		this->evnItc();
		*isItcEvent = true;
		return;
	}
	struct sockaddr_in peer;
	static socklen_t socklen = sizeof(struct sockaddr_in);
	while (true)
	{
		ssize_t len = ::recvfrom(evn->data.fd, this->rbuf, sizeof(this->rbuf), MSG_DONTWAIT, (struct sockaddr*) &peer, &socklen);
		if (len < 1) 
			break;
		if (len < 24) 
		{
			LOG_DEBUG("kcp pdu can not less len 24 bytes, peer: %s, dat: %s", Net::sockaddr2str(&peer).c_str(), Net::hex2strUperCaseSpace(this->rbuf, len).c_str())
			break;
		}
		if (len > this->mtu)
		{
			LOG_DEBUG("over the xsc-udp server mtu: %d", this->mtu)
			continue;
		}
		this->stat->incv(XscWorkerStatItem::XSC_WORKER_RX_BYTES, len);
		uint conv = ikcp_getconv(this->rbuf);
		string cid;
		SPRINTF_STRING(&cid, "%08X%08X%04X", conv, ntohl(peer.sin_addr.s_addr), ntohs(peer.sin_port))
		auto it = this->channel.find(cid);
		if (it == this->channel.end())
		{
			this->evnConn(cid, conv, peer, this->rbuf, len);
			continue;
		}
		ikcp_input(it->second->kcp, (char*) this->rbuf, len);
		if (len > 24) 
			this->evnRead(cid, it->second);
	}
}

void XscRudpWorker::evnConn(const string& cid, uint conv, struct sockaddr_in& addr, uchar* dat, int size)
{
	XscRudpServer* rudpServer = ((XscRudpServer*) this->server);
	shared_ptr<XscRudpCfg> cfg = static_pointer_cast<XscRudpCfg>(rudpServer->cfg);
	shared_ptr<XscRudpChannel> channel = static_pointer_cast<XscRudpLog>(rudpServer->log)->newXscRudpChannel(this, rudpServer->genCfd(), Net::sockaddr2str(&addr));
	channel->cid = cid;
	channel->addr = (struct sockaddr_in*) ::malloc(sizeof(struct sockaddr_in));
	::memcpy(channel->addr, &addr, sizeof(struct sockaddr_in));
	channel->kcp = ikcp_create(conv, channel->addr);
	channel->kcp->output = ((int (*)(const char *buf, int len, ikcpcb *kcp, void *user)) (XscRudpWorker::kcpOutput));
	channel->kcp->mtu = cfg->kcpMtu;
	channel->kcp->rx_minrto = cfg->kcpMinRto;
	ikcp_nodelay(channel->kcp, cfg->kcpNodelay ? 1 : 0, cfg->kcpInterval, cfg->kcpResend, cfg->kcpNc ? 0 : 1);
	ikcp_wndsize(channel->kcp, cfg->kcpSndWind, cfg->kcpRcvWind);
	this->channel[cid] = channel;
	this->stat->inc(XscWorkerStatItem::XSC_WORKER_N2H_TOTAL);
	LOG_DEBUG("got a rudp channel from client, cid: %s", cid.c_str())
	this->checkHeartbeat(channel, static_pointer_cast<XscRudpCfg>(rudpServer->cfg)->heartbeat); 
	ikcp_input(channel->kcp, (char*) dat, size);
	this->evnRead(cid, channel);
}

void XscRudpWorker::evnRead(const string& cid, shared_ptr<XscRudpChannel> channel)
{
	int len;
	bool flag = true;
	while (flag)
	{
		len = ikcp_recv(channel->kcp, (char*) channel->rbuf + channel->dlen, this->mtu - channel->dlen); 
		if (len == -1) 
			break;
		if (len < -1) 
		{
			LOG_DEBUG("rudp channel exception, ret: %d, cid: %s", len, cid.c_str())
			flag = false;
			break;
		}
		this->stat->incv(XscWorkerStatItem::XSC_WORKER_RX_BYTES, len);
		channel->stat.incv(XscRudpChannelStatItem::XSC_RUDP_CHANNEL_RX_BYTES, len);
		static_pointer_cast<XscRudpLog>(this->server->log)->rx(channel.get(), channel->rbuf + channel->dlen, len);
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
		LOG_TRACE("rudp channel receive buffer full, can not read any more, we will close it, channel: %s", channel->toString().c_str())
		goto label;
	}
	if (flag) 
		return;
	label:
	LOG_TRACE("rudp channel disconnected: %s", channel->toString().c_str())
	if (!channel->est) 
	{
		LOG_TRACE("rudp channel already lost: %s", channel->toString().c_str())
		return;
	}
	this->delChannel(cid);
	channel->est = false;
	channel->evnDisc();
}

shared_ptr<XscRudpChannel> XscRudpWorker::findChannel(const string& cid)
{
	auto it = this->channel.find(cid);
	return it == this->channel.end() ? nullptr : it->second;
}

void XscRudpWorker::delChannel(const string& cid)
{
	if (this->channel.erase(cid) != 1)
	{
		LOG_FAULT("it`s a bug, can not found cid: %s", cid.c_str())
	}
	this->stat->inc(XscWorkerStatItem::XSC_WORKER_N2H_DESTORY);
}

void XscRudpWorker::dida(ullong now)
{
	this->timerMgr->dida(now);
}

void XscRudpWorker::checkChannelRt(ullong now)
{
	uint ts = now & 0xFFFFFFFFULL;
	shared_ptr<XscRudpCfg> cfg = static_pointer_cast<XscRudpCfg>(((XscRudpServer*) this->server)->cfg);
	for (auto& it : this->channel)
	{
		if (ikcp_check(it.second->kcp, ts) > ts)
			continue;
		ikcp_update(it.second->kcp, ts);
	}
}

void XscRudpWorker::checkHeartbeat(shared_ptr<XscRudpChannel> channel, int heartbeat )
{
	if (heartbeat < 1) 
		return;
	int hb = heartbeat * 3;
	this->timerMgr->addTimer(hb, [hb, channel]
	{
		if (!channel->est) 
		{
			LOG_DEBUG("rudp channel already lost: %s", channel->toString().c_str())
			return false;
		}
		if (Xsc::clock > channel->lts + (hb * 1000L))
		{
			LOG_DEBUG("rudp channel lost heart-beat, we will close it, channel: %s, elap: %llums", channel->toString().c_str(), Xsc::clock - channel->lts)
			channel->close();
			return false;
		}
		return true;
	});
}

int XscRudpWorker::kcpOutput(const char* buf, int len, void* kcp, void* peer)
{
	Net::udpSend(((XscRudpWorker*) Xsc::getXscWorker())->sfd, (uchar*) buf, len, (struct sockaddr_in*) peer);
	return 0;
}

XscRudpWorker::~XscRudpWorker()
{

}

