# DnsService功能简介

每一个module(modid+cmdid)下可能有多个host(ip+port)，该内容记录在数据库中。

该服务用于接收客户端对某module的请求并返回该module下的所有主机，即为客户端提供获取路由服务

# 编译与运行

mysql客户端安装：

​	该功能涉及到了对mysql数据库的操作，所以运行环境需要安装mysql服务器端和连接端。

​    这里安装的是C接口的，ubuntu下可以采用该命令进行安装sudo apt-get install libmysqlclient-dev 

​    或者去官网下载连接器

​	编译时-lmysqlclient

protobuf安装：

​	该功能使用protobuf实现了主机请求与响应的组包协议，可以到官网编译安装

​    git clone https://github.com/protocolbuffers/protobuf.git

​	编译时 -std=c++11 -pthread -lprotobuf -lpthread

# 实现

## 1.思路

框图：

![](./img/系统框图(类视角).png)



其中dnsService使用简化版本muduo库，采用 one loop per thread TCP服务器模型，线程池实现多计算线程处理多客户端业务请求，业务上DnsService接收客户端获取mod对应hosts请求(与DnsRoute交互)并构造相应数据返回同时使用DnsSubscibe模块的订阅功能；

DnsRoute主要负责和数据可交互，使用muduo简化版中的定时器功能定时检测数据库是否发生了内容变化(RouteVersion表中内容)，如果发生变化及时更新到提供给DnsService查询使用的映射表中，发生变化的同时使用DnsSubscibe模块的推送功能进行推送；

DnsSubscibe模块负责订阅与推送功能订阅时记录相应的mod及对应的fds，推送时从DnsService获取到所有的在线客户端，从DnsRoute处获取到发生变化的所有mod，在结合订阅链表整理出在线客户端订阅过的并且发生修改了的mod集合，构造好对应的host集合并整理成task任务交给DnsService的线程池做分发给各个客户端的处理。



## 2.mysql表结构

- 表`RouteData`: 保存了所有module与host的信息.

| 字段       | 数据类型         | 是否可以为空 | 主键 | 默认 | 附加   | 说明         |
| ---------- | ---------------- | ------------ | ---- | ---- | ------ | ------------ |
| id         | int(10) unsigned | No           | 是   | NULL | 自增长 | 该条数据ID   |
| modid      | int(10) unsigned | No           |      | NULL |        | 模块ID       |
| cmdid      | int(10) unsigned | No           |      | NULL |        | 指令ID       |
| serverip   | int(10) unsigned | No           |      | NULL |        | 服务器IP地址 |
| serverport | int(10) unsigned | No           |      | NULL |        | 服务器端口   |

- 表`RouteVersion`: 当前`RouteData`路由版本号，每次管理端修改某mod的路由，`RouteVersion`表中的版本号都被更新为当前时间戳

| 字段    | 数据类型         | 是否可以为空 | 主键 | 默认 | 附加   |
| ------- | ---------------- | ------------ | ---- | ---- | ------ |
| id      | int(10) unsigned | No           | 是   | NULL | 自增长 |
| version | int(10) unsigned | No           |      | NULL |        |

- 表`RouteChange`: 每次管理端修改某mod的路由，会记录本次对哪个mod进行修改（增、删、改），以便指示最新的`RouteData`路由有哪些mod变更了。

| 字段    | 数据类型            | 是否可以为空 | 主键 | 默认 | 附加   |
| ------- | ------------------- | ------------ | ---- | ---- | ------ |
| id      | int(10) unsigned    | No           | 是   | NULL | 自增长 |
| modid   | int(10) unsigned    | No           |      | NULL |        |
| cmdid   | int(10) unsigned    | No           |      | NULL |        |
| version | bigint(20) unsigned | No           |      | NULL |        |



mod增删改处理流程：

​	如果需要增删改，需要操作三张表来完成，第一张为RouteData(进行数据更改)，将要更改的mod操作完成后操作第二张RouteChange(记录此次更改了那些module)，第三张为RouteVersion，做version更新，这一步会让DnsService感知，数据库中的mod做了更改。



## 3.相关类

### 3.1.DnsService

数据结构：

