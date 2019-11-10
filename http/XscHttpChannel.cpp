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

#include "XscHttpChannel.h"
#include "XscHttpServer.h"
#include "XscHttpCfg.h"
#include "../core/XscTimerMgr.h"

XscHttpChannel::XscHttpChannel(shared_ptr<XscHttpWorker> wk, int mtu, int cfd, const string &peer) :
		XscTcpChannel(ActorType::ACTOR_N2H, wk, mtu, cfd, peer)
{
	this->proType = XscProtocolType::XSC_PROTOCOL_HTTP;
	this->est = true;
	this->headerReady = false;
	this->rsp = false;
	this->headerLen = 0;
	this->expectedBodyLen = 0;
	this->bodyLen = 0;
	this->rnrn = 0;
}

void XscHttpChannel::sendJson(const string& json, shared_ptr<map<string, string>> ext)
{
	string str;
	SPRINTF_STRING(&str, "HTTP/1.1 200 OK\r\n")
	SPRINTF_STRING(&str, "Server: dev5-xsc\r\n")
	SPRINTF_STRING(&str, "Content-Type: application/json; charset=utf-8\r\n")
	if (ext != nullptr)
	{
		for (auto& it : *ext)
		{
			SPRINTF_STRING(&str, "%s: %s\r\n", it.first.c_str(), it.second.c_str())
		}
	}
	SPRINTF_STRING(&str, "Content-Length: %zu\r\n", json.length())
	SPRINTF_STRING(&str, "Connection: close\r\n")
	SPRINTF_STRING(&str, "\r\n")
	SPRINTF_STRING(&str, "%s", json.c_str())
	this->send((uchar*) str.data(), str.length());
	this->closeWait((XscTcpWorker*) this->worker);
}

void XscHttpChannel::sendBin(uchar* dat, int len, ullong contentLen, shared_ptr<map<string, string>> ext)
{
	string str;
	SPRINTF_STRING(&str, "HTTP/1.1 200 OK\r\n")
	SPRINTF_STRING(&str, "Server: dev5-xsc\r\n")
	SPRINTF_STRING(&str, "Content-Type: application/octet-stream\r\n")
	if (ext != nullptr)
	{
		for (auto& it : *ext)
		{
			SPRINTF_STRING(&str, "%s: %s\r\n", it.first.c_str(), it.second.c_str())
		}
	}
	SPRINTF_STRING(&str, "Content-Length: %llu\r\n", contentLen)
	SPRINTF_STRING(&str, "Connection: close\r\n")
	SPRINTF_STRING(&str, "\r\n")
	this->send((uchar*) str.data(), str.length());
	this->send(dat, len);
	if ((((ullong) len) & 0x00000000FFFFFFFF) == contentLen) 
		this->closeWait((XscTcpWorker*) this->worker);
}

void XscHttpChannel::sendBinContinue(uchar* dat, int len)
{
	this->send(dat, len);
}

void XscHttpChannel::sendBinNoMore(uchar* dat, int len)
{
	this->send(dat, len);
	this->closeWait((XscTcpWorker*) this->worker);
}

int XscHttpChannel::evnRecv(XscWorker* wk, uchar* dat, int len)
{
	int ret = !this->headerReady ? this->evnRecvHeader((XscHttpWorker*) wk, dat, len) : this->evnRecvBody((XscHttpWorker*) wk, dat, len);
	if (this->rsp) 
		return ret;
	if (ret < 0) 
	{
		string info = "DEV5-XSC: Forbidden";
		string str;
		SPRINTF_STRING(&str, "HTTP/1.1 403 Forbidden\r\n")
		SPRINTF_STRING(&str, "Server: dev5-xsc\r\n")
		SPRINTF_STRING(&str, "Content-Type: text/html; charset=utf-8\r\n")
		SPRINTF_STRING(&str, "Content-Length: %zu\r\n", info.length())
		SPRINTF_STRING(&str, "Connection: close\r\n")
		SPRINTF_STRING(&str, "\r\n")
		SPRINTF_STRING(&str, "%s", info.c_str())
		this->sendBinNoMore((uchar*) str.data(), str.length());
		return len;
	}
	return ret;
}

int XscHttpChannel::evnRecvHeader(XscHttpWorker* wk, uchar* dat, int len)
{
	int i = 0;
	for (; i < len; ++i)
	{
		switch (this->rnrn)
		{
		case 0: 
			this->rnrn = dat[i] == '\r' ? 1 : 0;
			break;
		case 1: 
			this->rnrn = dat[i] == '\n' ? 2 : (dat[i] == '\r' ? 1 : 0);
			break;
		case 2: 
			this->rnrn = dat[i] == '\r' ? 3 : 0;
			break;
		case 3: 
		{
			if (dat[i] != '\n') 
			{
				LOG_DEBUG("must be '\\n', this: %s", this->toString().c_str())
				return -1;
			}
			this->rnrn = 4;
			auto cfg = static_pointer_cast<XscHttpCfg>(((XscHttpServer*) (this->worker->server))->cfg);
			if (cfg->headerSize > 0 && this->headerLen >= cfg->headerSize)
			{
				LOG_DEBUG("over the http-header limit size(%dbytes), we will close this channel, this: %s", cfg->headerSize, this->toString().c_str())
				return -1;
			}
			++(this->headerLen);
			this->cache4header.push_back(dat[i]);
			if (!this->decodeHeader()) 
				return -1;
			if (!this->checkHeader()) 
				return -1;
			this->headerReady = true;
			if (!this->evnHeader(wk, this->header)) 
				return -1;
			return this->method == "POST" ? (this->evnRecvBody(wk, dat + i + 1, len - i - 1) == -1 ? -1 : len) : len; 
		}
			break;
		default:
			LOG_FAULT("it`s a bug, rnrn: %d, cache: %zu, this: %s", this->rnrn, this->cache4header.size(), this->toString().c_str())
			break;
		}
		auto cfg = static_pointer_cast<XscHttpCfg>(((XscHttpServer*) (this->worker->server))->cfg);
		if (cfg->headerSize > 0 && this->headerLen >= cfg->headerSize)
		{
			LOG_DEBUG("over the http-header limit size(%dbytes), we will close this channel, this: %s", cfg->headerSize, this->toString().c_str())
			return -1;
		}
		++this->headerLen;
		this->cache4header.push_back(dat[i]);
	}
	return len;
}

