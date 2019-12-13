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

#include "XscRudpCfg.h"

XscRudpCfg::XscRudpCfg() :
		XscUdpCfg()
{
	this->lazyClose = 5;
	this->heartbeat = 15;
	this->n2hZombie = 10;
	this->n2hTransTimeout = 15;
	this->kcpNodelay = true;
	this->kcpInterval = 10;
	this->kcpResend = 2;
	this->kcpNc = false;
	this->kcpRcvWind = 0x80;
	this->kcpSndWind = 0x80;
	this->kcpMtu = 1400;
	this->kcpMinRto = 10;
}

XscRudpCfg::~XscRudpCfg()
{

}

