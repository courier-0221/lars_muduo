#include "RouterEchoServerSingleCalcThread.h"

RouterEchoServerSingleCalcThread::RouterEchoServerSingleCalcThread(EventLoop* pLoop, NetAddress addr)
    :_pLoop(pLoop)
    ,_pServer(pLoop, addr)
{
    _pServer.setCallback(this);
}

RouterEchoServerSingleCalcThread::~RouterEchoServerSingleCalcThread()
{}

void RouterEchoServerSingleCalcThread::start()
{
    _pServer.addMsgRouter(111, std::bind(&RouterEchoServerSingleCalcThread::msg01callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _pServer.addMsgRouter(222, std::bind(&RouterEchoServerSingleCalcThread::msg02callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _pServer.addMsgRouter(333, std::bind(&RouterEchoServerSingleCalcThread::msg03callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    _pServer.start();
}

void RouterEchoServerSingleCalcThread::onConnection(TcpConnectionPtr conn)
{
    cout << "RouterEchoServerSingleCalcThread::onConnection" << endl;
}

void RouterEchoServerSingleCalcThread::onMessage(TcpConnectionPtr conn, Buffer* pBuf)
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
            cout << "RouterEchoServerSingleCalcThread::onMessage Invalid length:  " << head.msgLen << endl;
            break;
        }
        else if (pBuf->readableBytes() >= static_cast<size_t>(head.msgLen) + MESSAGE_HEAD_LEN)	//达到一条完整的消息
        {
            pBuf->retrieve(MESSAGE_HEAD_LEN);   //清除msghead
            string msg(pBuf->peek(), head.msgLen);

            _pServer.callMsgRouter(head, msg, conn);
            pBuf->retrieve(head.msgLen);    //清除msgdata
        }
        else		//未达到一条完整的消息
        {
            break;
        }      
    }
}

void RouterEchoServerSingleCalcThread::onWriteComplate(TcpConnectionPtr conn)
{
    cout << "RouterEchoServerSingleCalcThread::onWriteComplate" << endl;
}

void RouterEchoServerSingleCalcThread::onClose(TcpConnectionPtr conn)
{
    cout << "RouterEchoServerSingleCalcThread::onClose" << endl;
}

void RouterEchoServerSingleCalcThread::msg01callback(MsgHead head, string &msgdata, TcpConnectionPtr conn)
{
    cout << "RouterEchoServerSingleCalcThread::msg01callback" << " recv " << msgdata.size() << " bytes data [" << msgdata.c_str()
        << "] SEND tid = " << CurrentThread::tid() << endl;
    conn->send(msgdata + "\n");
}
void RouterEchoServerSingleCalcThread::msg02callback(MsgHead head, string &msgdata, TcpConnectionPtr conn)
{
    cout << "RouterEchoServerSingleCalcThread::msg02callback" << " recv " << msgdata.size() << " bytes data [" << msgdata.c_str()
        << "] SEND tid = " << CurrentThread::tid() << endl;
    conn->send(msgdata + "\n");
}
void RouterEchoServerSingleCalcThread::msg03callback(MsgHead head, string &msgdata, TcpConnectionPtr conn)
{
    cout << "RouterEchoServerSingleCalcThread::msg03callback" << " recv " << msgdata.size() << " bytes data [" << msgdata.c_str()
        << "] SEND tid = " << CurrentThread::tid() << endl;
    conn->send(msgdata + "\n");
}