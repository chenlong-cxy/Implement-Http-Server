#pragma once

#include <iostream>
#include <queue>
#include <pthread.h>
#include "Task.hpp"
#include "Log.hpp"

#define NUM 6

class ThreadPool{
    private:
        std::queue<Task> _task_queue; //任务队列
        int _num;
        bool _stop;
        pthread_mutex_t _mutex;
        pthread_cond_t _cond;
        static ThreadPool* _inst;
    private:
        //构造函数私有
        ThreadPool(int num = NUM)
            :_num(num)
            ,_stop(false)
        {
            pthread_mutex_init(&_mutex, nullptr);
            pthread_cond_init(&_cond, nullptr);
        }
        //拷贝构造函数私有或删除
        ThreadPool(const ThreadPool&)=delete;

        bool IsEmpty()
        {
            return _task_queue.empty();
        }
        bool IsStop()
        {
            return _stop;
        }
        void LockQueue()
        {
            pthread_mutex_lock(&_mutex);
        }
        void UnLockQueue()
        {
            pthread_mutex_unlock(&_mutex);
        }
        void ThreadWait()
        {
            pthread_cond_wait(&_cond, &_mutex);
        }
        void ThreadWakeUp()
        {
            pthread_cond_signal(&_cond);
        }
    public:
        static ThreadPool* GetInstance()
        {
            static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
            if(_inst == nullptr){
                pthread_mutex_lock(&mtx);
                if(_inst == nullptr){
                    _inst = new ThreadPool();
                    _inst->InitThreadPool();
                }
                pthread_mutex_unlock(&mtx);
            }
            return _inst;
        }
        static void* ThreadRoutine(void* arg)
        {
            pthread_detach(pthread_self());
            ThreadPool* tp = (ThreadPool*)arg;
            while(true){
                tp->LockQueue();
                while(tp->IsEmpty()){
                    tp->ThreadWait();
                }
                Task task;
                tp->PopTask(task);
                tp->UnLockQueue();

                task.ProcessOn(); //处理任务
            }
        }
        bool InitThreadPool()
        {
            pthread_t tid;
            for(int i = 0;i < _num;i++){
                if(pthread_create(&tid, nullptr, ThreadRoutine, this) != 0){
                    LOG(FATAL, "create thread pool error!");
                    return false;
                }
            }
            LOG(INFO, "create thread pool success!");
            return true;
        }
        void PushTask(const Task& task)
        {
            LockQueue();
            _task_queue.push(task);
            UnLockQueue();
            ThreadWakeUp();
        }
        void PopTask(Task& task)
        {
            task = _task_queue.front();
            _task_queue.pop();
        }
        ~ThreadPool()
        {
            pthread_mutex_destroy(&_mutex);
            pthread_cond_destroy(&_cond);
        }
};
ThreadPool* ThreadPool::_inst = nullptr;
