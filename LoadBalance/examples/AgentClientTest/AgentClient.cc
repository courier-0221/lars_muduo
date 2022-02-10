#include "AgentClient.h"


AgentClient::AgentClient(EventLoop* loop, const NetAddress& serverAddr)
		: _loop(loop)
		, _client(_loop, serverAddr)
{
    _client.setCallback(this);
}

void AgentClient::connect()
{
    _client.connect();
    //注册消息转发
    _client.addMsgRouter(lars::ID_GetHostResponse, std::bind(&AgentClient::msgRecvTheBestHostInfoCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _client.addMsgRouter(lars::ID_API_GetRouteResponse, std::bind(&AgentClient::msgRecvAllHostsCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void AgentClient::onMessage(TcpConnectionPtr conn, Buffer* pBuf)
{
    while (pBuf->readableBytes() >= MESSAGE_HEAD_LEN)  
    {
        MsgHead head;
        const void* data = pBuf->peek();
        memcpy(&head, data, sizeof head);

        head.msgId = networkToHost32(head.msgId);
        head.msgLen = networkToHost32(head.msgLen);

        //cout << "AgentClient::onMessage head.msgid " << head.msgId << " head.msgLen " << head.msgLen << endl;

        if (head.msgLen > 65536 || head.msgLen < 0)
        {
            cout << "DnsClient::onMessage Invalid length:  " << head.msgLen << endl;
            break;
        }
        else if (pBuf->readableBytes() >= static_cast<size_t>(head.msgLen) + MESSAGE_HEAD_LEN)	//达到一条完整的消息
        {
            pBuf->retrieve(MESSAGE_HEAD_LEN);   //清除msghead
            string msg(pBuf->peek(), head.msgLen);
            _client.callMsgRouter(head, msg, conn);    //转到对应路由执行
            pBuf->retrieve(head.msgLen);    //清除msgdata
        }
        else		//未达到一条完整的消息
        {
            break;
        }      
    }
}

void AgentClient::sendReportRequestMsg(int msgId)
{
    string ip("192.168.1.100");
    uint32_t port = 8082;
    //uint32_t modid = rand() % 3 + 1;
    uint32_t modid = 1;
    uint32_t cmdid = 2;
    
    Buffer buf = {0};
    MsgHead msghead = {0};
    string msgdata = {0};

    //msgdata
    lars::ReportRequest req;
    req.set_modid(modid);
    req.set_cmdid(cmdid);
    req.set_retcode(0);
    lars::HostInfo *hp = req.mutable_host();
    //ip
    struct in_addr inaddr;
    inet_aton(ip.c_str(), &inaddr);
    int ip_num = htonl(inaddr.s_addr);
    hp->set_ip(ip_num);
    //port
    hp->set_port(port);
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
        << " modid " << modid << " cmdid " << cmdid << endl;
}

void AgentClient::sendGetAllHostsRequestMsg(int msgId)
{
    uint32_t modid = 1;
    //uint32_t cmdid = rand() % 2 + 1;
    uint32_t cmdid = 2;

    Buffer buf = {0};
	MsgHead msghead = {0};
	string msgdata = {0};

    //msgdata
    lars::GetRouteRequest req;
    req.set_modid(modid);
    req.set_cmdid(cmdid);
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

    cout << "AgentClient::sendGetAllHostsRequestMsg modid " << modid << " cmdid " << cmdid << " msglen " << msgdata.size() << endl; 
}

void AgentClient::sendGetBestHostRequestMsg(int msgId)
{
    uint32_t modid = 1;
    //uint32_t cmdid = rand() % 3 + 1;
    uint32_t cmdid = 2;
    uint32_t seq = 119;

    Buffer buf = {0};
	MsgHead msghead = {0};
	string msgdata = {0};

    //msgdata
    lars::GetHostRequest req;
    req.set_seq(seq);
    req.set_modid(modid);
    req.set_cmdid(cmdid);
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

    cout << "AgentClient::sendGetBestHostRequestMsg msgId " << msgId << " modid " << modid << " cmdid " << cmdid << " msglen " << msgdata.size() << endl; 
}

void AgentClient::msgRecvAllHostsCB(MsgHead head, string &msgdata, TcpConnectionPtr conn)
{
    //解包得到数据
    lars::GetRouteResponse rsp;
    rsp.ParseFromArray(msgdata.data(), head.msgLen);

    for (int i = 0; i < rsp.host_size(); i++) 
    {
        cout << "AgentClient::msgRecvAllHostsCB modid = " << rsp.modid() << " cmdid = " << rsp.cmdid() 
                << " recv -->ip = " << rsp.host(i).ip() << " -->port = " << rsp.host(i).port() << endl;
    }
}

void AgentClient::msgRecvTheBestHostInfoCB(MsgHead head, string &msgdata, TcpConnectionPtr conn)
{
    string ip = {0};
    uint32_t port   = 0;
    uint32_t modid  = 0;
    uint32_t cmdid  = 0;

    //解包得到数据
    lars::GetHostResponse rsp;
    rsp.ParseFromArray(msgdata.data(), head.msgLen);

    modid = rsp.modid();
    cmdid = rsp.cmdid();

    if (rsp.retcode() == 0)
    {
        lars::HostInfo host = rsp.host();

        struct in_addr inaddr;
        uint32_t host_ip = ntohl(host.ip());
        inaddr.s_addr = host_ip;

        ip = inet_ntoa(inaddr);
        port = host.port();
    }
    
    cout << "AgentClient::msgRecvTheBestHostInfoCB modid = " << rsp.modid() << " cmdid = " << rsp.cmdid() 
            << " uesable -->ip = " << ip << " -->port = " << port << endl;
}
