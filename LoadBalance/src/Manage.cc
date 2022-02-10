#include "Manage.h"

Manage::Manage() : _mutex()
{
    resetLBStatus();
}

void Manage::resetLBStatus()
{
    MutexLockGuard lock(_mutex);
    for (ManageMap::iterator it = _loadbalances.begin(); it != _loadbalances.end(); it++)
    {
        LoadBalance *lb = it->second;
        if (lb->_status == LoadBalance::PULLING)
        {
            lb->_status = LoadBalance::NEW;
        }
    }
}

int Manage::getRoute(uint32_t modid, uint32_t cmdid, lars::GetRouteResponse &rsp)
{
    int ret = lars::RET_SUCC;

    uint64_t key = ((uint64_t)modid << 32) + cmdid;

    MutexLockGuard lock(_mutex);
    //当前module已经存在_loadbalances中
    if (_loadbalances.find(key) != _loadbalances.end())
    {
        LoadBalance *lb = _loadbalances[key];

        std::vector<HostInfo*> vec;
        lb->getAllHosts(vec);

        for (std::vector<HostInfo*>::iterator it = vec.begin(); it != vec.end(); it++)
        {
            lars::HostInfo host;
            host.set_ip((*it)->_ip);
            host.set_port((*it)->_port);
            rsp.add_host()->CopyFrom(host);
        }

        //若路由并没有处于PULLING状态，且有效期已经超时，则重新拉取，防止dnsserver更新后负载均衡感知不到
        Timestamp currentTime(Timestamp::now());
        if ((lb->_status == LoadBalance::NEW) && 
            (currentTime.microSecondsSinceEpoch() - lb->_lastUpdateTime.microSecondsSinceEpoch() > UPDATE_TIMEOUT*Timestamp::kMicroSecondsPerSecond)) 
        {
            cout << "Manage::getRoute overtime pull now " << endl;
            lb->pullHostFromDns();
        }
    }
    //当前 module 不存在_loadbalances中
    else
    {
        LoadBalance *lb = new LoadBalance(modid, cmdid);
        _loadbalances[key] = lb;
        lb->pullHostFromDns();
        ret = lars::RET_NOEXIST;
        cout << "Manage::getRoute map not exit " << endl; 
    }

    return ret;
}

int Manage::getTheBestHost(uint32_t modid, uint32_t cmdid, lars::GetHostResponse &rsp)
{
    int ret = lars::RET_SUCC;
    uint64_t key = ((uint64_t)modid << 32) + cmdid;

    MutexLockGuard lock(_mutex);
    //判断module是否在 _loadbalances 中，如果在 选择一个返回；如果不在 从dnsService中拉取最新的hostInfo内容并做更新，并返回不存在。
    if (_loadbalances.find(key) != _loadbalances.end()) //存在
    {
        LoadBalance *lb = _loadbalances[key];

        //存在module对应的lb 但里面的host为空，说明正在pull()中，还没有从dnsService返回来，所以直接回复不存在
        if (true == lb->empty())
        {
            assert(lb->_status == LoadBalance::PULLING);
            rsp.set_retcode(lars::RET_NOEXIST);
        }
        else
        {
            //选择一个最佳的返回
            ret = lb->choiceOneHost(rsp);
            rsp.set_retcode(ret);

            //若路由并没有处于PULLING状态，且有效期已经超时，则重新拉取，防止dnsserver更新后负载均衡感知不到
            Timestamp currentTime(Timestamp::now());
            if ((lb->_status == LoadBalance::NEW) && 
                (currentTime.microSecondsSinceEpoch() - lb->_lastUpdateTime.microSecondsSinceEpoch() > UPDATE_TIMEOUT*Timestamp::kMicroSecondsPerSecond)) 
            {
                lb->pullHostFromDns();
            }
        }
    }
    else    //不存在
    {
        LoadBalance *lb = new LoadBalance(modid, cmdid);
        _loadbalances[key] = lb;
        lb->pullHostFromDns();
        rsp.set_retcode(lars::RET_NOEXIST);
        ret = lars::RET_NOEXIST;
    }

    return ret;
}

void Manage::reportHost(lars::ReportRequest req)
{
    uint32_t modid = req.modid();
    uint32_t cmdid = req.cmdid();
    int retcode = req.retcode();
    int ip = req.host().ip();
    int port = req.host().port();
    
    uint64_t key = ((uint64_t)modid << 32) + cmdid;

    MutexLockGuard lock(_mutex);
    if (_loadbalances.find(key) != _loadbalances.end()) 
    {
        LoadBalance *lb = _loadbalances[key];
        //LoadBalance处理当前上报请求，通过当前主机的上报结果在内部调整当前Host是否位于空闲链表还是过载链表中
        lb->adjustHostWhereTo(ip, port, retcode);
        //上报信息给远程reporter服务
        lb->pushCallResultToReport();
    }
}

int Manage::updateLocalHostsMap(uint32_t modid, uint32_t cmdid, lars::GetRouteResponse &rsp)
{
    uint64_t key = ((uint64_t)modid << 32) + cmdid;
    MutexLockGuard lock(_mutex);

    //在_loadbalances中找到对应的key 
    if (_loadbalances.find(key) != _loadbalances.end())
    {
        LoadBalance *lb = _loadbalances[key];

        //如果dnsserver没有返回该 module 对应的任何Host信息，说明dns不在维护了需要将本地的也删除；如果返回了Host信息需要更新到对应的balance中
        if (0 == rsp.host_size())
        {
            delete lb;
            _loadbalances.erase(key);
        }
        else
        {
            lb->updateLocalHostsFromDns(rsp);
        }
    }

    return 0;
}


