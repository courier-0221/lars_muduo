#ifndef AGENTSERVER_H
#define AGENTSERVER_H

#include "DnsClient.h"
#include "RepoterClient.h"
#include "Manage.h"
#include "Common.h"
#include "TcpServer.h"
#include "EventLoop.h"
#include "ICommonCallback.h"
#include "ThreadPool.h"
#include "TcpConnection.h"
#include "CurrentThread.h"
#include "EndianCode.h"
#include <string>
#include <iostream>
#include "lars.pb.h"
using namespace std;

//消息头的长度，固定值  id+len
#define MESSAGE_HEAD_LEN (8)

class AgentServer : public IMuduoUser
{
public:
    AgentServer(EventLoop* pLoop, NetAddress addr);
    ~AgentServer();
    void start();
    virtual void onConnection(TcpConnectionPtr conn);
    virtual void onMessage(TcpConnectionPtr conn, Buffer* pBuf);
    virtual void onWriteComplate(TcpConnectionPtr conn);
    virtual void onClose(TcpConnectionPtr conn);
    //根据消息id匹配处理
    //内部通过计算返回给agentclient一个最佳的host信息
    void msgGetTheBestHostInfoCB(MsgHead head, string &msgdata, TcpConnectionPtr conn);
    //将节点调用信息上报到RepoterService
    void msgReportStatusCB(MsgHead head, string &msgdata, TcpConnectionPtr conn);
    //返回给agentclient一个module下所有的host信息
    void msgGetAllHostsCB(MsgHead head, string &msgdata, TcpConnectionPtr conn);

private:
    EventLoop* _pLoop;
    TcpServer _pServer;
    //ThreadPool _threadpool;

public:
    DnsClient _dnsClient;
    RepoterClient _repoterClient;
    Manage _manage;
};

#endif