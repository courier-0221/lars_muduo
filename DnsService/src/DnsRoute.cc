#include "DnsRoute.h"
#include "DnsService.h"
#include "EventLoop.h"

extern std::shared_ptr<DnsService> gDnsService;
extern EventLoop gLoop;

DnsRoute::DnsRoute()
    : _mapLock()
    , _version(0)
{
    cout << "DnsRoute::DnsRoute()" << endl;
    if (0 == connectMysql()) { cout << "connectMysql error" << endl; }
    buildRouteMapA();   //将数据库中的RouteData的数据加载到_routeDataMapA中
    removeChanges(true);//删除RouteChange表中所有更改信息
    gLoop.runEvery(2, std::bind(&DnsRoute::checkRouteChangesFunc, this));   //定时任务监控数据库中是否有mod发生了变更
}

int DnsRoute::connectMysql()
{
    string dbHost("127.0.0.1");
	string dbUser("root");
	string dbPasswd("123456");
	string dbName("lars_dns");
	short dbPort = 3306;
    mysql_init(&_dbConn);
    my_bool reconnect = 1;
    mysql_options(&_dbConn, MYSQL_OPT_RECONNECT, &reconnect);  //开启mysql断开链接自动重连机制
    mysql_options(&_dbConn, MYSQL_OPT_CONNECT_TIMEOUT, "30");  //设置一个超时定期重连
    if (NULL == mysql_real_connect(&_dbConn, dbHost.c_str(), dbUser.c_str(), dbPasswd.c_str(), dbName.c_str(), dbPort, NULL, 0))
    {
        cout << "DnsRoute::connectMysql connect db err! " <<  mysql_error(&_dbConn) << endl;
        return 0;
    }
    else
    {
        cout << "DnsRoute::connectMysql connect db succ!" << endl;
        return 1;
    }
}

void DnsRoute::buildRouteMapA() 
{
    char sql[MY_SQL_SENTENCE_LEN] = {0};
    MYSQL_ROW row = {0};

    //查找数据库将数据库中的内容加载进 _routeDataMapA 中
    snprintf(sql, MY_SQL_SENTENCE_LEN, "SELECT * FROM RouteData;");
    if (0 != mysql_real_query(&_dbConn, sql, strlen(sql))) 
    {
        cout << "DnsRoute::buildRouteMapA select RouteData error " << mysql_error(&_dbConn) << endl;
    }
    MYSQL_RES *result = mysql_store_result(&_dbConn);
    long lineNum = mysql_num_rows(result);
    for (int i = 0; i < lineNum; i++) 
    {
        row = mysql_fetch_row(result);
        uint32_t modid = atoi(row[1]);
        uint32_t cmdid = atoi(row[2]);
        uint32_t ip = atoi(row[3]);
        uint32_t port = atoi(row[4]);
        uint64_t key = (((uint64_t)modid) << 32) + cmdid;
        uint64_t value = (((uint64_t)ip) << 32) + port;
        _routeDataMapA[key].insert(value);
        cout << "DnsRoute::buildRouteMapA modid = " << modid << " cmdid " << cmdid << " ip " << ip << " port " << port << endl;
    }
    mysql_free_result(result);
}

void DnsRoute::loadRouteMapB()
{
    char sql[MY_SQL_SENTENCE_LEN] = {0};
    MYSQL_ROW row = {0};

    _routeDataMapB.clear();

    //查找数据库将数据库中的内容加载进 _routeDataMapB 中
    snprintf(sql, MY_SQL_SENTENCE_LEN, "SELECT * FROM RouteData;");
    if (0 != mysql_real_query(&_dbConn, sql, strlen(sql))) 
    {
        cout << "DnsRoute::loadRouteMapB select RouteData error" << mysql_error(&_dbConn) << endl;
    }
    MYSQL_RES *result = mysql_store_result(&_dbConn);
    long lineNum = mysql_num_rows(result);
    for (int i = 0; i < lineNum; i++) 
    {
        row = mysql_fetch_row(result);
        int32_t modid = atoi(row[1]);
        int32_t cmdid = atoi(row[2]);
        int32_t ip = atoi(row[3]);
        int32_t port = atoi(row[4]);
        uint64_t key = (((uint64_t)modid) << 32) + cmdid;
        uint64_t value = (((uint64_t)ip) << 32) + port;
        _routeDataMapB[key].insert(value);
        //cout << "DnsRoute::loadRouteMapB modid = " << modid << " cmdid " << cmdid << " ip " << ip << " port " << port << endl;
    }
    mysql_free_result(result);
}

hostSet DnsRoute::getHosts(uint32_t modid, uint32_t cmdid)
{
    hostSet hosts;
    uint64_t key = (((uint64_t)modid) << 32) + cmdid;

    RwLockReadGuard rlock(_mapLock);
    routeMap::iterator it = _routeDataMapA.find(key);
    if (it != _routeDataMapA.end()) 
    {
        hosts = it->second; //找到了对应的hostSet
    }

    return hosts;
}

