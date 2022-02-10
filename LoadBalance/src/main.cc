#include "AgentServer.h"

std::shared_ptr<AgentServer> gAgentServer;

int main(int args, char** argv)
{
    cout << "main" << " tid = " << CurrentThread::tid() << endl;

    NetAddress addr("127.0.0.1", 11109);
    EventLoop loop;
    gAgentServer.reset(new AgentServer(&loop, addr));
    gAgentServer->start();
    loop.loop();
    return 0;
}
