#include "StoreReport.h"

StoreReport::StoreReport()
{
    string dbHost("127.0.0.1");
	string dbUser("root");
	string dbPasswd("123456");
	string dbName("lars_dns");
	short dbPort = 3306;
    //多线程使用mysql需要先调用mysql_library_init
    mysql_library_init(0, NULL, NULL);
    mysql_init(&_dbConn);
    my_bool reconnect = 1;
    mysql_options(&_dbConn, MYSQL_OPT_RECONNECT, &reconnect);  //开启mysql断开链接自动重连机制
    mysql_options(&_dbConn, MYSQL_OPT_CONNECT_TIMEOUT, "30");  //设置一个超时定期重连
    if (NULL == mysql_real_connect(&_dbConn, dbHost.c_str(), dbUser.c_str(), dbPasswd.c_str(), dbName.c_str(), dbPort, NULL, 0))
    {
        cout << "StoreReport::StoreReport connect db err! " <<  mysql_error(&_dbConn) << endl;
    }
    else
    {
        cout << "StoreReport::StoreReport connect db succ!" << endl;
    }
}

void StoreReport::store(lars::ReportStatusRequest req)
{
    for (int i = 0; i < req.results_size(); i++) 
    {
        //一条report 调用记录
        const lars::HostCallResult &result = req.results(i);
        int overload = result.overload() ? 1: 0;
        char sql[MY_SQL_SENTENCE_LEN];
        
        snprintf(sql, MY_SQL_SENTENCE_LEN, "INSERT INTO ServerCallStatus"
                "(modid, cmdid, ip, port, caller, succ_cnt, err_cnt, ts, overload) "
                "VALUES (%d, %d, %u, %u, %u, %u, %u, %lld, %d) ON DUPLICATE KEY "
                "UPDATE succ_cnt = %u, err_cnt = %u, ts = %lld, overload = %d",
                req.modid(), req.cmdid(), result.ip(), result.port(), req.caller(),
                result.succ(), result.err(), req.ts(), overload,
                result.succ(), result.err(), req.ts(), overload);
        
        cout << "StoreReport::store INSERT SQL[" << sql << "]" << endl;

        mysql_ping(&_dbConn);//ping 测试一下，防止链接断开，会触发重新建立连接
        if (0 != mysql_real_query(&_dbConn, sql, strlen(sql))) 
        {
            cout << "StoreReport::store Fial to Insert into ServerCallStatus " << mysql_error(&_dbConn) << endl;
        }
    }
}