int DnsRoute::loadVersion()
{
    char sql[MY_SQL_SENTENCE_LEN] = {0};
    snprintf(sql, MY_SQL_SENTENCE_LEN,  "SELECT version from RouteVersion WHERE id = 1;");
    if (0 != mysql_real_query(&_dbConn, sql, strlen(sql))) 
    {
        cout << "DnsRoute::loadVersion select RouteVersion error " << mysql_error(&_dbConn) << endl;
        return -1;
    }
    MYSQL_RES *result = mysql_store_result(&_dbConn);
    long lineNum = mysql_num_rows(result);
    if (0 == lineNum) 
    {
        cout << "DnsRoute::loadVersion No version in table Routeversion " << mysql_error(&_dbConn) << endl;
        return -1;
    }
    
    //得到version并判断是否发生改变
    MYSQL_ROW row = mysql_fetch_row(result);
    long new_version = atol(row[0]);
    if (new_version == _version) 
    {
        mysql_free_result(result);
        return 0;
    }
    else
    {
        _version = new_version;
        cout << "DnsRoute::loadVersion now new route version is " << _version << endl;
        mysql_free_result(result);
        return 1;
    }
}

void DnsRoute::loadChanges(std::vector<uint64_t>& changeList)
{
    char sql[MY_SQL_SENTENCE_LEN] = {0};
    MYSQL_ROW row = {0}; 

    //获取最新的修改版本数据
    snprintf(sql, MY_SQL_SENTENCE_LEN, "SELECT modid,cmdid FROM RouteChange WHERE version >= %ld;", _version);
    if (0 != mysql_real_query(&_dbConn, sql, strlen(sql))) 
    {
        cout << "DnsRoute::loadChanges select RouteVersion error: " << mysql_error(&_dbConn) << endl;
    }
    MYSQL_RES *result = mysql_store_result(&_dbConn);
    long lineNum = mysql_num_rows(result);
    if (lineNum == 0)   //没有更改直接返回
    {
        cout << "DnsRoute::loadChanges no Change in RouteChange: " << mysql_error(&_dbConn) << endl;
        return ;
    }
    for (long i = 0; i < lineNum; i ++) 
    {
        row = mysql_fetch_row(result);
        int modid = atoi(row[0]);
        int cmdid = atoi(row[1]);
        uint64_t mod = (((uint64_t)modid) << 32) + cmdid;
        changeList.push_back(mod);
        cout << "DnsRoute::loadChanges changedInfo modid = " << modid << " cmdid " << cmdid << endl;
    }
    mysql_free_result(result);
}

void DnsRoute::removeChanges(bool removeAll)
{
    char sql[MY_SQL_SENTENCE_LEN] = {0};
    if (false == removeAll)
    {
        snprintf(sql, MY_SQL_SENTENCE_LEN, "DELETE FROM RouteChange WHERE version <= %ld;", _version);
    }
    else
    {
        snprintf(sql, MY_SQL_SENTENCE_LEN, "DELETE FROM RouteChange;");
    }
    if (0 != mysql_real_query(&_dbConn, sql, strlen(sql)))
    {
        cout << "DnsRoute::removeChanges delete RouteChange: " << mysql_error(&_dbConn) << endl;
    } 
}

void DnsRoute::swap()
{
    RwLockWriteGuard guard(_mapLock);
    _routeDataMapB.swap(_routeDataMapA);
}

void DnsRoute::checkRouteChangesFunc(void)
{
    int loadTime = 10; //10s自动加载RouteData一次
    Timestamp currentTime(Timestamp::now());

    int ret = loadVersion();
    if (1 == ret)   //version被更改表中mod数据发生改变
    {
        //将数据库中最新的RouteData表中的数据加载到 _routeDataMapB 中
        loadRouteMapB();
        //将 _routeDataMapB 的数据更新到 _routeDataMapA 中
        swap();
        //获取当前已经被修改的modid/cmdid集合vector
        std::vector<uint64_t> changes;
        loadChanges(changes);
        //给订阅修改的mod客户端推送消息
        gDnsService->_dnsSubcribe.publish(changes);
        //删除当前版本之前的修改记录
        removeChanges(false);
        //更新时间
        _lastLoadTime = currentTime;
    }
    else    //version没有被修改，表中数据没有改变，超时时间到也加载到mapB
    {
        if (currentTime.microSecondsSinceEpoch() - _lastLoadTime.microSecondsSinceEpoch() >= loadTime*Timestamp::kMicroSecondsPerSecond) 
        {
            loadRouteMapB();
            swap();
            _lastLoadTime = currentTime;
        }
    }
}
