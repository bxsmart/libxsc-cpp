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

#ifndef RUDP_XSCRUDPCHANNELSTAT_H_
#define RUDP_XSCRUDPCHANNELSTAT_H_

#include <libmisc.h>

enum XscRudpChannelStatItem
{
	XSC_RUDP_CHANNEL_RX_BYTES = 0x00, 
	XSC_RUDP_CHANNEL_RX_MSGS, 
	XSC_RUDP_CHANNEL_TX_BYTES, 
	XSC_RUDP_CHANNEL_TX_MSGS, 
	XSC_RUDP_CHANNEL_STAT_END
};

class XscRudpChannelStat
{
public:
	void inc(uint item); 
	void incv(uint item, ullong v); 
	ullong get(uint item); 
	void clear(); 
public:
	XscRudpChannelStat();
	virtual ~XscRudpChannelStat();
private:
	atomic_ullong items[XscRudpChannelStatItem::XSC_RUDP_CHANNEL_STAT_END] = { 0 };
};

#endif 
