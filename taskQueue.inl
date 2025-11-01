namespace louis {
namespace thread {

// 构造析构
template <typename T>
TaskQueue<T>::TaskQueue() {
    pthread_mutex_init(&mutex, nullptr);
}
template <typename T>
TaskQueue<T>::~TaskQueue() {
    pthread_mutex_destroy(&mutex);
}

// 取出任务
template <typename T>
Task<T> TaskQueue<T>::popTask() {
    pthread_mutex_lock(&mutex);
    Task task = qTasks.front();
    qTasks.pop();
    pthread_mutex_unlock(&mutex);
    return task;
}

// 添加任务
template <typename T>
void TaskQueue<T>::pushTask(const Task<T>& task) {
    pthread_mutex_lock(&mutex);
    qTasks.push(task);
    pthread_mutex_unlock(&mutex);
}

// 获取队列元素个数
template <typename T>
std::size_t TaskQueue<T>::getTasksCount() {
    pthread_mutex_lock(&mutex);
    std::size_t size = qTasks.size();
    pthread_mutex_unlock(&mutex);
    return size;
}
}  // namespace Thread
}  // namespace Louis