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

#ifndef WS_XSCWEBSOCKETCHANNEL_H_
#define WS_XSCWEBSOCKETCHANNEL_H_

#include "XscWebSocketWorker.h"
#include "../tcp/XscTcpChannel.h"

#define WS_FRAME_CONTINUATION								0x00
#define WS_FRAME_TEXT										0x01
#define WS_FRAME_BINARY										0x02
#define WS_FRAME_CLOSE										0x08
#define WS_FRAME_PING										0x09
#define WS_FRAME_PONG										0x0A

class XscWebSocketChannel: public XscTcpChannel
{
public:
	void send(uchar* dat, int len); 
public:
	XscWebSocketChannel(shared_ptr<XscWebSocketWorker> wk, int mtu, int cfd, const string &peer);
	virtual ~XscWebSocketChannel();
};

#endif 
