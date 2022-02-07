#ifndef DNSROUTE_H
#define DNSROUTE_H

#include "Common.h"
#include "Timestamp.h"
#include "RwLock.h"
#include "Thread.h"
#include <mysql/mysql.h>
#include "DnsCommon.h"
#include "lars.pb.h"

//表示 module(modid+cmdid) --> hosts(ip+port)的对应关系
//一个module下可以存在多个host，这个类用来读取数据库中的内容并维护到类的表结构中
class DnsRoute : noncopyable
{
public:
    DnsRoute();
    ~DnsRoute() { cout << "DnsRoute::~DnsRoute()" << endl; }
    hostSet getHosts(uint32_t modid, uint32_t cmdid);   //通过modid/cmdid获取全部的当前模块所挂载的host集合
private:
    int connectMysql();    //连接mysql
    void buildRouteMapA();   //将RouteData表中数据加载到mapA中
    void loadRouteMapB();   //将RouteData表中数据加载到mapB中
    int loadVersion();  //加载当前版本 return:0-成功-version没有改变; 1-成功-version有改变; -1-失败
    void loadChanges(std::vector<uint64_t>& changeList);  //加载RouteChange 得到修改的modid/cmdid
    void swap();    //将 _routeDataMapB 的数据更新到 _routeDataMapA 中
    void checkRouteChangesFunc(void);   //周期性后端检查db的route信息的更改业务
    //操作RouteChange表，默认删除当前版本之前的全部修改，removeAll=true删除RouteChange表中所有更改信息
    void removeChanges(bool removeAll = false);
private:
    MYSQL _dbConn;
    RwLock _mapLock; //map读写锁
    long _version;  //当前的版本号
    routeMap _routeDataMapA; //客户端查询走这张表
    routeMap _routeDataMapB; //数据库中有更新先更新到这张表，然后在给mapA
    Timestamp _lastLoadTime;  //上一次从数据库加载内容时间戳
};

#endif