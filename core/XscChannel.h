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

#ifndef CORE_XSCCHANNEL_H_
#define CORE_XSCCHANNEL_H_

#if !defined (__LIBXSC_H__) && !defined (LIBXSC)
#error only libxsc.h can be included directly.
#endif

#include "XscServer.h"
#include "XscUsr.h"

class XscChannel: public Actor
{
public:
	XscProtocolType proType; 
	bool est; 
	int cfd; 
	string peer; 
	XscWorker* worker; 
	weak_ptr<XscUsr> usr; 
	ullong gts; 
	ullong lts; 
public:
	void setXscUsr(shared_ptr<XscUsr> usr); 
	void incMsg(); 
	XscChannel(XscProtocolType proType, ActorType aType, XscWorker* wk, int cfd, const string& peer);
	virtual ~XscChannel();
public:
	virtual void send(uchar* dat, int len) = 0; 
	virtual void close() = 0; 
	virtual void closeSlient() = 0; 
	virtual void lazyClose() = 0; 
	virtual void dida(ullong now) = 0; 
};

#endif 

