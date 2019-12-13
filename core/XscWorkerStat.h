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

#ifndef CORE_XSCWORKERSTAT_H_
#define CORE_XSCWORKERSTAT_H_

#if !defined (__LIBXSC_H__) && !defined (LIBXSC)
#error only libxsc.h can be included directly.
#endif

#include <libmisc.h>

enum XscWorkerStatItem
{
	XSC_WORKER_RX_BYTES = 0x00, 
	XSC_WORKER_RX_MSGS, 
	XSC_WORKER_TX_BYTES, 
	XSC_WORKER_TX_MSGS, 
	XSC_WORKER_N2H_TOTAL, 
	XSC_WORKER_N2H_DESTORY, 
	XSC_WORKER_STAT_END
};

class XscWorkerStat
{
public:
	void inc(uint item); 
	void incv(uint item, ullong v); 
	ullong get(uint item); 
public:
	XscWorkerStat();
	virtual ~XscWorkerStat();
private:
	atomic_ullong items[XscWorkerStatItem::XSC_WORKER_STAT_END] = { 0 };
};

#endif 
