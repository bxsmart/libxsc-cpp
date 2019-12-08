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

#ifndef XSCSELECTOR_H_
#define XSCSELECTOR_H_

#include "XscWorker.h"
#include "XscMsgMgr.h"
#include "XscJoinCounter.h"

class XscLog;
class XscCfg;

enum XscProtocolType
{
	XSC_PROTOCOL_TCP = 0x00, 
	XSC_PROTOCOL_UDP, 
	XSC_PROTOCOL_RUDP, 
	XSC_PROTOCOL_SCTP, 
	XSC_PROTOCOL_HTTP, 
	XSC_PROTOCOL_WEBSOCKET, 
};

class XscServer: public enable_shared_from_this<XscServer>
{
public:
	string ne; 
	XscProtocolType proType; 
	atomic_int rrSeq; 
	shared_ptr<XscLog> log; 
	shared_ptr<XscJoinCounter> waitWorker; 
	vector<shared_ptr<XscWorker>> xscWorker; 
	bool tracing; 
	bool n2hTracing; 
public:
	virtual bool startup(shared_ptr<XscCfg> cfg) = 0; 
	virtual bool publish() = 0; 
public:
	shared_ptr<XscWorker> rr(); 
	XscServer(const string& ne , shared_ptr<XscLog> log);
	virtual ~XscServer();
public:
	static void add(const string& name, shared_ptr<XscServer> server); 
	static shared_ptr<XscServer> get(const string& name); 
	static void names(list<string>& lis); 
private:
	static unordered_map<string, shared_ptr<XscServer>> server; 
};

#endif 
