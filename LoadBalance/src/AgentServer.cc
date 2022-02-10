#include "AgentServer.h"

AgentServer::AgentServer(EventLoop* pLoop, NetAddress addr)
    : _pLoop(pLoop)
    , _pServer(pLoop, addr)
    //, _threadpool()
    , _dnsClient(NetAddress("127.0.0.1", 11111))
    , _repoterClient(NetAddress("127.0.0.1", 11110))
{
    _pServer.setCallback(this);
}

AgentServer::~AgentServer()
{}

void AgentServer::start()
{
    //注册消息转发
    _pServer.addMsgRouter(lars::ID_GetHostRequest, std::bind(&AgentServer::msgGetTheBestHostInfoCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _pServer.addMsgRouter(lars::ID_ReportRequest, std::bind(&AgentServer::msgReportStatusCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _pServer.addMsgRouter(lars::ID_API_GetRouteRequest, std::bind(&AgentServer::msgGetAllHostsCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    
    _pServer.start();
    //_threadpool.start(5);
    _dnsClient.start();
    _repoterClient.start();
}

void AgentServer::onConnection(TcpConnectionPtr conn)
{
    //cout << "AgentServer::onConnection connfd " << conn->getFd() << endl;
}

void AgentServer::onClose(TcpConnectionPtr conn)
{
    cout << "AgentServer::onClose connfd " << conn->getFd() << endl;
}

void AgentServer::onMessage(TcpConnectionPtr conn, Buffer* pBuf)
{
    while (pBuf->readableBytes() >= MESSAGE_HEAD_LEN)  
    {
        MsgHead head;
        const void* data = pBuf->peek();
        memcpy(&head, data, sizeof head);

        head.msgId = networkToHost32(head.msgId);
        head.msgLen = networkToHost32(head.msgLen);

        if (head.msgLen > 65536 || head.msgLen < 0)
        {
            cout << "AgentServer::onMessage Invalid length:  " << head.msgLen << endl;
            break;
        }
        else if (pBuf->readableBytes() >= static_cast<size_t>(head.msgLen) + MESSAGE_HEAD_LEN)	//达到一条完整的消息
        {
            pBuf->retrieve(MESSAGE_HEAD_LEN);   //清除msghead
            string msg(pBuf->peek(), head.msgLen);
            _pServer.callMsgRouter(head, msg, conn);    //转到对应路由执行
            pBuf->retrieve(head.msgLen);    //清除msgdata
        }
        else	//未达到一条完整的消息
        {
            break;
        }      
    }
}

void AgentServer::onWriteComplate(TcpConnectionPtr conn)
{
    //cout << "AgentServer::onWriteComplate" << endl;
}

void AgentServer::msgGetTheBestHostInfoCB(MsgHead head, string &msgdata, TcpConnectionPtr conn)
{
    Buffer buf = {0};
	MsgHead msghead = {0};
	string sendData = {0};

    lars::GetHostRequest req;         
    req.ParseFromArray(msgdata.data(), head.msgLen);

    uint32_t modid = req.modid();
    uint32_t cmdid = req.cmdid();

    //msgdata
    lars::GetHostResponse rsp;
    rsp.set_seq(req.seq());
    rsp.set_modid(modid);
    rsp.set_cmdid(cmdid);

    //根据modid+cmdid获取一个最佳host
    _manage.getTheBestHost(modid, cmdid, rsp);

    rsp.SerializeToString(&sendData);
    buf.append(sendData.data(), sendData.size());
    //msghead
    msghead.msgId = hostToNetwork32(lars::ID_GetHostResponse);
    msghead.msgLen = hostToNetwork32(static_cast<int32_t>(sendData.size()));
    buf.prepend(&msghead, sizeof msghead);
    //msghead + msgdata ==> send
    string response(buf.peek(), buf.readableBytes());
    conn->send(response);
}

void AgentServer::msgReportStatusCB(MsgHead head, string &msgdata, TcpConnectionPtr conn)
{
    //cout << "AgentServer::msgReportStatusCB msgLen " << head.msgLen << endl;
    lars::ReportRequest req;
    req.ParseFromArray(msgdata.data(), head.msgLen);
    _manage.reportHost(req);
}

void AgentServer::msgGetAllHostsCB(MsgHead head, string &msgdata, TcpConnectionPtr conn)
{
    Buffer buf = {0};
	MsgHead msghead = {0};
	string sendData = {0};

    lars::GetRouteRequest req;         
    req.ParseFromArray(msgdata.data(), head.msgLen);
    int modid = req.modid();
    int cmdid = req.cmdid();

    //msgdata
    lars::GetRouteResponse rsp;
    rsp.set_modid(modid);
    rsp.set_cmdid(cmdid);

    //根据modid+cmdid获取该节点下的所有hosts
    _manage.getRoute(modid, cmdid, rsp);
    
    rsp.SerializeToString(&sendData);
    buf.append(sendData.data(), sendData.size());
	//msghead
    msghead.msgId = hostToNetwork32(lars::ID_API_GetRouteResponse);
    msghead.msgLen = hostToNetwork32(static_cast<int32_t>(sendData.size()));
    buf.prepend(&msghead, sizeof msghead);
    //msghead + msgdata ==> send
    string response(buf.peek(), buf.readableBytes());
    conn->send(response);

    cout << "AgentServer::msgGetAllHostsCB send response modid " << modid << " cmdid " << cmdid << " msglen " << sendData.size() << endl;
}

