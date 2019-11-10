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

#include "XscWorker.h"
#include "XscServer.h"
#include "XscTimerMgr.h"
#include "XscWorkerStat.h"

XscWorker::XscWorker(shared_ptr<XscServer> server) :
		Actor(ActorType::ACTOR_ITC, INVALID)
{
	this->server = server.get();
	this->timerMgr = nullptr;
	this->stat.reset(new XscWorkerStat());
	this->pt = 0;
	this->maxFdSize = 0;
	this->mtu  = 0;
	this->evn = ::eventfd(0, EFD_NONBLOCK);
	::pthread_mutex_init(&this->mutex, NULL);
}

void XscWorker::push(function<void()> cb)
{
	::pthread_mutex_lock(&this->mutex);
	this->afs.push(cb);
	::pthread_mutex_unlock(&this->mutex);
	static ullong count = 1;
	::write(this->evn, &count, sizeof(ullong));
}

XscWorker::~XscWorker()
{

}

