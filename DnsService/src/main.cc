#include "DnsCommon.h"
#include "DnsService.h"

std::shared_ptr<DnsService> gDnsService;
EventLoop gLoop;

int main(int args, char** argv)
{
    cout << "main" << " tid = " << CurrentThread::tid() << endl;

    NetAddress addr("127.0.0.1", 11111);
    gDnsService.reset(new DnsService(&gLoop, addr));
    gDnsService->start();
    gLoop.loop();
    return 0;
}
