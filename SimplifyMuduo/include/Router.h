#ifndef ROUTER_H
#define ROUTER_H

#include "TcpConnection.h"

#include <functional>
#include <iostream>
#include <map>
using namespace std;

//解决tcp粘包问题的消息头
struct MsgHead
{
    int32_t msgId;
    int32_t msgLen;
};

//定义一个路由回调函数的数据类型
//typedef void msgCB(uint32_t msgid, uint32_t msglen, const char *msgdata, TcpConnectionPtr conn);
//typedef std::function<void(MsgHead, const char*, TcpConnectionPtr)> msgCB;
typedef std::function<void(MsgHead, string&, TcpConnectionPtr)> msgCB;

//消息路由分发机制 msgid + cb
class MsgRouter
{
public:
    MsgRouter(): _router() { cout << "MsgRouter::msg_router init..." << endl; }

    //msgid+cb注册
    int registerMsgRouter(int32_t msgid, msgCB userMsgCB) 
    {
        if (_router.find(msgid) != _router.end())  
        {
            std::cout << "MsgRouter::msgID = " << msgid << " is already register..." << std::endl;
            return -1;
        }
        cout <<"MsgRouter::Add msg callback msgid = " << msgid << endl;
        _router[msgid] = userMsgCB;
        return 0;
    }
    //删除
    int unRegisterMsgRouter(int32_t msgid, msgCB userMsgCB) 
    {
        if (_router.find(msgid) != _router.end())  
        {
            std::cout << "MsgRouter::msgID = " << msgid << " is already register..." << std::endl;
            return -1;
        }
        cout <<"MsgRouter::Add msg callback msgid = " << msgid << endl;
        _router[msgid] = userMsgCB;
        return 0;
    }

    //调用注册的回调函数业务
    void callMsgRouter(MsgHead head, string &msgdata, TcpConnectionPtr conn) 
    {
        //std::cout << "call msgid = " << head.msgId << std::endl;

        if (_router.find(head.msgId) == _router.end()) 
        {
            cout << "MsgRouter::callMsgRouter msgid " << head.msgId << " is not register" << endl;
            return;
        }

        msgCB callback = _router[head.msgId];
        callback(head, msgdata, conn); //调用注册的回调业务
    }

private:
    map<int32_t, msgCB> _router;    //针对消息的路由分发， key是msgid， value是注册的回调业务处理函数
};

#endif
