#include "ReporterService.h"

ReporterService::ReporterService(EventLoop* pLoop, NetAddress addr)
    : _pLoop(pLoop)
    , _pServer(pLoop, addr)
    , _storeReport()
    , _threadpool()
{
    _pServer.setCallback(this);
}

ReporterService::~ReporterService()
{}

void ReporterService::start()
{
    _pServer.addMsgRouter(lars::ID_ReportStatusRequest, std::bind(&ReporterService::msgGetReportStatusCB, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _pServer.start();
    _threadpool.start(5);
}

void ReporterService::onConnection(TcpConnectionPtr conn)
{
    cout << "ReporterService::onConnection connfd " << conn->getFd() << endl;
}

void ReporterService::onClose(TcpConnectionPtr conn)
{
    cout << "ReporterService::onClose connfd " << conn->getFd() << endl;
}

void ReporterService::onMessage(TcpConnectionPtr conn, Buffer* pBuf)
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
            cout << "ReporterService::onMessage Invalid length:  " << head.msgLen << endl;
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

void ReporterService::onWriteComplate(TcpConnectionPtr conn)
{
    //cout << "ReporterService::onWriteComplate" << endl;
}

void ReporterService::msgGetReportStatusCB(MsgHead head, string &msgdata, TcpConnectionPtr conn)
{
    lars::ReportStatusRequest req;
    req.ParseFromArray(msgdata.data(), head.msgLen);
    _threadpool.addTask(std::bind(&ReporterService::threadPoolWorkTask, this, req));
}

void ReporterService::threadPoolWorkTask(lars::ReportStatusRequest req)
{
    cout << "ReporterService::threadPoolWorkTask tid = " << CurrentThread::tid() << endl;
    _storeReport.store(req);
}



