#ifndef REPOTERCLIENT_H_
#define REPOTERCLIENT_H_

#include "Common.h"
#include "TcpClient.h"
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

class RepoterClient : public IMuduoUser ,private noncopyable
{
public:
    RepoterClient(NetAddress addr);
    ~RepoterClient();
    void start();
    virtual void onConnection(TcpConnectionPtr conn);
    virtual void onMessage(TcpConnectionPtr conn, Buffer* pBuf);
    virtual void onWriteComplate(TcpConnectionPtr conn);
    virtual void onClose(TcpConnectionPtr conn);
    //获取与对应server通信的connection 进行发送处理
    TcpConnectionPtr getConnection() const { return _client.connection(); }
private:
    void threadWorkTask(void);//线程工作任务
private:
    EventLoop _loop;
    TcpClient _client;
    Thread _thread;
};

#endif