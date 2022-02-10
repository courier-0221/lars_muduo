# AgentServer功能简介

功能1.最佳节点获取服务

​    client需要获取modid+cmdid下一个可用的主机节点（向DnsServer发请求），LoadBalance经过内部计算返回业务一个认为可以的主机节点（module下可以有多个主机节点，具体返回哪个loadbalance内部计算会有结果）。

功能2.所有节点获取服务

​	client获取modid+cmdid下的所有主机节点向server发送请求进而通过dnsclient转发到dnsservice处获得。

功能3.节点调用结果上报服务

​    client获取到节点后，进一步判断节点是否可以正常使用，然后将结果发送回来，进而LoadBalance需要记录下当前module下的哪个主机节点被使用了以便LoadBalance计算使用。



总结：

​	整合两部分功能（DNS+Report），然后增加一个给出主机的计算方式



# 实现

## 1.思路

整体框图：

![](./img/系统框图(类视角).png)



负载均衡框图：

计算相关

理论基础

每个模块modid/cmdid下有若干节点，节点的集合称为此模块的路由； 对于每个节点，有两种状态：
idle：此节点可用；
overload：此节点过载，暂时不可用。



## 2.相关类

### 2.1.HostInfo

​    代表一个module（modid+cmdid）下的一个host信息（ip+port），并且有很多属性信息来记录该节点在负载均衡时的信息，用于统计节点的使用情况进而判断是过载还是空闲状态。

### 2.2.LoadBalance

​    一个LoadBalance对应一个module，对该module下的所有hosts信息做统计，何时处于idle何时处于overload，内部维护两个链表，一个空闲链表一个过载链表，agentclient向agentserver请求最佳host后进行判定该host是否可用然后将是否可用的结果反馈给agentserver，agentserver内部对client的反馈做统计，判定该节点过载还是空闲等一系列动作。

### 2.3.Manage

​    一个manage管理所有的module及其对应的loadbalance。

### 2.4.DnsClient

​    DnsService的客户端，使用简化版本muduo实现，采用 one loop per thread TCP服务器模型，主要提供向DnsService获取mod下所有节点的请求。agentclient向agentserver发送数据请求，agentserver到后通知dnsclient向service发送获取所有节点的请求，回dnsclient应答后将数据交给agentserver处理，另外一种接收到service发送的数据情况为数据库中mod发生改变主动推送，同样将数据交给agentserver。实现为在AgentServer中创建一个线程该线程中运行该服务。

### 2.5.RepoterClient

​    RepoterService的客户端，使用简化版本muduo实现，采用 one loop per thread TCP服务器模型，主要提供向ReportService上报节点调用结果的请求。agentclient向agentserver发送数据请求，agentserver接收到后通知reportclient向service发送上报数据请求，并且不需要有应答。实现为在AgentServer中创建一个线程该线程中运行该服务。

### 2.6.AgentServer

​    该模块整合Manage、DnsClient、和RepoterClient，当agentclient向agentserver请求一个module的最佳主机时通过manage里面module对应的hosts内容做计算返回一个最佳的，客户端收到后进一步判断是否可以使用，然后给出使用状态通过agentserver在进一步通过reportclient将请求发送出去。











