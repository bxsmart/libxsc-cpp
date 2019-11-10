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

#include "Xsc.h"
#include "../tcp/XscTcpServer.h"
#include "../tcp/XscTcpWorker.h"

static atomic_int __xscWorkerIndexSeq__(0); 
ullong Xsc::clock = DateMisc::nowGmt0(); 
pthread_key_t Xsc::pkey; 
vector<shared_ptr<XscWorker> > Xsc::xscWorker; 

Xsc::Xsc()
{

}

bool Xsc::init()
{
	::prctl(PR_SET_NAME, "xsc-main");
	::signal(SIGPIPE, SIG_IGN);
	::pthread_key_create(&Xsc::pkey, NULL);
	return true;
}

void Xsc::hold(function<void(ullong now)> cb)
{
	while (true)
	{
		Misc::sleep(1000); 
		Xsc::clock = DateMisc::nowGmt0();
		for (uint i = 0; i < Xsc::xscWorker.size(); ++i)
		{
			shared_ptr<XscWorker> wk = Xsc::xscWorker.at(i);
			wk->future([wk]
			{
				wk->dida(Xsc::clock);
			});
		}
		cb(Xsc::clock);
	}
}

int Xsc::getXscWorkIndex()
{
	XscWorker* xscWorker = (XscWorker*) ::pthread_getspecific(Xsc::pkey);
	return xscWorker == NULL ? -1 : xscWorker->wk;
}

XscWorker* Xsc::getXscWorker()
{
	return (XscWorker*) ::pthread_getspecific(Xsc::pkey);
}

XscTcpWorker* Xsc::getXscTcpWorker()
{
	return (XscTcpWorker*) Xsc::getXscWorker();
}

shared_ptr<XscTcpServer> Xsc::getXscTcpServer()
{
	return static_pointer_cast<XscTcpServer>(Xsc::getXscTcpWorker()->server->shared_from_this());
}

shared_ptr<XscServer> Xsc::getXscServer()
{
	return Xsc::getXscTcpWorker()->server->shared_from_this();
}

int Xsc::genXscWorkerIndex()
{
	return __xscWorkerIndexSeq__.fetch_add(1);
}

Xsc::~Xsc()
{

}

