#include "TcpClient.h"
#include "CurrentThread.h"
#include "Router.h"
#include "EndianCode.h"
#include "lars.pb.h"

//消息头的长度，固定值  id+len
#define MESSAGE_HEAD_LEN (8)

class RepoterClient : public IMuduoUser
{
 public:
	RepoterClient(EventLoop* loop, const NetAddress& serverAddr)
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
		cout << "RepoterClient::onConnection" << endl;
	}

    virtual void onMessage(TcpConnectionPtr conn, Buffer* pBuf)
	{
		string msg(pBuf->retrieveAllAsString());
		
		cout << "RepoterClient::onMessage recv tid = " << CurrentThread::tid() << " msgsize = " << msg.size() << endl;
	}

    virtual void onWriteComplate(TcpConnectionPtr conn)
	{
		//cout << "RepoterClient::onWriteComplate" << endl;
	}

    virtual void onClose(TcpConnectionPtr conn) {cout << "RepoterClient::onClose" << endl;}

	void sendReportStatusRequestMsg(int msgId)
	{
		Buffer buf = {0};
		MsgHead msghead = {0};
		string msgdata = {0};

		//msgdata
		lars::ReportStatusRequest req;
		uint32_t modid = rand() % 3 + 1;
		req.set_modid(modid);
		req.set_cmdid(1);
		req.set_caller(123);
		req.set_ts(time(NULL));
		for (int i = 0; i < 3; i ++)
		{
			lars::HostCallResult result;
			result.set_ip(i + 1);
			result.set_port((i + 1) * (i + 1));
			result.set_succ(100);
			result.set_err(3);
			if (i == 2)
			{
				result.set_overload(true);
			}
			else
			{
				result.set_overload(false);
			}
			req.add_results()->CopyFrom(result);
		}
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

		cout << "sendRouteRequestMsg msgId " << msgId << " msgLen " << msgdata.size() 
			<< " modid " << modid << " cmdid " << 1 << endl;
	}

 private:
	EventLoop* _loop;
	TcpClient _client;
};

int main(int argc, char **argv)
{
    cout << "main pid = " << ::getpid() << " tid = " << CurrentThread::tid() << endl;
	
	EventLoop loop;
	NetAddress serverAddr("127.0.0.1", 11110);
	RepoterClient client(&loop, serverAddr);
	client.connect();
	loop.runEvery(5, std::bind(&RepoterClient::sendReportStatusRequestMsg, &client, lars::ID_ReportStatusRequest));
	loop.loop();

    return 0;
}
