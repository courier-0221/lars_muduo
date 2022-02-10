#ifndef HOSTINFO_H
#define HOSTINFO_H

#include "Timestamp.h"
#include <stdint.h>

//经过若干次获取请求host节点后，试探选择一次overload过载节点
#define PROBE_NUM           (10)
//初始化HostInfo主机信息访问成功的个数，防止刚启动时少量失败就认为过载
#define INIT_SUCC_CNT       (180)
//当idle节点切换至over_load时的初始化失败次数，主要为了累计一定成功次数才能切换会idle
#define INIT_ERR_CNT        (5)
//当idle节点失败率高于此值，节点变overload状态
#define ERR_RATE            (0.1)
//当overload节点成功率高于此值，节点变成idle状态
#define SUCC_RATE           (0.5)
//当idle节点连续失败次数超过此值，节点变成overload状态
#define CONTIN_ERR_LIMIT    (15)
//当overload节点连续成功次数超过此值, 节点变成idle状态
#define CONTIN_SUCC_LIMIT   (15)
//整个窗口的真实失败率阈值
#define WINDOW_ERR_RATE     (0.7)
//对于某个modid/cmdid下的某个idle状态的host，需要清理一次负载信息的周期
#define IDLE_TIMEOUT        (15)
//对于某个modid/cmdid/下的某个overload状态的host，在过载队列等待的最大时间
#define OVERLOAD_TIMEOUT    (15)
//对于每个NEW状态的modid/cmdid，多久更新一下本地路由,秒
#define UPDATE_TIMEOUT      (15)

//被代理的主机基本信息

class HostInfo
{
public:
    HostInfo(uint32_t ip, uint32_t port, uint32_t vsucc);
    ~HostInfo() {}
    void setIdle();
    void setOverload();
    bool checkWindow();     //计算整个窗口内的真实失败率，如果达到连续失败窗口值则返回true，代表需要调整状态

public:
    uint32_t _ip;           //host被代理主机IP
    uint32_t _port;         //host被代理主机端口
    uint32_t _vsucc;        //虚拟成功次数(API反馈)，用于过载(overload)，空闲(idle)判定
    uint32_t _verr;         //虚拟失败个数(API反馈)，用于过载(overload)，空闲(idle)判定
    uint32_t _rsucc;        //真实成功个数, 给Reporter上报用户观察
    uint32_t _rerr;         //真实失败个数，给Reporter上报用户观察
    uint32_t _continSucc;   //连续成功次数
    uint32_t _continErr;    //连续失败次数
    bool _overload;         //是否过载
    Timestamp _idlePts;     //当前节点成为idle状态的时刻 
    Timestamp _overloadPts; //当节点成为overload状态的时刻 
};

#endif

