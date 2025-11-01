namespace louis {
namespace thread {

// 创建线程池
template <typename T>
ThreadPool<T>::ThreadPool(int min, int max) {
    // 初始化任务队列
    TaskQueue_ = new TaskQueue<T>();

    // 线程相关初始化
    // 存储工作线程ID的数组，要足够存储最大线程个数的空间
    workerThreadIDs_ = new pthread_t[max];
    if (workerThreadIDs_ == nullptr) {
        std::cout << "new workerThreadIDs failed..." << std::endl;
        delete TaskQueue_;
    }
    // 清空工作线程ID，方便后面判断
    std::memset(workerThreadIDs_, 0, sizeof(pthread_t) * max);

    int minThreads_ = min;  // 最小线程数
    int maxThreads_ = max;  // 最大线程数
    int busyCount_ = 0;     // 忙碌线程计数
    int liveCount_ = min;   // 存活线程计数
    int exitCount_ = 0;     // 退出线程计数

    // 创建初始工作线程
    for (int i = 0; i < min; ++i) {
        pthread_create(&workerThreadIDs_[i], nullptr, worker, this);
    }

    // 创建管理者线程
    pthread_create(&managerThreadID_, nullptr, manager, this);

    // 初始化互斥锁
    pthread_mutex_init(&poolMutex_, nullptr);
    // 初始化条件变量
    pthread_cond_init(&notEmpty_, nullptr);  // 队列非空条件变量
}

// 销毁线程池
template <typename T>
ThreadPool<T>::~ThreadPool() {
    // 更改关闭标志
    shutdown_ = 1;

    // 先回收管理者线程
    pthread_join(managerThreadID_, nullptr);

    // 唤醒阻塞的工作线程，让其自杀
    for (int i = 0; i < liveCount_; ++i) {
        pthread_cond_signal(&notEmpty_);
    }

    // 睡眠1秒，让阻塞的工作线程有时间退出
    sleep(1);

    // 释放资源
    if (TaskQueue_) {
        delete TaskQueue_;
        TaskQueue_ = nullptr;
    }

    if (workerThreadIDs_) {
        delete[] workerThreadIDs_;
        workerThreadIDs_ = nullptr;
    }

    pthread_mutex_destroy(&poolMutex_);
    pthread_cond_destroy(&notEmpty_);
}

// 单个线程退出
template <typename T>
void ThreadPool<T>::threadExit() {
    // 获取当前线程的ID
    pthread_t tid = pthread_self();

    // 遍历工作线程ID数组，找到对应的线程ID
    pthread_mutex_lock(&poolMutex_);
    for (int i = 0; i < maxThreads_; ++i) {
        if (workerThreadIDs_[i] == tid) {
            // 将对应的线程ID还原为0
            workerThreadIDs_[i] = 0;
            std::cout << "线程" << tid << "退出" << std::endl;
            break;
        }
    }
    pthread_mutex_unlock(&poolMutex_);
    // 退出当前线程
    pthread_exit(nullptr);
}

// 管理者线程函数
template <typename T>
void* ThreadPool<T>::manager(void* arg) {
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while (!pool->shutdown_) {
        // 每3秒检测一次
        sleep(3);

        // 再次判断是否退出
        if (pool->shutdown_) {
            break;
        }

        std::cout << "开始检测" << std::endl;

        // 取出线程池中任务的数量与当前线程的数量
        pthread_mutex_lock(&pool->poolMutex_);
        int taskNum = pool->TaskQueue_->getTasksCount();
        int threadNum = pool->liveCount_;
        int busyNum = pool->busyCount_;
        pthread_mutex_unlock(&pool->poolMutex_);

        // 增加线程
        // 任务数 > 存活线程数 && 存活线程数 < 最大线程数，增加线程
        if (taskNum > threadNum && threadNum < pool->maxThreads_) {
            // 加锁
            pthread_mutex_lock(&pool->poolMutex_);

            // 一次只添加两个
            int count = 0;
            for (int i = 0; taskNum > threadNum && threadNum < pool->maxThreads_ && count < COUNT; ++i) {
                // 从空闲的线程ID中分配
                if (pool->workerThreadIDs_[i] == 0) {
                    pthread_create(&pool->workerThreadIDs_[i], nullptr, worker, pool);
                    pool->liveCount_++;
                    count++;
                }
            }
            std::cout << "增加线程" << std::endl;
            // 解锁
            pthread_mutex_unlock(&pool->poolMutex_);
        }

        // 减少线程
        // 忙碌线程数 * 2 < 存活线程数 && 存活线程数 > 最小线程数，减少线程
        if (busyNum * 2 < threadNum && threadNum > pool->minThreads_) {
            pthread_mutex_lock(&pool->poolMutex_);
            pool->exitCount_ = COUNT;  // 设置一次性退出的线程数
            pthread_mutex_unlock(&pool->poolMutex_);
            // 唤醒空闲的等待的工作线程，让它们自杀
            for (int i = 0; i < COUNT; ++i) {
                pthread_cond_signal(&pool->notEmpty_);
            }
        }
    }
    pthread_exit(nullptr);
}

// 工作线程函数
template <typename T>
void* ThreadPool<T>::worker(void* arg) {
    ThreadPool* pool = static_cast<ThreadPool*>(arg);

    while (true) {
        // 加锁
        pthread_mutex_lock(&pool->poolMutex_);

        // 队列空了但没关闭
        while (pool->TaskQueue_->getTasksCount() == 0 && !pool->shutdown_) {
            // not_empty条件变量未满足，阻塞并释放锁
            pthread_cond_wait(&pool->notEmpty_, &pool->poolMutex_);
            // 判断是否自杀
            if (pool->exitCount_ > 0) {
                // 退出线程计数减少
                pool->exitCount_--;
                // 再次判断线程数是否大于最小线程数
                if (pool->liveCount_ > pool->minThreads_) {
                    pool->liveCount_--;
                    // 先解锁再退出
                    pthread_mutex_unlock(&pool->poolMutex_);
                    std::cout << "threadExit() called..." << std::endl;
                    pool->threadExit();
                }
            }
        }

        // 队列关闭了，退出当前线程
        if (pool->shutdown_) {
            printf("队列关闭了，工作线程%ld退出\n", pthread_self());
            // 先释放锁
            pthread_mutex_unlock(&pool->poolMutex_);
            pool->threadExit();
            break;
        }

        // 取出任务
        Task<T> task = pool->TaskQueue_->popTask();

        // 解锁
        pthread_mutex_unlock(&pool->poolMutex_);

        // 执行任务
        pthread_mutex_lock(&pool->poolMutex_);
        pool->busyCount_++;  // 忙碌线程计数增加
        pthread_mutex_unlock(&pool->poolMutex_);
        task.process_(task.arg_);
        // 释放资源
        delete task.arg_;
        task.arg_ = nullptr;
        pthread_mutex_lock(&pool->poolMutex_);
        pool->busyCount_--;  // 忙碌线程计数减少
        pthread_mutex_unlock(&pool->poolMutex_);
        printf("执行任务完成\n");
    }
    return NULL;
}

// 添加任务到线程池
template <typename T>
bool ThreadPool<T>::addTask(callback process, void* arg) {
    // 加锁
    pthread_mutex_lock(&poolMutex_);

    // 检查队列是否已关闭，因为可能在唤醒时队列关闭
    if (shutdown_) {
        pthread_mutex_unlock(&poolMutex_);
        return false;
    }

    // 添加任务到rear指向的位置，然后移动
    Task<T> task(process,arg);
    TaskQueue_->pushTask(task);

    // 唤醒等待的工作线程
    pthread_cond_signal(&notEmpty_);

    // 解锁
    pthread_mutex_unlock(&poolMutex_);

    return true;
}

// 获取线程池状态
template <typename T>
int ThreadPool<T>::getBusyCount() {
    pthread_mutex_lock(&poolMutex_);
    int busy_count = busyCount_;
    pthread_mutex_unlock(&poolMutex_);
    return busy_count;
}

template <typename T>
int ThreadPool<T>::getAliveCount() {
    pthread_mutex_lock(&poolMutex_);
    int live_count = liveCount_;
    pthread_mutex_unlock(&poolMutex_);
    return live_count;
}

}  // namespace Thread
}  // namespace Louis