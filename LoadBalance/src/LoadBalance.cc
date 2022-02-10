#include "LoadBalance.h"
#include "AgentServer.h"

extern std::shared_ptr<AgentServer> gAgentServer;

LoadBalance::LoadBalance(uint32_t modid, uint32_t cmdid)
    : _status(PULLING)
    , _lastUpdateTime(0)
    , _modid(modid)
    , _cmdid(cmdid)
{
}

LoadBalance::~LoadBalance()
{}

bool LoadBalance::empty() const
{
    return _hostMap.empty();
}

void LoadBalance::getAllHosts(std::vector<HostInfo*> &vec)
{
    HostMap::iterator it;
    for (it = _hostMap.begin(); it != _hostMap.end(); it++) 
    {
        HostInfo *hi = it->second;
        vec.push_back(hi);
    }
}

int LoadBalance::choiceOneHost(lars::GetHostResponse &rsp)
{
    //如果空闲链表为空说明没有空闲节点了，尝试从过载链表中拿节点
    if (true == _idleList.empty())
    {
        if (_access_cnt >= PROBE_NUM) //判断访问次数是否已经超过了probe_num
        {
            _access_cnt = 0;
            getHostFromList(rsp, _overloadList);
            cout << "LoadBalance::choiceOneHost _idleList is empty--->choice from _overloadList!!!" << endl;
        }
        else
        {
            ++_access_cnt;
            return lars::RET_OVERLOAD;  //直接返回已经过载
        }
    }
    //如果空闲链表不为空
    else
    {
        //判断是否有Host过载，如果有过载根据访问次数卡阈值的方式决定从哪个链表中拿，如果没有过载从空闲链表中拿
        if (true == _overloadList.empty())
        {
            getHostFromList(rsp, _idleList);
        }
        else
        {
            //判断访问次数是否超过probe_num阈值，超过从 过载链表 取出一个，没超过从 空闲链表 取出一个
            if (_access_cnt >= PROBE_NUM)
            {
                getHostFromList(rsp, _overloadList);
                struct in_addr saddr;
                saddr.s_addr = htonl(rsp.host().ip());
                cout << "LoadBalance::choiceOneHost _access_cnt = " << _access_cnt << " _probe_num = " << PROBE_NUM
                     << " ---> choice from _overloadList ip = " << inet_ntoa(saddr) << " port = " << rsp.host().port() << endl;
                _access_cnt = 0;
            }
            else
            {
                ++_access_cnt;
                getHostFromList(rsp, _idleList);
            }
        }
    }

    return lars::RET_SUCC;
}

void LoadBalance::getHostFromList(lars::GetHostResponse &rsp, HostList &l)
{
    //选择list中第一个节点
    HostInfo *host = l.front();

    //HostInfo自定义类型,proto3并没提供set方法,而是通过mutable_接口返回HostInfo的指针,可根据此指针进行赋值操作
    lars::HostInfo* hip = rsp.mutable_host();
    hip->set_ip(host->_ip);
    hip->set_port(host->_port);

    //将上面处理过的第一个节点放在队列的末尾
    l.pop_front();
    l.push_back(host);
}

void LoadBalance::pullHostFromDns()
{
    Buffer buf = {0};
    MsgHead msghead = {0};
    string msgdata = {0};

    //msgdata
    lars::GetRouteRequest req;
    req.set_modid(_modid);
    req.set_cmdid(_cmdid);
    req.SerializeToString(&msgdata);
    buf.append(msgdata.data(), msgdata.size());
    //msghead
    msghead.msgId = hostToNetwork32(lars::ID_GetRouteRequest);
    msghead.msgLen = hostToNetwork32(static_cast<int32_t>(msgdata.size()));
    buf.prepend(&msghead, sizeof msghead);
    //msghead + msgdata ==> send
    string request(buf.peek(), buf.readableBytes());
    //通过dns client的conn 向 DnsService 发送请求
    TcpConnectionPtr conn = gAgentServer->_dnsClient.getConnection();
    conn->send(request);

    //由于远程会有一定延迟，所以应该给当前的load_balance模块标记一个正在拉取的状态
    _status = PULLING;  

    cout << "LoadBalance::pullHostFromDns send request msgId " << lars::ID_GetRouteRequest 
        << " modid " << _modid << " cmdid " << _cmdid << endl;
}

