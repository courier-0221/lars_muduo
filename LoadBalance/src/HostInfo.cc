#include "HostInfo.h"

HostInfo::HostInfo(uint32_t ip, uint32_t port, uint32_t vsucc)
    : _ip(ip)
    , _port(port)
    , _vsucc(vsucc)
    , _verr(0)
    , _rsucc(0)
    , _rerr(0)
    , _continSucc(0)
    , _continErr(0)
    , _overload(false)
{
}

void HostInfo::setIdle()
{
    _vsucc = INIT_SUCC_CNT;
    _verr = 0;
    _rsucc = 0;
    _rerr = 0;
    _continSucc = 0;
    _continErr = 0;
    _overload = false;
    _idlePts = Timestamp(Timestamp::now());//设置判定为idle状态的时刻,也是重置窗口时间
}

void HostInfo::setOverload()
{
    _vsucc = 0;
    _verr = INIT_ERR_CNT;//overload的初试虚拟err错误次数
    _rsucc = 0;
    _rerr = 0;
    _continErr = 0;
    _continSucc = 0;
    _overload = true;
    _overloadPts = Timestamp(Timestamp::now());//设置判定为overload的时刻
}

bool HostInfo::checkWindow() 
{
    if (0 == (_rsucc + _rerr)) 
    {
        return false; //分母不能为0
    }

    if ((_rerr * 1.0 / (_rsucc + _rerr)) >= WINDOW_ERR_RATE) 
    {
        return true;
    }
    else
    {
        return false;
    }
}