modSet:(set) key(modid+cmdid)    *记录 mod modid/cmdid集合*

requestModMap:(map) key(clientFd) <==> value(modSet)	*记录client(clientFd)请求过的modSet*

OnlineClient:(set) key(clientFd)记录连接上来的clientFd	*记录client(clientFd)*

ClientMap:(map) key(clientFd) <==> value(TcpConnection)	*记录client(clientFd)对应的TcpConnection*



实现：

1.客户端连接：

(1).将clientfd记录到OnlineClient数据结构中

(2).将clientfd和对应的TcpConnection记录到ClientMap数据结构中

(3).new一个modSet，将clientfd和modSet记录到requestModMap数据结构中

2.处理客户端发起的请求：

请求会带着modid和cmdid==>mod

(1).订阅功能

​	判断当前客户端是否有订阅过mod，(从requestModMap中根据clientFd找到对应的modSet，然后根据mod在modSet中查找)，找不到则使用DnsSubcribe的订阅接口订阅。

(2).根据mod从数据库中找到对应的hostSet返回给客户端

​	调用DnsRoute的获取主机接口

3.路由转发：

​	根据注册的requestid转到对应的回调接口处执行。

总结：

​	调用DnsRoute的接口获取mod对应的host信息，调用DnsSubcribe的订阅接口



### 3.2.DnsRoute

数据结构：

hostSet:(set) ip+port	 *主机*

routeMap:(map) key(modid+cmdid) <==> value(hostSet==>n个主机)	*module与hosts的对应关系*



实现：

 维护两张routeMap表，mapA和mapB。mapA的作用是给客户端查询使用的；mapB的作用是当数据库中有更改时先将数据库RouteData表中的数据加载到mapB中然后再更新到mapA中，起到缓冲作用。

1.初始化

​	连接数据库，从数据库中加载内容到mapA中

2.检查数据库中的routeMap表是否改变

​	结合框架中epoll监听的定时器来实现，每2s判断一次。判断条件为数据库dns表中的version字段是否变化

(1).发生变化

​	a.将数据库中最新的RouteData表中的数据加载到 临时的MapB 中，然后更新到mapA中

​	b.加载RouteChange表中的修改项给DnsSubcribe模块做推送使用

​	c.删除当前版本之前的修改记录

(2).没发生变化

​	只需要将数据库中最新的RouteData表中的数据加载到 临时的MapB 中，然后更新到mapA中

总结：

​	从数据库中映射出mod与hosts的关系到mapA提供给DnsService查询使用，定时器函数中定时监控数据库中是否有mod发生了变化，如果有则使用DnsSubcribe通知给订阅过的客户端。

### 3.3.DnsSubscribe

数据结构：

subcribeMap: (map)  key(modid+cmdid) <==> value(clientFds[set类型])*订阅表记录module对应的所有客户端*

publishMap: (map)  key(clientFd) <==> value(mods[set类型])*发布表记录客户端订阅的哪些module发生了修改*



实现：

维护一张订阅表与一张发布表。

1.订阅

​	DnsServer中处理客户端发起的请求时调用该模块的订阅功能，拿到modid+cmdid和clientFd将其存储到subcribeMap表中

2.发布

​	DnsRoute的轮询任务中检测到数据库module与host发生了变更，获取到变更内容(指module)传入到改模块的发布功能，然后调用该模块的发布功能通知对应的client。

(1).根据changeMods vector 从 subcribeMap 查找并构造出 publishMap 内容

(2).将推送任务放到线程池的任务中做，订阅模块只对请求module的客户端做记录，有可能客户端断开了连接该模块没有感知，所以只对服务器端还在线的客户端做推送。也就是根据服务器端还在线的客户端从pulishMap中再次过滤出有效数据，pulishMap中无用数据就可以清除掉了(客户端断开连接没有取消订阅)。

(3).根据mod从DnsRoute模块找到数据库中更改后的内容然后组包发给对应客户端。

总结：

​	发布功能整合两部分资源，一部分为DnsService中当前在线的客户端，一部分为DnsRoute中发生了变化的mod，然后将最新的mod与hosts关系通知给对应的客户端。