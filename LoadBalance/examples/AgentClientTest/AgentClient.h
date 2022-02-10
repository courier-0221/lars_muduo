#ifndef _AGENTCLIENT_H_
#define _AGENTCLIENT_H_

#include "TcpClient.h"
#include "CurrentThread.h"
#include "Router.h"
#include "EndianCode.h"
#include "lars.pb.h"

//消息头的长度，固定值  id+len
#define MESSAGE_HEAD_LEN (8)

class AgentClient : public IMuduoUser
{
 public:
	AgentClient(EventLoop* loop, const NetAddress& serverAddr);
    ~AgentClient(){ cout << "~AgentClient" << endl; }

	void connect();
	virtual void onConnection(TcpConnectionPtr conn) {cout << "AgentClient::onConnection" << endl;}
    virtual void onMessage(TcpConnectionPtr conn, Buffer* pBuf);
    virtual void onWriteComplate(TcpConnectionPtr conn) {}
    virtual void onClose(TcpConnectionPtr conn) {cout << "AgentClient::onClose" << endl;}

	void sendReportRequestMsg(int msgId);   //agentserver不会回响应
    void sendGetAllHostsRequestMsg(int msgId);
    void sendGetBestHostRequestMsg(int msgId);

    void msgRecvTheBestHostInfoCB(MsgHead head, string &msgdata, TcpConnectionPtr conn);
    void msgRecvAllHostsCB(MsgHead head, string &msgdata, TcpConnectionPtr conn);

 private:
	EventLoop* _loop;
	TcpClient _client;
};



#endif
