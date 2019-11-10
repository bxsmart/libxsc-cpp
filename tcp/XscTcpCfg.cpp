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

#include "XscTcpCfg.h"

XscTcpCfg::XscTcpCfg() :
		XscCfg()
{
	this->addr = "0.0.0.0:1224";
	this->worker = 4;
	this->peerLimit = 1024;
	this->peerMtu = 0x10000;
	this->peerRcvBuf = 0x10000;
	this->peerSndBuf = 0x10000;
	this->lazyClose = 10;
	this->tracing = false;
	this->heartbeat = 15;
	this->n2hZombie = 10;
	this->n2hTransTimeout = 15;
	this->n2hTracing = false;
	this->h2nReConn = 3;
	this->h2nTransTimeout = 15;
}

string XscTcpCfg::toString()
{
	string str;
	SPRINTF_STRING(&str, "addr: %s, worker: %u, peerLimit: %u, peerMtu: %u, peerRcvBuf: %u, peerSndBuf: %u, lazyClose: %u, tracing: %s, heartbeat: %u, n2hZombie: %u, n2hTransTimeout: %u, n2hTracing: %s", 
			this->addr.c_str(),
			this->worker,
			this->peerLimit,
			this->peerMtu,
			this->peerRcvBuf,
			this->peerSndBuf,
			this->lazyClose,
			this->tracing ? "true" : "false",
			this->heartbeat,
			this->n2hZombie,
			this->n2hTransTimeout,
			this->n2hTracing ? "true" : "false")
	SPRINTF_STRING(&str, ", h2nReConn: %u, h2nTransTimeout: %u", this->h2nReConn, this->h2nTransTimeout)
	return str;
}

XscTcpCfg::~XscTcpCfg()
{

}

