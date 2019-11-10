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

#ifndef LIBXSC_H_
#define LIBXSC_H_

#define __LIBXSC_H__

#include "actor/Actor.h"
#include "actor/ActorBlocking.h"
#include "actor/ActorBlockingSingleThread.h"
#include "core/Xsc.h"
#include "core/XscCfg.h"
#include "core/XscChannel.h"
#include "core/XscMisc.h"
#include "core/XscMsgMgr.h"
#include "core/XscServer.h"
#include "core/XscTimer.h"
#include "core/XscTimerMgr.h"
#include "core/XscUsr.h"
#include "core/XscWorker.h"
#include "core/XscWorkerStat.h"
#include "http/XscHttpCfg.h"
#include "http/XscHttpChannel.h"
#include "http/XscHttpChannelStat.h"
#include "http/XscHttpLog.h"
#include "http/XscHttpServer.h"
#include "http/XscHttpWorker.h"
#include "tcp/XscTcpCfg.h"
#include "tcp/XscTcpChannel.h"
#include "tcp/XscTcpChannelStat.h"
#include "tcp/XscTcpLog.h"
#include "tcp/XscTcpServer.h"
#include "tcp/XscTcpWorker.h"
#include "ws/XscWebSocketCfg.h"
#include "ws/XscWebSocketChannel.h"
#include "ws/XscWebSocketLog.h"
#include "ws/XscWebSocketServer.h"
#include "ws/XscWebSocketWorker.h"

#endif 
