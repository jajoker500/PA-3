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
    mtx.lock(); // start by locking while messing with the queue
    if(done) { // if already finished cant add
        std::cerr << "Cannot added task to queue" << std::endl;
        mtx.unlock(); // release lock
        return; // escape
    }
    task->name = name; // update the task name
    queue.push_back(task); // push the task into the queue
    mtx.unlock(); // release the lock
    std::cout << "Added task: " << name << std::endl; // say whats added
    wake_up.notify_one(); // wake up one thread
}

void ThreadPool::run_thread() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx); // need unique lock for wait
        //TODO2: if no tasks left, continue
        while (!done && queue.empty()) { // if not done and queue empty wait for notify
            wake_up.wait(lock); // wait for notify
        }
        //TODO1: if done and no tasks left, break
        if(done && queue.empty()) { std::cout << "Stopping thread" << std::endl; break;} // update done/ break if done and empty
        //TODO3: get task from queue, remove it from queue, and run it
        Task *currTask = queue.front(); // set the current task to the front of the queue
        queue.erase(queue.begin()); //dequeue
        std::cout << "Started task: " << currTask->name << std::endl;
        lock.unlock(); // unlock
        currTask->Run(); // now run it
        std::cout << "Finished task: " << currTask->name << std::endl;
        //TODO4: delete task
        delete currTask; // deletes it
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
    mtx.lock(); // lock to update volatile value
    done = true; // we told to stop so update done
    mtx.unlock(); // unlock
    wake_up.notify_all(); // let every thread know to finish up and stop
    while(!threads.empty()) { // while still threads in queue
        std::thread* currThread = threads.back(); // look at one
        threads.pop_back(); // pop it
        currThread->join(); // wait for it to finish
        delete currThread; // delete it
    }
}
