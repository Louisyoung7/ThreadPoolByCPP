#pragma once

#include <pthread.h>

#include <queue>

namespace louis {
namespace thread {

// 用using给函数指针起别名（C++11）
using callback = void (*)(void* arg);

// 任务结构体
template <class T>
struct Task {
   public:
    // 无参构造
    Task() {
        process_ = nullptr;
        arg_ = nullptr;
    }
    // 有参构造
    Task(callback process, void* arg) {
        process_ = process;
        arg_ = static_cast<T*>(arg);
    }

    // 显式指定编译器提供默认构造函数（C++11）
    ~Task() = default;

    callback process_;  // 处理函数
    T* arg_;            // 处理函数的参数
};

// 任务队列
template <class T>
class TaskQueue {
   public:
    // 构造析构
    TaskQueue();
    ~TaskQueue();

    // 取出任务
    Task<T> popTask();

    // 添加任务
    void pushTask(const Task<T>& task);

    // 获取队列元素个数
    std::size_t getTasksCount();

   private:
    std::queue<Task<T>> qTasks;  // 底层队列
    pthread_mutex_t mutex;    // 互斥锁
};
}  // namespace thread
}  // namespace louis

#include "taskQueue.inl"