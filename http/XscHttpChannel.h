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

#ifndef HTTP_XSCHTTPCHANNEL_H_
#define HTTP_XSCHTTPCHANNEL_H_

#if !defined (__LIBXSC_H__) && !defined (LIBXSC)
#error only libxsc.h can be included directly.
#endif

#include "XscHttpWorker.h"
#include "../tcp/XscTcpChannel.h"

class XscHttpChannel: public XscTcpChannel
{
public:
	void sendJson(const string& json, shared_ptr<map<string, string>> ext = nullptr); 
	void sendBin(uchar* dat, int len, ullong contentLen, shared_ptr<map<string, string>> ext = nullptr); 
	void sendBinContinue(uchar* dat, int len); 
	void sendBinNoMore(uchar* dat, int len); 
public:
	uint getHeaderLen(); 
	ullong getContentLength(); 
	ullong getBodyLen(); 
	string getHeader(const string& key); 
	XscHttpChannel(XscHttpWorker* wk, int mtu, int cfd, const string &peer);
	virtual ~XscHttpChannel();
public:
	virtual bool evnHeader(XscHttpWorker* wk, map<string, string>& header) = 0; 
	virtual bool evnBody(XscHttpWorker* wk, uchar* dat, int len, bool more) = 0; 
private:
	int evnRecv(XscWorker* wk, uchar* dat, int len); 
	int evnRecvHeader(XscHttpWorker* wk, uchar* dat, int len); 
	int evnRecvBody(XscHttpWorker* wk, uchar* dat, int len); 
	bool decodeHeader(); 
	bool checkHeader(); 
	void closeWait(XscTcpWorker* wk); 
protected:
	bool headerReady; 
	bool rsp; 
	int rnrn; 
	string method; 
	string path; 
	string ver; 
	map<string, string> header; 
	uint headerLen; 
	ullong expectedBodyLen; 
	ullong bodyLen; 
	vector<uchar> cache4header; 
};

#endif 
