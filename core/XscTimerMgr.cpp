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

#include "XscTimerMgr.h"
#include "Xsc.h"
#include "XscServer.h"

XscTimerMgr::XscTimerMgr(XscWorker* xscWorker) :
		Actor(ActorType::ACTOR_ITC, xscWorker->wk)
{
	this->xscWorker = xscWorker;
	this->tid = 0;
	this->slot = 0;
	this->lts = Xsc::clock;
	for (int i = 0; i < XSC_TIMER_TICKS; ++i)
		this->wheel.push_back(new unordered_map<uint, XscTimer*>());
}


void XscTimerMgr::dida(ullong now)
{
	if ((time_t) (now - this->lts) < DateMisc::sec) 
		return;
	this->lts = now;
	this->loop(); 
	this->slot += 1; 
	this->slot = this->slot == XSC_TIMER_TICKS ? 0 : this->slot;
}

uint XscTimerMgr::addTimer(int sec , function<bool(void)> cb )
{
	int indx = Xsc::getXscWorkIndex();
	if (indx == -1 || indx != this->xscWorker->wk)
	{
		LOG_FAULT("it`s a bug, index: %d, wk: %d, stack: %s", indx, this->xscWorker->wk, Misc::getStack().c_str());
		return 0;
	}
	return this->add(sec, cb);
}

uint XscTimerMgr::addTimerOneTime(int sec , function<void(void)> cb )
{
	function<bool(void)> xcb = [cb]
	{
		cb();
		return false;
	};
	return this->addTimer(sec, xcb);
}


void XscTimerMgr::loop()
{
	unordered_map<uint, XscTimer*>* solt = this->wheel.at(this->slot);
	list<pair<uint, XscTimer*>> lis;
	for (auto& it : *solt) 
		lis.push_back(make_pair(it.first, it.second));
	list<pair<uint, XscTimer*>> tmp; 
	for (auto& it : lis)
	{
		if (it.second->loop == 0) 
		{
			if (it.second->cb()) 
				tmp.push_back(make_pair(it.first, it.second));
			else
				delete it.second;
			solt->erase(it.first); 
		} else
			it.second->loop -= 1;
	}
	for (auto& it : tmp) 
	{
		int m = it.second->sec % XSC_TIMER_TICKS;
		int pos = this->slot + m;
		it.second->slot = pos < XSC_TIMER_TICKS ? pos : (pos - XSC_TIMER_TICKS) ;
		this->wheel.at(it.second->slot)->insert(make_pair(it.first, it.second));
	}
}

uint XscTimerMgr::add(int sec , function<bool(void)> cb )
{
	int m = sec % XSC_TIMER_TICKS;
	int pos = this->slot + m;
	XscTimer* xt = new XscTimer(sec, (sec / XSC_TIMER_TICKS), pos < XSC_TIMER_TICKS ? pos : (pos - XSC_TIMER_TICKS) , cb);
	unordered_map<uint, XscTimer*>* solt = this->wheel.at(xt->slot);
	solt->insert(make_pair(++(this->tid), xt));
	return this->tid;
}

string XscTimerMgr::toString()
{
	return "xsc-timer-manager";
}

XscTimerMgr::~XscTimerMgr()
{

}

