#include "DnsService.h"

DnsService::DnsService(EventLoop* pLoop, NetAddress addr)
    : _pLoop(pLoop)
    , _pServer(pLoop, addr)
{
    _pServer.setCallback(this);
}

DnsService::~DnsService()
{}

void DnsService::start()
{
    _pServer.addMsgRouter(lars::ID_GetRouteRequest, std::bind(&DnsService::msgGetRouteCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _pServer.start();
    _threadpool.start(3);
}

void DnsService::onConnection(TcpConnectionPtr conn)
{
    cout << "DnsService::onConnection connfd " << conn->getFd() << endl;
    modSetPtr module(new modSet());
    _requesModLog[conn->getFd()] = module; //记录client(connfd)请求过的mod(modid+cmdid)
    _onClient.insert(conn->getFd());//记录client(connfd)
    _clientMap[conn->getFd()] = conn;//记录client(connfd)对应的TcpConnection
}

void DnsService::onClose(TcpConnectionPtr conn)
{
    cout << "DnsService::onClose erase connfd " << conn->getFd() << endl;
    _requesModLog.erase(conn->getFd());
    _onClient.erase(conn->getFd());
    _clientMap.erase(conn->getFd());
}

void DnsService::onMessage(TcpConnectionPtr conn, Buffer* pBuf)
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
            cout << "DnsService::onMessage Invalid length:  " << head.msgLen << endl;
            break;
        }
        else if (pBuf->readableBytes() >= static_cast<size_t>(head.msgLen) + MESSAGE_HEAD_LEN)	//达到一条完整的消息
        {
            pBuf->retrieve(MESSAGE_HEAD_LEN);   //清除msghead
            string msg(pBuf->peek(), head.msgLen);
            _pServer.callMsgRouter(head, msg, conn);    //转到对应路由执行
            pBuf->retrieve(head.msgLen);    //清除msgdata
        }
        else		//未达到一条完整的消息
        {
            break;
        }      
    }
}

void DnsService::onWriteComplate(TcpConnectionPtr conn)
{
    //cout << "DnsService::onWriteComplate" << endl;
}

void DnsService::msgGetRouteCB(MsgHead head, string &msgdata, TcpConnectionPtr conn)
{
    int modid, cmdid;
    lars::GetRouteRequest req;
    req.ParseFromArray(msgdata.data(), head.msgLen);
    modid = req.modid();
    cmdid = req.cmdid();
    
    //订阅
    uint64_t mod = (((uint64_t)modid) << 32) + cmdid;
    RequestModMap::iterator reques = _requesModLog.find(conn->getFd());
    if (reques != _requesModLog.end()) 
    {
        //没找到则订阅
        if (reques->second->find(mod) == reques->second->end())
        {
            reques->second->insert(mod);
            _dnsSubcribe.subscribe(mod, conn->getFd());
            cout << "DnsService::msgGetRouteCB connfd " << conn->getFd() << " subscribe modid = " << modid << " cmdid " << cmdid << endl;
        }
    }
    else
    {
        cout << "DnsService::msgGetRouteCB connfd " << conn->getFd() << " is not in RequestMap !!!" << endl;
    }
    
    //加入线程池
    _threadpool.addTask(std::bind(&DnsService::threadPoolWorkTask, this, req, conn));
}

void DnsService::threadPoolWorkTask(lars::GetRouteRequest req, TcpConnectionPtr conn)
{
    //解析proto内容 得到 modid 和 cmdIid
    int modid, cmdid;
    modid = req.modid();
    cmdid = req.cmdid();

    cout << "DnsService::threadPoolWorkTask tid = " << CurrentThread::tid() << " modid = " << modid << " cmdid = " << cmdid << endl;
    
    //返回数据给客户端 
    Buffer buf = {0};
    string sendData = {0};
    MsgHead msghead = {0};
    //msgdata
    lars::GetRouteResponse rsp;
    rsp.set_modid(modid);
    rsp.set_cmdid(cmdid);
    //通过modid/cmdid 获取hostSet信息
    hostSet hosts = _dnsRoute.getHosts(modid, cmdid);
    for (hostSet::iterator it = hosts.begin(); it != hosts.end(); it++) 
    {
        uint64_t ip_port = *it;
        lars::HostInfo host;
        host.set_ip((uint32_t)(ip_port>>32));
        host.set_port((uint32_t)(ip_port));
        rsp.add_host()->CopyFrom(host);
    }
    rsp.SerializeToString(&sendData);
    buf.append(sendData.data(), sendData.size());
    //msghead
    msghead.msgId = hostToNetwork32(lars::ID_GetRouteResponse);
    msghead.msgLen = hostToNetwork32(static_cast<int32_t>(sendData.size()));
    buf.prepend(&msghead, sizeof msghead);
    //msghead + msgdata ==> send
	string response(buf.peek(), buf.readableBytes());
    conn->send(response);
}