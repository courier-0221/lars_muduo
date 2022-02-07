#include "TcpClient.h"
#include "CurrentThread.h"
#include "Router.h"
#include "EndianCode.h"
#include "lars.pb.h"

//消息头的长度，固定值  id+len
#define MESSAGE_HEAD_LEN (8)

class DnsClient : public IMuduoUser
{
 public:
	DnsClient(EventLoop* loop, const NetAddress& serverAddr)
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
		cout << "DnsClient::onConnection" << endl;
	}

    virtual void onMessage(TcpConnectionPtr conn, Buffer* pBuf)
	{
		string msg(pBuf->retrieveAllAsString());
		
		//解包得到数据
		lars::GetRouteResponse rsp;
		rsp.ParseFromArray(msg.data(), msg.size());
		
		cout << "DnsClient::onMessage recv tid = " << CurrentThread::tid() << " modid = " << rsp.modid() << " cmdid = " << rsp.cmdid() << " host_size = " << rsp.host_size() << endl;

		for (int i = 0; i < rsp.host_size(); i++) 
		{
			cout << "DnsClient::onMessage recv -->ip = " << rsp.host(i).ip() << " -->port = " << rsp.host(i).port() << endl;
		}
	}

    virtual void onWriteComplate(TcpConnectionPtr conn)
	{
		//cout << "DnsClient::onWriteComplate" << endl;
	}

    virtual void onClose(TcpConnectionPtr conn) {cout << "TestClient::onClose" << endl;}

	void sendRouteRequestMsg(int msgId)
	{
		Buffer buf = {0};
		MsgHead msghead = {0};
		string msgdata = {0};

		//msgdata
		lars::GetRouteRequest req;
		req.set_modid(1);
		req.set_cmdid(1);
		req.SerializeToString(&msgdata);
		buf.append(msgdata.data(), msgdata.size());
		//msghead
		msghead.msgId = hostToNetwork32(msgId);
		msghead.msgLen = hostToNetwork32(static_cast<int32_t>(msgdata.size()));
		buf.prepend(&msghead, sizeof msghead);
		//msghead + msgdata ==> send
		string request(buf.peek(), buf.readableBytes());
		TcpConnectionPtr conn = _client.connection();
		conn->send(request);

		cout << "sendRouteRequestMsg msgId " << msgId << " msgLen " << msgdata.size() << endl;
	}


 private:
	EventLoop* _loop;
	TcpClient _client;
};

int main(int argc, char **argv)
{
    cout << "main pid = " << ::getpid() << " tid = " << CurrentThread::tid() << endl;
	
	EventLoop loop;
	NetAddress serverAddr("127.0.0.1", 11111);
	DnsClient client(&loop, serverAddr);
	client.connect();
	loop.runEvery(2, std::bind(&DnsClient::sendRouteRequestMsg, &client, lars::ID_GetRouteRequest));
	loop.loop();

    return 0;
}
