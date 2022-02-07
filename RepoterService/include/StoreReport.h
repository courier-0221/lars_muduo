#ifndef STOREREPORT_H
#define STOREREPORT_H

#include <string>
#include <iostream>
#include <mysql/mysql.h>
#include "lars.pb.h"
using namespace std;

#define MY_SQL_SENTENCE_LEN (1024)  //sql语句最大长度

class StoreReport
{
public:
    StoreReport();
    void store(lars::ReportStatusRequest req);  //存储进数据库
private:
    MYSQL _dbConn;
};

#endif