#pragma once

#include <pthread.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "taskQueue.h"

namespace louis {
namespace thread {

// 线程池类
template <class T>
class ThreadPool {
   public:
    // 创建线程池
    ThreadPool(int min, int max);

    // 销毁线程池
    ~ThreadPool();

    // 添加任务到线程池
    bool addTask(callback process, void* arg);

    // 获取线程池状态
    int getBusyCount();

    int getAliveCount();

   private:
    // 管理者线程函数
    static void* manager(void* arg);

    // 工作线程函数
    static void* worker(void* arg);

    // 单个线程退出
    void threadExit();

   private:
    // 任务队列
    TaskQueue<T>* TaskQueue_;

    pthread_t* workerThreadIDs_;  // 工作线程数组
    pthread_t managerThreadID_;   // 管理者线程
    int minThreads_;              // 最小线程数
    int maxThreads_;              // 最大线程数
    int busyCount_;               // 忙碌线程计数
    int liveCount_;               // 存活线程计数
    int exitCount_;               // 退出线程计数
    bool shutdown_ = false;       // 关闭标志（类内初始化C++11）

    pthread_mutex_t poolMutex_;  // 保护线程池的互斥锁
    pthread_cond_t notEmpty_;    // 队列非空条件

    static const int COUNT = 2;  // 每次增加或减少的线程数量
};

}  // namespace Thread
}  // namespace Louis

#include "threadpool.inl"