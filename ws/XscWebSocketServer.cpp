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

#include "XscWebSocketServer.h"
#include "XscWebSocketCfg.h"
#include "../core/Xsc.h"

XscWebSocketServer::XscWebSocketServer(const string& ne, shared_ptr<XscWebSocketLog> log) :
		XscTcpServer(ne, log)
{
	this->proType = XscProtocolType::XSC_PROTOCOL_WEBSOCKET;
}

bool XscWebSocketServer::startup(shared_ptr<XscCfg> c)
{
	shared_ptr<XscWebSocketCfg> cfg = static_pointer_cast<XscWebSocketCfg>(c);
	if (cfg->worker < 1)
	{
		LOG_ERROR("web socket server cfg error: %s", cfg->toString().c_str())
		return false;
	}
	this->cfg = cfg;
	this->tracing = cfg->tracing;
	this->n2hTracing = cfg->n2hTracing;
	this->waitWorker.reset(new XscJoinCounter(this->cfg->worker));
	for (uint i = 0; i < this->cfg->worker; ++i)
	{
		shared_ptr<XscWebSocketWorker> worker(new XscWebSocketWorker(static_pointer_cast<XscWebSocketServer>(this->shared_from_this()), this->cfg->peerLimit / this->cfg->worker ));
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
	LOG_INFO("xsc web socket server startup successful, addr: %s, worker: %d", this->cfg->addr.c_str(), this->cfg->worker)
	return true;
}

bool XscWebSocketServer::publish()
{
	string ip;
	int port;
	if (!Net::str2ipAndPort(this->cfg->addr.c_str(), &ip, &port))
	{
		LOG_FAULT("host format error: %s", this->cfg->addr.c_str())
		return false;
	}
	int sock = Net::tcpConnectNoBlocking(ip.c_str(), port, 1);
	if (sock > 0) 
	{
		LOG_ERROR("address already on listen: %s", this->cfg->addr.c_str())
		Net::close(sock);
		return false;
	}
	for (uint i = 0; i < this->cfg->worker; ++i)
	{
		shared_ptr<XscWebSocketWorker> worker = static_pointer_cast<XscWebSocketWorker>(this->xscWorker.at(i));
		worker->future([worker, ip, port]
		{
			worker->publish(ip, port);
		});
	}
	return true;
}
XscWebSocketServer::~XscWebSocketServer()
{

}

