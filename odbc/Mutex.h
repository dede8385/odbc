#ifndef __ODBC_MUTEX_H__
#define __ODBC_MUTEX_H__

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

namespace ODBC
{

class MutexLock : boost::noncopyable
{
public:
    MutexLock()
    {
        MCHECK(pthread_mutex_init(&mutex_, NULL));
    }

    ~MutexLock()
    {
        MCHECK(pthread_mutex_destroy(&mutex_));
    }

    void lock()
    {
        MCHECK(pthread_mutex_lock(&mutex_));
    }

    void unlock()
    {
        MCHECK(pthread_mutex_unlock(&mutex_));
    }

    pthread_mutex_t* getPthreadMutex()
    {
        return &mutex_;
    }
    
private:
    pthread_mutex_t mutex_;
};

class MutexLockGuard : boost::noncopyable
{
public:
    explicit MutexLockGuard(MutexLock& mutex) : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~MutexLockGuard()
    {
        mutex_.unlock();
    }

private:
    MutexLock& mutex_;
};

} // namespace ODBC

// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name"

#endif  // __ODBC_MUTEX_H__
