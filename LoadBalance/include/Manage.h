#ifndef MANAGE_H
#define MANAGE_H

#include "LoadBalance.h"
#include "lars.pb.h"
#include "Common.h"
#include "Mutex.h"
#include "TimeStamp.h"
#include "CurrentThread.h"
#include <string>
#include <iostream>
#include <map>
using namespace std;

//每个module对应一个LoadBalance，Manage的作用就是管理所有的module及其对应的LoadBalance

//key: modid+cmdid    value: LoadBalance
typedef std::map<uint64_t, LoadBalance*> ManageMap;

class Manage 
{
public:
    Manage();
    ~Manage() { cout << "Manage::~Manage()" << endl; }

    // agentclient <--> agent --> DnsService 获取某个modid/cmdid的全部主机，将返回的主机结果存放在rsp中
    int getRoute(uint32_t modid, uint32_t cmdid, lars::GetRouteResponse &rsp);
    // agentclient --> agent --> repoterServices 上报某主机的获取结果
    void reportHost(lars::ReportRequest req);
    // agentclient --> agent 获取一个最佳的host主机，将返回的主机结果存放在rsp中
    int getTheBestHost(uint32_t modid, uint32_t cmdid, lars::GetHostResponse &rsp);
    // DnsService --> agent 根据DnsService返回的结果做更新
    int updateLocalHostsMap(uint32_t modid, uint32_t cmdid, lars::GetRouteResponse &rsp);

private:
    //将全部的LoadBalance都重置为NEW状态
    void resetLBStatus();

private:
    ManageMap _loadbalances;
    MutexLock _mutex; 
};


#endif