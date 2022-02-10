#include "AgentClient.h"

int main(int argc, char **argv)
{
    cout << "main pid = " << ::getpid() << " tid = " << CurrentThread::tid() << endl;
	
	EventLoop loop;
	NetAddress serverAddr("127.0.0.1", 11109);
	AgentClient client(&loop, serverAddr);
	client.connect();
	loop.runAfter(3, std::bind(&AgentClient::sendGetAllHostsRequestMsg, &client, lars::ID_API_GetRouteRequest));
	loop.runAfter(4, std::bind(&AgentClient::sendGetBestHostRequestMsg, &client, lars::ID_GetHostRequest));
	loop.runEvery(5, std::bind(&AgentClient::sendReportRequestMsg, &client, lars::ID_ReportRequest));
	loop.loop();

    return 0;
}