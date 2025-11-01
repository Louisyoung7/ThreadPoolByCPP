#include "threadpool.h"
using namespace louis::thread;

void func(void* arg) {
    int* num = static_cast<int*>(arg);
    std::cout << "执行任务 tid: " << pthread_self() << ", num: " << *num << std::endl;
    usleep(200000);
}

int main(int argc, char const* argv[]) {
    // 创建线程池
    ThreadPool<int> pool(3, 100);

    // 创建并添加任务
    std::cout << "开始添加任务" << std::endl;
    for (int i = 0; i < 100; ++i) {
        int* num = new int(i + 100);
        pool.addTask(func, num);
    }
    std::cout << "添加任务完毕" << std::endl;

    sleep(5);
    return 0;
}