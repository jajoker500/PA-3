#include "pool.h"
#include <mutex>
#include <iostream>

Task::Task() = default;
Task::~Task() = default;

ThreadPool::ThreadPool(int num_threads) {
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(new std::thread(&ThreadPool::run_thread, this));
    }
}

ThreadPool::~ThreadPool() {
    for (std::thread *t: threads) {
        delete t;
    }
    threads.clear();

    for (Task *q: queue) {
        delete q;
    }
    queue.clear();
}

void ThreadPool::SubmitTask(const std::string &name, Task *task) {
    //TODO: Add task to queue, make sure to lock the queue
    mtx.lock();
    if(done) {
        std::cerr << "Cannot added task to queue" << std::endl;
        mtx.unlock();
        return;
    }
    task->name = name;
    queue.push_back(task);
    mtx.unlock();
    std::cout << "Added task: " << name << std::endl;
    wake_up.notify_one();
}

void ThreadPool::run_thread() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        //TODO2: if no tasks left, continue
        while (!done && queue.empty()) {
            wake_up.wait(lock);
        }
        //TODO1: if done and no tasks left, break
        if(done && queue.empty()) { std::cout << "Stopping thread" << std::endl; break;}
        //TODO3: get task from queue, remove it from queue, and run it
        Task *currTask = queue.front();
        queue.erase(queue.begin());
        std::cout << "Started task: " << currTask->name << std::endl;
        lock.unlock();
        currTask->Run();
        std::cout << "Finished task: " << currTask->name << std::endl;
        //TODO4: delete task
        delete currTask;
    }
}

// Remove Task t from queue if it's there
void ThreadPool::remove_task(Task *t) {
    mtx.lock();
    for (auto it = queue.begin(); it != queue.end();) {
        if (*it == t) {
            queue.erase(it);
            mtx.unlock();
            return;
        }
        ++it;
    }
    mtx.unlock();
}

void ThreadPool::Stop() {
    //TODO: Delete threads, but remember to wait for them to finish first
    std::cout << "Called Stop()" << std::endl;
    mtx.lock();
    done = true;
    mtx.unlock();
    wake_up.notify_all();
    while(!threads.empty()) {
        std::thread* currThread = threads.back();
        threads.pop_back();
        currThread->join();
        delete currThread;
    }
}
