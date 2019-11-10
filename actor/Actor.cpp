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

#include "Actor.h"
#include "ActorBlocking.h"
#include "../core/Xsc.h"
#include "../core/XscWorker.h"

Actor::Actor(ActorType type, int wk)
{
	this->type = type;
	this->wk = wk;
}

void Actor::future(function<void()> cb)
{
	if (this->type == ActorType::ACTOR_BLOCKING)
	{
		((ActorBlocking*) this)->push(cb);
		return;
	}
	if (this->wk == INVALID)
	{
		LOG_FAULT("it`s a bug, worker-index: %d, this: %s", this->wk, this->toString().c_str())
		return;
	}
	if (this->wk == Xsc::getXscWorkIndex()) 
	{
		cb();
		return;
	}
	Xsc::xscWorker.at(this->wk)->push(cb); 
}

Actor::~Actor()
{

}

