#ifndef REPORTERSERVICE_H
#define REPORTERSERVICE_H

#include "StoreReport.h"
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

class ReporterService : public IMuduoUser
{
public:
    ReporterService(EventLoop* pLoop, NetAddress addr);
    ~ReporterService();
    void start();
    virtual void onConnection(TcpConnectionPtr conn);
    virtual void onMessage(TcpConnectionPtr conn, Buffer* pBuf);
    virtual void onWriteComplate(TcpConnectionPtr conn);
    virtual void onClose(TcpConnectionPtr conn);
    //根据消息id匹配处理
    void msgGetReportStatusCB(MsgHead head, string &msgdata, TcpConnectionPtr conn);
private:
    void threadPoolWorkTask(lars::ReportStatusRequest req);//线程池工作任务，用于调用入库操作
private:
    EventLoop* _pLoop;
    TcpServer _pServer;
    StoreReport _storeReport;
    ThreadPool _threadpool;
};

#endif