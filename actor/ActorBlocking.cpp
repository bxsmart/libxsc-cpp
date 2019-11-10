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

#include "ActorBlocking.h"

ActorBlocking::ActorBlocking(const string& name, int pool) :
		Actor(ActorType::ACTOR_BLOCKING, INVALID)
{
	this->name = name;
	this->busy = false;
	this->size = 0;
	::pthread_mutex_init(&this->lock, NULL);
	::pthread_cond_init(&this->cond, NULL);
	for (int i = 0; i < pool; ++i)
	{
		pthread_t t;
		if (::pthread_create(&t, NULL, ActorBlocking::svc, this) == -1)
		{
			LOG_FAULT("can not create thread any more.")
			return;
		}
		::pthread_detach(t);
	}
}

void* ActorBlocking::svc(void* arg)
{
	ActorBlocking* ab = (ActorBlocking*) arg;
	::prctl(PR_SET_NAME, ab->name.c_str());
	LOG_INFO("actor-blocking thread start successful: %s", ab->name.c_str())
	queue<function<void()>> cbs;
	while (1)
	{
		ab->busy = false;
		::pthread_mutex_lock(&ab->lock);
		while (ab->afs.empty())
			::pthread_cond_wait(&ab->cond, &ab->lock);
		cbs.push(ab->afs.front());
		ab->afs.pop();
		ab->size -= 1;
		::pthread_mutex_unlock(&ab->lock);
		ab->busy = true;
		while (!cbs.empty())
		{
			cbs.front()();
			cbs.pop();
		}
	}
	return NULL;
}

void ActorBlocking::push(function<void()> cb)
{
	bool nll = false;
	pthread_mutex_lock(&this->lock);
	nll = this->afs.empty();
	this->afs.push(cb);
	this->size += 1;
	pthread_mutex_unlock(&this->lock);
	if (nll)
		pthread_cond_signal(&this->cond);
}

bool ActorBlocking::isBusy()
{
	return this->busy;
}

ActorBlocking::~ActorBlocking()
{

}

