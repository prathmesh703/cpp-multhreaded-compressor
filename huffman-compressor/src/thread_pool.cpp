#include "thread_pool.hpp"

ThreadPool::ThreadPool(std::size_t numThreads) : stop(false), completedTasks(0) {
    for (std::size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            workerLoop();
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(mtx);
        stop = true;
    }
    cv.notify_all();

    for (std::thread& w : workers) {
        if (w.joinable()) {
            w.join();
        }
    }
}

void ThreadPool::submit(void (*func)(void*), void* arg) {
    {
        std::unique_lock<std::mutex> lock(mtx);
        tasks.push({func, arg});
    }
    cv.notify_one();
}

void ThreadPool::waitAll(std::size_t totalTasks) {
    std::unique_lock<std::mutex> lock(mtx);
    cvWait.wait(lock, [this, totalTasks] {
        return completedTasks >= totalTasks;
    });
}

void ThreadPool::notifyTaskDone() {
    {
        std::unique_lock<std::mutex> lock(mtx);
        completedTasks++;
    }
    cvWait.notify_one();
}

unsigned int ThreadPool::getHardwareConcurrency() {
    unsigned int count = std::thread::hardware_concurrency();
    return count > 0 ? count : 4;
}

void ThreadPool::workerLoop() {
    while (true) {
        Task task = {nullptr, nullptr};
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {
                return stop || !tasks.empty();
            });

            if (stop && tasks.empty()) {
                return;
            }

            task = tasks.front();
            tasks.pop();
        }

        if (task.func) {
            task.func(task.arg);
        }
        notifyTaskDone();
    }
}
