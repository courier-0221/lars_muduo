#ifndef ROUTERECHOSERVERSINGLECALCTHREAD_H
#define ROUTERECHOSERVERSINGLECALCTHREAD_H

#include "ICommonCallback.h"
#include "TcpServer.h"
#include "ThreadPool.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "CurrentThread.h"
#include "EndianCode.h"

#include <iostream>

//消息头的长度，固定值  id+len
#define MESSAGE_HEAD_LEN (8)

//消息头+消息体 最大长度限制
#define MESSAGE_LENGTH_LIMIT (65535 - MESSAGE_HEAD_LEN)

class RouterEchoServerSingleCalcThread : public IMuduoUser
{
public:
    RouterEchoServerSingleCalcThread(EventLoop* pLoop, NetAddress addr);
    ~RouterEchoServerSingleCalcThread();
    void start();
    virtual void onConnection(TcpConnectionPtr conn);
    virtual void onMessage(TcpConnectionPtr conn, Buffer* pBuf);
    virtual void onWriteComplate(TcpConnectionPtr conn);
    virtual void onClose(TcpConnectionPtr conn);
    
    //根据消息id匹配处理
    void msg01callback(MsgHead head, string &msgdata, TcpConnectionPtr conn);
    void msg02callback(MsgHead head, string &msgdata, TcpConnectionPtr conn);
    void msg03callback(MsgHead head, string &msgdata, TcpConnectionPtr conn);

private:
    EventLoop* _pLoop;
    TcpServer _pServer;
};

#endif
