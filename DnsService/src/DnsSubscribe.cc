#include "DnsSubscribe.h"
#include "DnsService.h"

extern std::shared_ptr<DnsService> gDnsService;

DnsSubscribe::DnsSubscribe() 
    : _bookList()
    , _bookListLock()
    , _pushList()
    , _pushListLock()
{
}

void DnsSubscribe::subscribe(uint64_t mod, int fd)
{
    //将mod-->fd对应关系加入_bookList中
    MutexLockGuard lock(_bookListLock);
    _bookList[mod].insert(fd);
}

void DnsSubscribe::unsubscribe(uint64_t mod, int fd)
{
    //将mod-->fd对应关系从_bookList中删除
    MutexLockGuard lock(_bookListLock);
    if (_bookList.find(mod) != _bookList.end()) 
    {
        _bookList[mod].erase(fd);
        if (_bookList[mod].empty() == true) 
        {
            _bookList.erase(mod);
        }
    }
}

void DnsSubscribe::makePublishMap(OnlineClient &onlineClient, publishMap &needPublish)
{
    MutexLockGuard lock(_pushListLock);
    publishMap::iterator it;
    //遍历_pushList 找到 onlineClient匹配的数据 放到needPublish中
    //_pushList中记录的clientfd是大于onlineClient的，在这个位置将断开连接的客户端清除掉
    for (it = _pushList.begin(); it != _pushList.end(); it++)
    {
        if (onlineClient.find(it->first) != onlineClient.end()) 
        {
            modSet::iterator ms_it;
            for (ms_it = _pushList[it->first].begin(); ms_it != _pushList[it->first].end(); ms_it++) 
            {
                needPublish[it->first].insert(*ms_it);
            }
        }
        else    //没找到说明客户端已经断开了连接需要做取消订阅操作
        {
            modSet::iterator ms_it;
            for (ms_it = _pushList[it->first].begin(); ms_it != _pushList[it->first].end(); ms_it++) 
            {
                unsubscribe(*ms_it, it->first);
            }
        }
    }
}

void DnsSubscribe::pushChangeTask(void)
{
    //获取全部在线的客户端fd
    OnlineClient onlineClient = gDnsService->getAllOnlineClient();

    //从 _pushList 中找到与 onlineClient 集合匹配的 fd， 放到一个新的 publishMap 中
    publishMap needPublish;//真正需要发布的订单
    needPublish.clear();
    makePublishMap(onlineClient, needPublish); 

    //依此从 needPublish 中取出数据 发送给对应的客户端
    publishMap::iterator it;
    for (it = needPublish.begin(); it != needPublish.end(); it ++) 
    {
        int fd = it->first;

        modSet::iterator ms_it;
        //遍历fd对应的 modid/cmdid的集合
        for (ms_it = it->second.begin(); ms_it != it->second.end(); ms_it++) 
        {
            Buffer buf = {0};
		    MsgHead msghead = {0};
		    string msgdata = {0};

            int modid = int((*ms_it) >> 32);
            int cmdid = int(*ms_it);

            //msgdata
            lars::GetRouteResponse rsp;
            rsp.set_modid(modid);
            rsp.set_cmdid(cmdid);

            //通过route查询对应host信息 进行封装 
            hostSet hosts = gDnsService->_dnsRoute.getHosts(modid, cmdid);
            for (hostSet::iterator hs_it = hosts.begin(); hs_it != hosts.end(); hs_it++) 
            {
                uint64_t ip_port_pair = *hs_it; 
                lars::HostInfo hostInfo;
                hostInfo.set_ip((uint32_t)(ip_port_pair>>32));
                hostInfo.set_port((int)(ip_port_pair));
                rsp.add_host()->CopyFrom(hostInfo);
            }

            //将rsp 发送给对应的 clientfd
            rsp.SerializeToString(&msgdata);
		    buf.append(msgdata.data(), msgdata.size());

            //msghead
            msghead.msgId = hostToNetwork32(lars::ID_GetRouteResponse);
            msghead.msgLen = hostToNetwork32(static_cast<int32_t>(msgdata.size()));
            buf.prepend(&msghead, sizeof msghead);
            //msghead + msgdata ==> send
            string response(buf.peek(), buf.readableBytes());
            //通过fd得到连接对象
            TcpConnectionPtr conn = gDnsService->_clientMap[fd];
            conn->send(response);

            _pushList.erase(fd);
        }
    }
}

void DnsSubscribe::publish(std::vector<uint64_t> &changeMods)
{
    MutexLockGuard booklock(_bookListLock);
    MutexLockGuard pushlock(_pushListLock);
    
    std::vector<uint64_t>::iterator it;
    //处理所有的变更mod
    for (it = changeMods.begin(); it != changeMods.end(); it++) 
    {
        uint64_t mod = *it;
        //当前发生修改的mod有被客户端订阅，找到对应所有的clientFds，按照结构添加到publishMap中做推送
        if (_bookList.find(mod) != _bookList.end())
        {
            set<int>::iterator fds_it;
            for (fds_it = _bookList[mod].begin(); fds_it != _bookList[mod].end(); fds_it++) 
            {
                _pushList[*fds_it].insert(mod);
            }
        }
    }

    //主动推送消息到对应客户端，给_pushList中的每个fd发送新的modid/cmdid对应主机信息 
    gDnsService->_threadpool.addTask(std::bind(&DnsSubscribe::pushChangeTask, this));
}
