#ifndef LOADBALANCE_H
#define LOADBALANCE_H

#include "HostInfo.h"
#include "lars.pb.h"
#include "Common.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include <string>
#include <iostream>
#include <map>
#include <list>
#include <vector>
using namespace std;

//记录一个module下所有的HostInfo

// key:uint64(ip+port), value:HostInfo(对应ip+port信息)
typedef std::map<uint64_t, HostInfo*> HostMap;

//HostInfo list集合
typedef std::list<HostInfo*> HostList; 

class LoadBalance : noncopyable
{
public:
    LoadBalance(uint32_t modid, uint32_t cmdid);
    ~LoadBalance();

    //判断是否已经没有host在当前LB节点中
    bool empty() const;


    //Host相关
    //如果list中没有host信息，需要从远程的DNS Service发送GetRouteHost请求申请
    void pullHostFromDns();
    //根据dnsservice远程返回的结果，更新_hostMap
    void updateLocalHostsFromDns(lars::GetRouteResponse &rsp);
    //从一个hostList中得到一个节点放到 GetHostResponse 的HostInfo中
    void getHostFromList(lars::GetHostResponse &rsp, HostList &l);
    //从当前的双队列中获取host信息
    int choiceOneHost(lars::GetHostResponse &rsp);
    //获取当前挂载下的全部host信息 添加到vec中
    void getAllHosts(std::vector<HostInfo*> &vec);


    //report相关
    //根据状态调整主机所处链表，空闲链表还是过载链表
    void adjustHostWhereTo(uint32_t ip, uint32_t port, int retcode);
    //提交host的调用结果给远程reporter service上报结果
    void pushCallResultToReport();
    
    //当前load_balance模块的状态
    enum STATUS
    {
        PULLING, //正在从远程dns service通过网络拉取
        NEW      //正在创建新的LoadBalance模块
    };
    STATUS _status;  //当前的状态
    
    Timestamp _lastUpdateTime; //最后更新hostMap时间戳
    
private:
    uint32_t _modid;
    uint32_t _cmdid;
    uint32_t _access_cnt;       //请求次数，每次请求+1,判断是否超过probe_num阈值
    HostMap _hostMap;           //当前load_balance模块所管理的全部ip + port为主键的 host信息集合
    HostList _idleList;        //空闲队列
    HostList _overloadList;    //过载队列
};

#endif