void LoadBalance::updateLocalHostsFromDns(lars::GetRouteResponse &rsp)
{
    std::set<uint64_t> dnsReturnHosts;
    std::set<uint64_t> needDeleteHosts;

    //插入 如果自身的_hostMap找不到 DnsService 返回的key，说明是新增，加入到两个位置 _hostMap + _idleList
    for (int i = 0; i < rsp.host_size(); i++) 
    {
        const lars::HostInfo& h = rsp.host(i);
        uint64_t key = ((uint64_t)h.ip() << 32) + h.port();

        dnsReturnHosts.insert(key);   //记录dnsserver返回了哪些key

        if (_hostMap.find(key) == _hostMap.end()) //找不到则新增
        {
            HostInfo *hi = new HostInfo(h.ip(), h.port(), INIT_SUCC_CNT);
            _hostMap[key] = hi;
            _idleList.push_back(hi);
        }
    }
   
    //删除 如果该key 在 _hostMap中存在 & 在dnsserver返回的结果集不存在，需要锁定被删除
    for (HostMap::iterator it = _hostMap.begin(); it != _hostMap.end(); it++) 
    {
        if (dnsReturnHosts.find(it->first) == dnsReturnHosts.end())
        {    
            needDeleteHosts.insert(it->first);
        }
    }
    for (std::set<uint64_t>::iterator it = needDeleteHosts.begin(); it != needDeleteHosts.end(); it++)
    {
        uint64_t key = *it;
        HostInfo *hi = _hostMap[key];
        if (hi->_overload == true)
        {
            _overloadList.remove(hi);
        } 
        else
        {
            _idleList.remove(hi);
        }
        delete hi;
    }

    //更新从DNS server获取内容的时间戳，并将状态设置为NEW
    _lastUpdateTime = Timestamp(Timestamp::now());
    _status = NEW;
}

void LoadBalance::adjustHostWhereTo(uint32_t ip, uint32_t port, int retcode)
{
    uint64_t key = ((uint64_t)ip << 32)  + port;
    
    if (_hostMap.find(key) == _hostMap.end()) 
    {
        return;
    }

    //1.计数统计
    HostInfo *hi = _hostMap[key];
    if (retcode == lars::RET_SUCC)  // retcode == 0
    {
        //更新虚拟成功、真实成功次数
        hi->_vsucc++;
        hi->_rsucc++;
        //连续成功增加
        hi->_continSucc++; 
        //连续失败次数归零
        hi->_continErr = 0;
    }
    else
    {
        //更新虚拟失败、真实失败次数 
        hi->_verr++;
        hi->_rerr++;
        //连续失败个数增加
        hi->_continErr++;
        //连续成功次数归零
        hi->_continSucc = 0;
    }

    //2.检查节点状态，检查idle节点是否满足overload条件，或者overload节点是否满足idle条件
    //必要项：根据调用状态返回值做判定
    //--> 如果是dile节点,则只有调用失败才有必要判断是否达到overload条件；两个判断条件
    if (hi->_overload == false && retcode != lars::RET_SUCC)
    {
        bool overload = false;
        //条件一.失败率大于预设值失败率，判定为overload
        double errRate = hi->_verr * 1.0 / (hi->_vsucc + hi->_verr);
        if (errRate > ERR_RATE)
        {
            overload = true;            
        }

        //条件二.连续失败次数达到阈值，判定为overload
        if (overload == false && hi->_continErr >= CONTIN_ERR_LIMIT)
        {
            overload = true;
        }

        //当前主机Host判定为过载后需要将它 从空闲链表拿出 放入到过载链表
        if (true == overload)
        {
            struct in_addr saddr;
            saddr.s_addr = htonl(hi->_ip);
            cout << "LoadBalance::report module[" << _modid << "," << _cmdid << "] " << " host[" << inet_ntoa(saddr) << "+" << hi->_port
                << "] change to overload!!! vsucc " << hi->_vsucc << " verr " << hi->_verr << endl;

            hi->setOverload();
            _idleList.remove(hi);
            _overloadList.push_back(hi);
            return;
        }

    }
    //--> 如果是overload节点，则只有调用成功才有必要判断是否达到idle条件，两个判断条件
    else if (hi->_overload == true && retcode == lars::RET_SUCC)
    {
        bool idle = false;

        //条件一.成功率大于预设值的成功率，判定为idle
        double succRate = hi->_vsucc * 1.0 / (hi->_vsucc + hi->_verr);
        if (succRate > SUCC_RATE)
        {
            idle = true;
        }

        //条件二.连续成功次数达到阈值，判定为idle
        if (idle == false && hi->_continSucc >= CONTIN_SUCC_LIMIT)
        {
            idle = true;
        }

        //当前主机Host判定为空闲后需要将它 从过载链表拿出 放入到空闲链表
        if (true == idle)
        {
            struct in_addr saddr;
            saddr.s_addr = htonl(hi->_ip);
            cout << "LoadBalance::report module[" << _modid << "," << _cmdid << "] " << " host[" << inet_ntoa(saddr) << "+" << hi->_port
                << "] change to idle!!! vsucc " << hi->_vsucc << " verr " << hi->_verr << endl;

            hi->setIdle();
            _overloadList.remove(hi);
            _idleList.push_back(hi);
            return;
        }
    }
    
    //3.超时&窗口检查机制
    //超时:当前时间戳与上一次主机节点变为空闲或者过载的时间戳是否超过设定的阈值
    //窗口检查:判定节点请求的真实失败率是否超过设定值
    //优化项：根据超时时间强制判断一次，判定主机节点真实失败率是否超过设定值
    Timestamp currentTime(Timestamp::now());
    if (false == hi->_overload) //idle状态Host
    {
        if (currentTime.microSecondsSinceEpoch() - hi->_idlePts.microSecondsSinceEpoch() >= IDLE_TIMEOUT*Timestamp::kMicroSecondsPerSecond)
        {
            if (hi->checkWindow() == true)  //真实失败率高于阈值，设置为过载
            {
                struct in_addr saddr;
                saddr.s_addr = htonl(hi->_ip);
                cout << "LoadBalance::report module[" << _modid << "," << _cmdid << "] " << " host[" << inet_ntoa(saddr) << "+" << hi->_port
                    << "]  change to overload cause windows err rate too high!!! rsucc " << hi->_rsucc << " rerr " << hi->_rerr << endl;

                hi->setOverload();
                _idleList.remove(hi);
                _overloadList.push_back(hi);
            }
            else
            {
                //重置窗口给默认信息
                hi->setIdle();
            }
        }
    }
    else    //overload状态Host
    {
        //超时时间到达给过载Host节点一个机会放入空闲链表中尝试
        if (currentTime.microSecondsSinceEpoch() - hi->_overloadPts.microSecondsSinceEpoch() >= OVERLOAD_TIMEOUT*Timestamp::kMicroSecondsPerSecond)
        {
            struct in_addr saddr;
            saddr.s_addr = htonl(hi->_ip);
            cout << "LoadBalance::report module[" << _modid << "," << _cmdid << "] " << " host[" << inet_ntoa(saddr) << "+" << hi->_port
                    << "]  reset to idle!!! vsucc " << hi->_vsucc << " verr " << hi->_verr << endl;

            hi->setIdle();
            _overloadList.remove(hi);
            _idleList.push_back(hi);
        }
    }
}

