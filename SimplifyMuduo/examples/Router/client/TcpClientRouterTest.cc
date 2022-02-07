#include "TcpClient.h"
#include "CurrentThread.h"
#include "Router.h"
#include "EndianCode.h"

//消息头的长度，固定值  id+len
#define MESSAGE_HEAD_LEN (8)

class TestClient : public IMuduoUser
{
 public:
	TestClient(EventLoop* loop, const NetAddress& serverAddr)
		: _loop(loop)
		, _client(_loop, serverAddr)
	{
		_client.setCallback(this);
	}

	void connect()
	{
		_client.connect();
	}

	virtual void onConnection(TcpConnectionPtr conn)
	{
		cout << "TestClient::onConnection" << endl;
	}

    virtual void onMessage(TcpConnectionPtr conn, Buffer* pBuf)
	{
		string msg(pBuf->retrieveAllAsString());
		cout << "TestClient::onMessage" << "recv tid = " << CurrentThread::tid() 
				<< " recvSize: " << msg.size() << " bytes " << " recvData: " << msg.c_str() << endl;
	}

    virtual void onWriteComplate(TcpConnectionPtr conn)
	{
		cout << "TestClient::onWriteComplate" << endl;
	}

    virtual void onClose(TcpConnectionPtr conn) {cout << "TestClient::onClose" << endl;}

	void sendmsg(int msgId, string msg)
	{
		MsgHead head;
		Buffer buf;
		
		buf.append(msg.data(), msg.size());
		
		head.msgId = hostToNetwork32(msgId);
		head.msgLen = hostToNetwork32(static_cast<int32_t>(msg.size()));
		buf.prepend(&head, sizeof head);

		string message(buf.peek(), buf.readableBytes());
		TcpConnectionPtr conn = _client.connection();
		if (conn.get()) conn->send(message);

		cout << "sendmsg msgId " << head.msgId << " msgLen " << head.msgLen << endl;
	}


 private:
	EventLoop* _loop;
	TcpClient _client;
};

int main(int argc, char* argv[])
{
	cout << "main pid = " << ::getpid() << " tid = " << CurrentThread::tid() << endl;
	
	EventLoop loop;
	NetAddress serverAddr("127.0.0.1", 11111);
	TestClient client(&loop, serverAddr);
	client.connect();
	loop.runEvery(2, std::bind(&TestClient::sendmsg, &client, 111, "msgId=111,msgData=11111"));
	loop.runEvery(2, std::bind(&TestClient::sendmsg, &client, 222, "msgId=222,msgData=22222"));
	loop.runEvery(2, std::bind(&TestClient::sendmsg, &client, 333, "msgId=333,msgData=33333"));
	loop.loop();

	return 0;
}