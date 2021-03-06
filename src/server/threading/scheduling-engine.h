#ifndef _SCHEDULING_ENGINE_H
#define _SCHEDULING_ENGINE_H

#include "method-request.h"
#include "common.h"
#include "threading/thread.h"
#include "utility/singleton.h"
#include "utility/exception.h"
#include "utility/queues/lock_queue.h"
#include "threading/lock.h"
#include "threading/semaphore.h"
#include <list>

class SchedulingEngine;

NEWEXCEPTION(SchedulingException);

class MethodThread : public Thread
{
    public:
        MethodThread(SchedulingEngine* sched): Thread() { sched_engine = sched; }
        ~MethodThread() {}

        void Execute(void* arg);
        void Setup() {}

    private:
        SchedulingEngine* sched_engine;
};

class SchedulingEngine
{
        friend class Singleton<SchedulingEngine>;
    public:
        SchedulingEngine();
        ~SchedulingEngine();

        int Initialize(uint32 n_thread);
        int Deactivate();
        bool IsActive();
        int Execute(MethodRequest* m_req);

        uint32 Count() const
        {
            return m_threads.size();
        }

        MethodRequest* GetNextMethod();

    private:
        std::list<MethodThread*> m_threads;

        LockQueue<MethodRequest*> q_method;

        Mutex m_mutex;
        Semaphore sem;
        bool b_active;
};

#define s_sched_engine Singleton<SchedulingEngine>::GetInstance()

#endif
