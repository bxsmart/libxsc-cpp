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

#ifndef XSCWORKER_H_
#define XSCWORKER_H_

#include "XscMsgMgr.h"
#include "XscWorkerStat.h"
#include "../actor/Actor.h"

class XscServer;
class XscTimerMgr;

class XscWorker: public Actor
{
public:
	volatile bool busy; 
	int evn; 
	int maxFdSize; 
	int mtu; 
	XscServer* server; 
	shared_ptr<XscTimerMgr> timerMgr; 
	shared_ptr<XscWorkerStat> stat; 
	shared_ptr<XscMsgMgr> msgMgr; 
	pthread_t pt; 
	pthread_mutex_t mutex; 
	queue<function<void()>> afs; 
public:
	virtual void dida(ullong now) = 0; 
public:
	void push(function<void()> cb); 
	XscWorker(shared_ptr<XscServer> server);
	virtual ~XscWorker();
};

#endif 
