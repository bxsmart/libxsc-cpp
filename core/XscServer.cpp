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

#include "Xsc.h"
#include "XscServer.h"
#include "XscLog.h"

unordered_map<string, shared_ptr<XscServer>> XscServer::server; 

XscServer::XscServer(const string& ne , shared_ptr<XscLog> log)
{
	this->ne = ne;
	this->log = log;
	this->proType = (XscProtocolType) 0;
	this->rrSeq.store(0);
	this->waitWorker = nullptr;
	this->tracing = false;
	this->n2hTracing = false;
}

shared_ptr<XscWorker> XscServer::rr()
{
	return this->xscWorker.at((this->rrSeq.fetch_add(1) & 0x00FFFF) % this->xscWorker.size());
}

void XscServer::add(const string& name, shared_ptr<XscServer> server)
{
	XscServer::server[name] = server; 
}

shared_ptr<XscServer> XscServer::get(const string& name)
{
	auto it = XscServer::server.find(name);
	return it == XscServer::server.end() ? nullptr : it->second;
}

void XscServer::names(list<string>& lis)
{
	for (auto& it : XscServer::server)
		lis.push_back(it.first);
}

XscServer::~XscServer()
{

}

