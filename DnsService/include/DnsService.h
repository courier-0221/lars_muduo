#ifndef DNSSERVICE_H
#define DNSSERVICE_H

#include "DnsSubscribe.h"
#include "DnsRoute.h"
#include "DnsCommon.h"
#include "ICommonCallback.h"
#include "TcpServer.h"
#include "ThreadPool.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "CurrentThread.h"
#include "EndianCode.h"
#include "lars.pb.h"

class DnsRoute;
class DnsSubscribe;

class DnsService : public IMuduoUser
{
public:
    DnsService(EventLoop* pLoop, NetAddress addr);
    ~DnsService();
    void start();
    virtual void onConnection(TcpConnectionPtr conn);
    virtual void onMessage(TcpConnectionPtr conn, Buffer* pBuf);
    virtual void onWriteComplate(TcpConnectionPtr conn);
    virtual void onClose(TcpConnectionPtr conn);
    
    //根据消息id匹配处理
    void msgGetRouteCB(MsgHead head, string &msgdata, TcpConnectionPtr conn);
    OnlineClient& getAllOnlineClient(){ return _onClient; }
private:
    //线程池工作任务，查询数据库并构造数据返回给客户端
    void threadPoolWorkTask(lars::GetRouteRequest req, TcpConnectionPtr conn);

private:
    EventLoop* _pLoop;
    TcpServer _pServer;
    OnlineClient _onClient; //在线客户端
    RequestModMap _requesModLog;    //在线客户端请求过的model
public:
    ThreadPool _threadpool;
    ClientMap _clientMap;   //fd 与 TcpConnectionPtr 对应关系
    DnsRoute _dnsRoute;
    DnsSubscribe _dnsSubcribe;
};

#endif
