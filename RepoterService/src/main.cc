#include "ReporterService.h"

int main(int args, char** argv)
{
    cout << "main" << " tid = " << CurrentThread::tid() << endl;

    NetAddress addr("127.0.0.1", 11110);
    EventLoop loop;
    ReporterService reportService(&loop, addr);
    reportService.start();
    loop.loop();
    return 0;
}