void LoadBalance::pushCallResultToReport()
{
    if (empty() == true) 
    {
        return;
    }

    //本地ip
    struct in_addr inaddr;
    inet_aton("127.0.0.1", &inaddr);
    uint32_t localIp = ntohl(inaddr.s_addr);

    Buffer buf = {0};
	MsgHead msghead = {0};
	string msgdata = {0};

    //msgdata
    lars::ReportStatusRequest req;
    req.set_modid(_modid);
    req.set_cmdid(_cmdid);
    Timestamp pts(Timestamp::now());
    req.set_ts(pts.microSecondsSinceEpoch()/Timestamp::kMicroSecondsPerSecond);
    req.set_caller(localIp);
    //从idleList取值
    for (HostList::iterator it = _idleList.begin(); it != _idleList.end(); it++)
    {
        HostInfo *hi = *it;    
        lars::HostCallResult call_res;
        call_res.set_ip(hi->_ip);
        call_res.set_port(hi->_port);
        call_res.set_succ(hi->_rsucc);
        call_res.set_err(hi->_rerr);
        call_res.set_overload(false);
    
        req.add_results()->CopyFrom(call_res);
    }
    //从overList取值
    for (HostList::iterator it = _overloadList.begin(); it != _overloadList.end(); it++)
    {
        HostInfo *hi = *it;
        lars::HostCallResult call_res;
        call_res.set_ip(hi->_ip);
        call_res.set_port(hi->_port);
        call_res.set_succ(hi->_rsucc);
        call_res.set_err(hi->_rerr);
        call_res.set_overload(true);
    
        req.add_results()->CopyFrom(call_res);
    }
    req.SerializeToString(&msgdata);
    buf.append(msgdata.data(), msgdata.size());
    
    //msghead
    msghead.msgId = hostToNetwork32(lars::ID_ReportStatusRequest);
    msghead.msgLen = hostToNetwork32(static_cast<int32_t>(msgdata.size()));
    buf.prepend(&msghead, sizeof msghead);
    //msghead + msgdata ==> send
    string request(buf.peek(), buf.readableBytes());
    //通过repoter client的conn 向 repoter sercice 发送请求
    TcpConnectionPtr conn = gAgentServer->_repoterClient.getConnection();
    conn->send(request);

    cout << "LoadBalance::pushCallResultToReport msgId " << lars::ID_ReportStatusRequest << " msgLen " << msgdata.size() 
			<< " modid " << _modid << " cmdid " << _cmdid << endl;
}
