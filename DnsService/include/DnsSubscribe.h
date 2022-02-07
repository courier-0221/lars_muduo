#ifndef DNSSUBSCRIBE_H
#define DNSSUBSCRIBE_H

#include "Common.h"
#include "Mutex.h"
#include "DnsCommon.h"
#include "lars.pb.h"

class DnsSubscribe : noncopyable
{
public:
    DnsSubscribe();
    ~DnsSubscribe() { cout << "DnsSubscribe::~DnsSubscribe()" << endl; }
    void subscribe(uint64_t mod, int fd);    //订阅功能
    void unsubscribe(uint64_t mod, int fd);  //取消订阅功能
    //发布功能 输入形参： 是那些mod被修改了，被修改的模块所对应的clientFd就应该被发布，收到新的modid/cmdid的结果
    void publish(std::vector<uint64_t> &changeMods);
private:
    //订阅模块感知不到客户端断开了链接，需要服务器给出在线的客户端来对pushlist中的数据做过滤
    void makePublishMap(OnlineClient &onlineClient, publishMap &needPublish);
    void pushChangeTask(void);  //主动推送任务
private:
    subcribeMap _bookList;  //订阅清单
    MutexLock _bookListLock;
    publishMap _pushList;   //发布清单
    MutexLock _pushListLock;
};

#endif