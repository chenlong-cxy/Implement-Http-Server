#pragma once

#include <iostream>
#include <queue>
#include <pthread.h>
#include "Task.hpp"
#include "Log.hpp"

#define NUM 6

//线程池
class ThreadPool{
    private:
        std::queue<Task> _task_queue; //任务队列
        int _num;                     //线程池中线程的个数
        pthread_mutex_t _mutex;       //互斥锁
        pthread_cond_t _cond;         //条件变量
        static ThreadPool* _inst;     //指向单例对象的static指针
    private:
        //构造函数私有
        ThreadPool(int num = NUM)
            :_num(num)
        {
            //初始化互斥锁和条件变量
            pthread_mutex_init(&_mutex, nullptr);
            pthread_cond_init(&_cond, nullptr);
        }
        //将拷贝构造函数和拷贝赋值函数私有或删除（防拷贝）
        ThreadPool(const ThreadPool&)=delete;
        ThreadPool* operator=(const ThreadPool&)=delete;

        //判断任务队列是否为空
        bool IsEmpty()
        {
            return _task_queue.empty();
        }

        //任务队列加锁
        void LockQueue()
        {
            pthread_mutex_lock(&_mutex);
        }
        
        //任务队列解锁
        void UnLockQueue()
        {
            pthread_mutex_unlock(&_mutex);
        }

        //让线程在条件变量下进行等待
        void ThreadWait()
        {
            pthread_cond_wait(&_cond, &_mutex);
        }
        
        //唤醒在条件变量下等待的一个线程
        void ThreadWakeUp()
        {
            pthread_cond_signal(&_cond);
        }

    public:
        //获取单例对象
        static ThreadPool* GetInstance()
        {
            static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; //定义静态的互斥锁
            //双检查加锁
            if(_inst == nullptr){
                pthread_mutex_lock(&mtx); //加锁
                if(_inst == nullptr){
                    //创建单例线程池对象并初始化
                    _inst = new ThreadPool();
                    _inst->InitThreadPool();
                }
                pthread_mutex_unlock(&mtx); //解锁
            }
            return _inst; //返回单例对象
        }

        //线程的执行例程
        static void* ThreadRoutine(void* arg)
        {
            pthread_detach(pthread_self()); //线程分离
            ThreadPool* tp = (ThreadPool*)arg;
            while(true){
                tp->LockQueue(); //加锁
                while(tp->IsEmpty()){
                    //任务队列为空，线程进行wait
                    tp->ThreadWait();
                }
                Task task;
                tp->PopTask(task); //获取任务
                tp->UnLockQueue(); //解锁

                task.ProcessOn(); //处理任务
            }
        }
        
        //初始化线程池
        bool InitThreadPool()
        {
            //创建线程池中的若干线程
            pthread_t tid;
            for(int i = 0;i < _num;i++){
                if(pthread_create(&tid, nullptr, ThreadRoutine, this) != 0){
                    LOG(FATAL, "create thread pool error!");
                    return false;
                }
            }
            LOG(INFO, "create thread pool success");
            return true;
        }
        
        //将任务放入任务队列
        void PushTask(const Task& task)
        {
            LockQueue();    //加锁
            _task_queue.push(task); //将任务推入任务队列
            UnLockQueue();  //解锁
            ThreadWakeUp(); //唤醒一个线程进行任务处理
        }

        //从任务队列中拿任务
        void PopTask(Task& task)
        {
            //获取任务
            task = _task_queue.front();
            _task_queue.pop();
        }

        ~ThreadPool()
        {
            //释放互斥锁和条件变量
            pthread_mutex_destroy(&_mutex);
            pthread_cond_destroy(&_cond);
        }
};
//单例对象指针初始化为nullptr
ThreadPool* ThreadPool::_inst = nullptr;

