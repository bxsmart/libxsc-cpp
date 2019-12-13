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

#include "XscRudpServer.h"
#include "XscRudpCfg.h"
#include "XscRudpWorker.h"
#include "../core/Xsc.h"

XscRudpServer::XscRudpServer(const string& ne, shared_ptr<XscRudpLog> log) :
		XscUdpServer(ne, log)
{
	this->proType = XscProtocolType::XSC_PROTOCOL_RUDP;
}

bool XscRudpServer::startup(shared_ptr<XscCfg> c)
{
	shared_ptr<XscRudpCfg> cfg = static_pointer_cast<XscRudpCfg>(c);
	if (cfg->worker < 1)
	{
		LOG_ERROR("rudp server cfg error: %s", cfg->toString().c_str())
		return false;
	}
	this->cfg = cfg;
	this->tracing = cfg->tracing;
	this->n2hTracing = cfg->tracing;
	this->waitWorker.reset(new XscJoinCounter(this->cfg->worker));
	for (uint i = 0; i < this->cfg->worker; ++i)
	{
		shared_ptr<XscRudpWorker> worker(new XscRudpWorker(this));
		worker->wk = Xsc::genXscWorkerIndex();
		this->xscWorker.push_back(worker);
		Xsc::xscWorker.push_back(worker);
		thread t([worker]()
		{
			worker->loop();
		});
		t.detach();
	}
	this->waitWorker->wait();
	LOG_INFO("xsc udp server startup successful, server-name: %s, addr: %s, worker: %d", this->ne.c_str(), this->cfg->addr.c_str(), this->cfg->worker)
	shared_ptr<XscRudpServer> server = static_pointer_cast<XscRudpServer>(this->shared_from_this());
	thread t([server]()
	{
		server->checkChannelRt();
	});
	t.detach();
	return true;
}

bool XscRudpServer::publish()
{
	string ip;
	int port;
	if (!Net::str2ipAndPort(this->cfg->addr.c_str(), &ip, &port))
	{
		LOG_FAULT("host format error: %s", this->cfg->addr.c_str())
		return false;
	}
	for (uint i = 0; i < this->cfg->worker; ++i)
	{
		shared_ptr<XscRudpWorker> worker = static_pointer_cast<XscRudpWorker>(this->xscWorker.at(i));
		worker->future([worker, ip, port]
		{
			worker->publish(ip, port);
		});
	}
	return true;
}

void XscRudpServer::checkChannelRt()
{
	shared_ptr<XscRudpCfg> cfg = static_pointer_cast<XscRudpCfg>(this->cfg);
	while (true)
	{
		Misc::sleep(cfg->kcpInterval);
		ullong now = DateMisc::nowGmt0();
		for (uint i = 0; i < this->cfg->worker; ++i)
		{
			shared_ptr<XscRudpWorker> worker = static_pointer_cast<XscRudpWorker>(this->xscWorker.at(i));
			worker->future([worker, now]
			{
				worker->checkChannelRt(now);
			});
		}
	}
}

XscRudpServer::~XscRudpServer()
{

}
