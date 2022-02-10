#include "DnsClient.h"
#include "AgentServer.h"

extern std::shared_ptr<AgentServer> gAgentServer;

DnsClient::DnsClient(NetAddress addr)
    : _loop()
    , _client(&_loop, addr)
    , _thread(std::bind(&DnsClient::threadWorkTask, this), "DnsClient")
{
    _client.setCallback(this);
}

DnsClient::~DnsClient()
{}

void DnsClient::start()
{
    _thread.start();
}

void DnsClient::onConnection(TcpConnectionPtr conn)
{
    cout << "DnsClient::onConnection connfd " << conn->getFd() << endl;
}

void DnsClient::onClose(TcpConnectionPtr conn)
{
    cout << "DnsClient::onClose connfd " << conn->getFd() << endl;
}

void DnsClient::onMessage(TcpConnectionPtr conn, Buffer* pBuf)
{
    //cout << "DnsClient::onMessage call updateLocalHostsFromDns " << endl;

    while (pBuf->readableBytes() >= MESSAGE_HEAD_LEN)  
    {
        MsgHead head;
        const void* data = pBuf->peek();
        memcpy(&head, data, sizeof head);

        head.msgId = networkToHost32(head.msgId);
        head.msgLen = networkToHost32(head.msgLen);

        if (head.msgLen > 65536 || head.msgLen < 0)
        {
            cout << "DnsClient::onMessage Invalid length:  " << head.msgLen << endl;
            break;
        }
        else if (pBuf->readableBytes() >= static_cast<size_t>(head.msgLen) + MESSAGE_HEAD_LEN)	//达到一条完整的消息
        {
            pBuf->retrieve(MESSAGE_HEAD_LEN);   //清除msghead
            string msg(pBuf->peek(), head.msgLen);
            
            //调用manage模块中的更新模块方法
            lars::GetRouteResponse rsp;
            rsp.ParseFromArray(msg.data(), msg.size());
            uint32_t modid = rsp.modid();
            uint32_t cmdid = rsp.cmdid();
            gAgentServer->_manage.updateLocalHostsMap(modid, cmdid, rsp);

            for (int i = 0; i < rsp.host_size(); i++) 
            {
                cout << "DnsClient::onMessage modid = " << rsp.modid() << " cmdid = " << rsp.cmdid() 
                        << " recv -->ip = " << rsp.host(i).ip() << " -->port = " << rsp.host(i).port() << endl;
            }
            
            pBuf->retrieve(head.msgLen);    //清除msgdata
        }
        else		//未达到一条完整的消息
        {
            break;
        }      
    }
}

void DnsClient::onWriteComplate(TcpConnectionPtr conn)
{
    //cout << "DnsClient::onWriteComplate" << endl;
}

void DnsClient::threadWorkTask(void)
{
    cout << "DnsClient::threadWorkTask tid = " << CurrentThread::tid() << endl;
    _client.connect();
	_loop.loop();
}