int XscHttpChannel::evnRecvBody(XscHttpWorker* wk, uchar* dat, int len)
{
	if (len < 1) 
		return len;
	this->bodyLen += len;
	if (this->bodyLen > this->expectedBodyLen)
	{
		LOG_DEBUG("over the 'Content-Length': (%llubytes), we will close this channel, this: %s", this->expectedBodyLen, this->toString().c_str())
		return -1;
	}
	return this->evnBody(wk, dat, len, this->bodyLen != this->expectedBodyLen) ? len : -1;
}

bool XscHttpChannel::decodeHeader()
{
	bool ret = true;
	bool r = false;
	int pos = 0;
	for (size_t i = 0; i < this->cache4header.size(); ++i)
	{
		uchar by = this->cache4header[i];
		if (by == '\r')
		{
			r = true;
			continue;
		}
		if (!r || by != '\n')
			continue;
		string_view line((char*) this->cache4header.data() + pos, i - 1 - pos);
		if (pos == 0) 
		{
			string::size_type indx0 = line.find(" "); 
			if (indx0 == string::npos)
			{
				LOG_DEBUG("http header first line format error: %s, this: %s", (char* )line.data(), this->toString().c_str())
				ret = false;
				break;
			}
			this->method.assign(line.data(), indx0);
			if (this->method != "GET" && this->method != "POST")
			{
				LOG_DEBUG("support 'GET' and 'POST' only, this: %s", this->toString().c_str())
				ret = false;
				break;
			}
			string::size_type indx1 = line.find(" ", indx0 + 1); 
			if (indx1 == string::npos)
			{
				LOG_DEBUG("http header first line format error: %s, this: %s", (char* )line.data(), this->toString().c_str())
				ret = false;
				break;
			}
			this->path.assign(line.data() + indx0 + 1, indx1 - indx0 - 1);
			this->ver.assign(line.data() + indx1 + 1, line.length() - indx1 - 1);
			r = false;
			pos = i + 1;
			continue;
		}
		if (line.length() < 3) 
			break;
		string::size_type indx = line.find(": ");
		if (indx == string::npos)
		{
			LOG_DEBUG("http header format error: %s, this: %s", (char* )line.data(), this->toString().c_str())
			ret = false;
			break;
		}
		string key(line.data(), indx);
		string val(line.data() + indx + 2, line.length() - indx - 2);
		this->header[key] = val;
		r = false;
		pos = i + 1;
	}
	this->cache4header.clear();
	return ret ? (!(this->method.empty() || this->path.empty() || this->ver.empty())) : ret;
}

bool XscHttpChannel::checkHeader()
{
	auto cfg = static_pointer_cast<XscHttpCfg>(((XscHttpServer*) (this->worker->server))->cfg);
	if (this->method == "POST")
	{
		auto it = this->header.find("Content-Length");
		if (it == this->header.end())
		{
			LOG_DEBUG("header 'Content-Length' is required, this: %s", this->toString().c_str())
			return false;
		}
		this->expectedBodyLen = ::atoll(it->second.c_str());
		if (cfg->bodySize > 0 && (this->expectedBodyLen > cfg->bodySize))
		{
			LOG_DEBUG("over the http-body limit size(%dbytes), 'Content-Length': %llubytes, we will close this channel, this: %s", cfg->bodySize, this->expectedBodyLen, this->toString().c_str())
			return false;
		}
	}
	for (auto& it : cfg->requiredHeader)
	{
		if (this->header.find(it) == this->header.end()) 
		{
			LOG_DEBUG("missing required http-header: %s, this: %s", it.c_str(), this->toString().c_str())
			return false;
		}
	}
	return true;
}

uint XscHttpChannel::getHeaderLen()
{
	return this->headerLen;
}

ullong XscHttpChannel::getContentLength()
{
	return this->expectedBodyLen;
}

ullong XscHttpChannel::getBodyLen()
{
	return this->bodyLen;
}

string XscHttpChannel::getHeader(const string& key)
{
	auto it = this->header.find(key);
	return it == this->header.end() ? "" : it->second;
}

void XscHttpChannel::closeWait(XscTcpWorker* wk)
{
	auto cfg = static_pointer_cast<XscHttpCfg>(((XscHttpServer*) (this->worker->server))->cfg);
	auto channel = static_pointer_cast<XscHttpChannel>(this->shared_from_this());
	wk->timerMgr->addTimerOneTime(cfg->closeWait, [channel] 
	{
		if (!channel->est)
		{
			return;
		}
		LOG_TRACE("close this http channel: %s", channel->toString().c_str())
		channel->close();
	});
}

XscHttpChannel::~XscHttpChannel()
{

}
