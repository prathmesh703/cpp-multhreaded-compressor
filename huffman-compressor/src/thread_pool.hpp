#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstddef>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t numThreads);
    ~ThreadPool();

    // Submit a task (function pointer + argument pointer)
    void submit(void (*func)(void*), void* arg);

    // Block until all totalTasks are completed
    void waitAll(std::size_t totalTasks);

    // Signal that one task has finished
    void notifyTaskDone();

    // Query hardware core count
    static unsigned int getHardwareConcurrency();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

private:
    struct Task {
        void (*func)(void*);
        void* arg;
    };

    void workerLoop();

    std::vector<std::thread> workers;
    std::queue<Task> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    std::condition_variable cvWait;
    bool stop;
    std::size_t completedTasks;
};
