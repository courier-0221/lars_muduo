# ReportService功能简介

 负责接收各agent对某modid、cmdid下节点的调用状态的上报(存储到数据库中)。 

功能1.Agent将某个host主机(ip,host) 本次的调用结果 发送给report service

功能2.用存储线程池提高大压力下数据库的入库操作。

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

其中ReportService使用简化版本muduo库，采用 one loop per thread TCP服务器模型，线程池实现多计算线程处理多客户端业务请求，业务上ReportService处理"客户端对某modid、cmdid下节点的调用状态的上报"请求(与StoreReport交互)存储到数据库中；

SroreReport主要负责和数据库交互，提供存储接口。

## 2.mysql表结构

- 表`ServerCallStatus`:

| 字段     | 数据类型   | 是否可以为空 | 主键 | 默认 | 附加 | 说明         |
| -------- | ---------- | ------------ | ---- | ---- | ---- | ------------ |
| modid    | int(11)    | No           | 是   | NULL |      | 模块ID       |
| cmdid    | int(11)    | No           | 是   | NULL |      | 指令ID       |
| ip       | int(11)    | No           | 是   | NULL |      | 服务器IP地址 |
| port     | int(11)    | No           | 是   | NULL |      | 服务器端口   |
| caller   | int(11)    | No           | 是   | NULL |      | 调用者       |
| succ_cnt | int(11)    | No           |      | NULL |      | 成功次数     |
| err_cnt  | int(11)    | No           |      | NULL |      | 失败次数     |
| ts       | bigint(20) | No           |      | NULL |      | 记录时间     |
| overload | char(1)    | No           |      | NULL |      | 是否过载     |

## 3.相关类

### 3.1.ReportService

基于muduo的one loop per thread TCP服务器模型+线程池，接收客户端的请求任务，存储到数据库中

### 3.2.StoreReport

完成数据库的连接，提供出数据库的存储接口。