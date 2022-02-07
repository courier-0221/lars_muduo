#ifndef DNSCOMMON_H
#define DNSCOMMON_H

#include "TcpConnection.h"
#include <stdint.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
using namespace std;

//消息头的长度，固定值  id+len
#define MESSAGE_HEAD_LEN (8)
//消息头+消息体 最大长度限制
#define MESSAGE_LENGTH_LIMIT (65535 - MESSAGE_HEAD_LEN)

//DnsSubscribe
//定义一个订阅列表的关系类型 key-> modid/cmdid,  value->fds(集合)
typedef map<uint64_t, set<int>> subcribeMap;
//定义一个发布列表的关系类型  key-> fd(订阅客户端的连接), value -> modid/cmdid
typedef map<int, set<uint64_t>> publishMap;

//DnsRoute
#define MY_SQL_SENTENCE_LEN (1024)  //sql语句最大长度
//定义用来保存 host Ip/port的集合类型
typedef set<uint64_t> hostSet;
//定义用来保存modID/cmdID 与hosts的Ip/port的对应的数据类型
typedef map<uint64_t, hostSet> routeMap;

//DnsService 
//记录 module modid/cmdid集合
typedef set<uint64_t> modSet;
typedef std::shared_ptr<modSet> modSetPtr;
//记录每一个fd请求了哪些 module
typedef std::map<int, modSetPtr> RequestModMap;
//在线的客户端
typedef std::set<int> OnlineClient;
//记录每一个fd对应的TcpConn
typedef std::map<int, TcpConnectionPtr> ClientMap;

#endif