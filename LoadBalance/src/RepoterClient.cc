#include "RepoterClient.h"

RepoterClient::RepoterClient(NetAddress addr)
    : _loop()
    , _client(&_loop, addr)
    , _thread(std::bind(&RepoterClient::threadWorkTask, this), "RepoterClient")
{
    _client.setCallback(this);
}

RepoterClient::~RepoterClient()
{}

void RepoterClient::start()
{
    _thread.start();
}

void RepoterClient::onConnection(TcpConnectionPtr conn)
{
    cout << "RepoterClient::onConnection connfd " << conn->getFd() << endl;
}

void RepoterClient::onClose(TcpConnectionPtr conn)
{
    cout << "RepoterClient::onClose connfd " << conn->getFd() << endl;
}

void RepoterClient::onMessage(TcpConnectionPtr conn, Buffer* pBuf)
{
    cout << "RepoterClient::onMessage connfd " << conn->getFd() << endl;
}

void RepoterClient::onWriteComplate(TcpConnectionPtr conn)
{
    cout << "RepoterClient::onWriteComplate" << endl;
}

void RepoterClient::threadWorkTask(void)
{
    cout << "RepoterClient::threadWorkTask tid = " << CurrentThread::tid() << endl;
    _client.connect();
	_loop.loop();
}



