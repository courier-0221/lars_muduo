#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "Common.h"
#include "ICommonCallback.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "Connector.h"
#include "Router.h"

class TcpClient : noncopyable, public IConnectorCallback
{
public:
    TcpClient(EventLoop* loop, const NetAddress& serverAddr);
    ~TcpClient();
    void connect();
    void setCallback(IMuduoUser* pUser) { _pUser = pUser; }
    virtual void newConnection(int sockfd); //重写该方法，Connector类中调用
    void removeConnection(const TcpConnectionPtr& conn);      //客户端断开连接时的服务端处理方法
    TcpConnectionPtr connection() const { return _connection; }

    //MsgRouter相关
    int addMsgRouter(int msgid, msgCB userMsgCB) {return _msgRouter.registerMsgRouter(msgid, userMsgCB); }
    int delMsgRouter(int msgid, msgCB userMsgCB) {return _msgRouter.unRegisterMsgRouter(msgid, userMsgCB); }
    void callMsgRouter(MsgHead head, string &msgdata, TcpConnectionPtr conn) {_msgRouter.callMsgRouter(head, msgdata, conn);}

private:
    EventLoop* _pLoop;
    NetAddress _serverAddr;
    IMuduoUser* _pUser;
    shared_ptr<Connector> _connector;
    TcpConnectionPtr _connection;
    MsgRouter _msgRouter;   //存储消息id和对应的callback
};

#endif