#ifndef RWLOCK_H
#define RWLOCK_H

#include <pthread.h>

//RAII封装
class RwLock
{
public:
    RwLock()
    {
        pthread_rwlock_init(&_rwlockid, NULL);
    }
    ~RwLock()
    {
        pthread_rwlock_destroy(&_rwlockid);
    }
    void rlock()
    {
        pthread_rwlock_rdlock(&_rwlockid);
    }
    void wlock()
    {
        pthread_rwlock_wrlock(&_rwlockid);
    }
    void unlock()
    {
        pthread_rwlock_unlock(&_rwlockid);
    }

    pthread_rwlock_t* getPthreadRwLock()
    {
        return &_rwlockid;       
    }
private:
    pthread_rwlock_t _rwlockid;
};

class RwLockReadGuard
{
public:
    RwLockReadGuard(RwLock& rwlock)
        :_rwlock(rwlock)
    {
        _rwlock.rlock();        
    }
    ~RwLockReadGuard()
    {
        _rwlock.unlock();        
    }
private:
    RwLock& _rwlock;
};

class RwLockWriteGuard
{
public:
    RwLockWriteGuard(RwLock& rwlock)
        :_rwlock(rwlock)
    {
        _rwlock.wlock();        
    }
    ~RwLockWriteGuard()
    {
        _rwlock.unlock();        
    }
private:
    RwLock& _rwlock;
};

#endif
