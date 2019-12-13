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

#include "XscUdpCfg.h"

XscUdpCfg::XscUdpCfg()
{
	this->peerMtu = 0x10000;
	this->peerSndBuf = 0x10000;
	this->peerRcvBuf = 0x10000;
	this->tracing = false;
}

string XscUdpCfg::toString()
{
	string str;
	SPRINTF_STRING(&str, "addr: %s, worker: %u, peerMtu: %d, peerSndBuf: %d, peerRcvBuf: %d, tracing: %s", 
			this->addr.c_str(),
			this->worker,
			this->peerMtu,
			this->peerSndBuf,
			this->peerRcvBuf,
			this->tracing ? "true" : "false")
	return str;
}

XscUdpCfg::~XscUdpCfg()
{

}

