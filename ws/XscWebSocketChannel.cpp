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

#include "XscWebSocketChannel.h"

XscWebSocketChannel::XscWebSocketChannel(shared_ptr<XscWebSocketWorker> wk, int mtu, int cfd, const string &peer) :
		XscTcpChannel(ActorType::ACTOR_N2H, wk, mtu, cfd, peer)
{
	this->proType = XscProtocolType::XSC_PROTOCOL_WEBSOCKET;
	this->est = true;
}

void XscWebSocketChannel::send(uchar* dat, int len)
{
	if (len < 0x7E) 
	{
		int size = len + 2;
		uchar* buf = (uchar*) ::malloc(size);
		buf[0] = 0x82;
		buf[1] = (uchar) size;
		::memcpy(buf + 2, dat, len);
		XscTcpChannel::send(buf, size);
		::free(buf);
		return;
	}
	if (len < 0x10000) 
	{
		int size = len + 4;
		uchar* buf = (uchar*) ::malloc(size);
		buf[0] = 0x82;
		buf[1] = 0x7E;
		ushort xx = ::ntohs(len);
		::memcpy(buf + 2, &xx, 2);
		::memcpy(buf + 4, dat, len);
		XscTcpChannel::send(buf, size);
		::free(buf);
		return;
	}
	int size = len + 10; 
	uchar* buf = (uchar*) ::malloc(size);
	dat[0] = 0x82;
	dat[1] = 0x7F;
	uint xx = ::ntohl(len);
	dat[2] = 0;
	dat[3] = 0;
	dat[4] = 0;
	dat[5] = 0;
	::memcpy(buf + 6, &xx, 4);
	::memcpy(buf + 10, dat, len);
	XscTcpChannel::send(buf, size);
	::free(buf);
}

XscWebSocketChannel::~XscWebSocketChannel()
{

}

