#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
using namespace std;

// Class that represents a simple thread pool
class ThreadPool {
public:
    // // Constructor to creates a thread pool with given
    // number of threads
    ThreadPool(size_t num_threads
               = thread::hardware_concurrency())
    {
        if (num_threads < 1) {
            fprintf(stderr, "Invalid number of threads, using 1 thread.\n");
            num_threads = 1;
        }

        this->num_threads = num_threads;
        this->threads_.reserve(num_threads);

        // Creating worker threads
        for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) {
            threads_.emplace_back([this, thread_id] {
                while (true) {
                    function<void(int)> task;
                    // The reason for putting the below code
                    // here is to unlock the queue before
                    // executing the task so that other
                    // threads can perform enqueue tasks
                    {
                        // Locking the queue so that data
                        // can be shared safely
                        unique_lock<mutex> lock(
                            queue_mutex_);

                        // Waiting until there is a task to
                        // execute or the pool is stopped
                        cv_in.wait(lock, [this] {
                            return !tasks_.empty() || stop_;
                        });
                        thread_busy ^= (1 << thread_id);

                        // exit the thread in case the pool
                        // is stopped and there are no tasks
                        if (stop_ && tasks_.empty()) {
                            thread_busy ^= (1 << thread_id);
                            return;
                        }

                        // Get the next task from the queue
                        task = move(tasks_.front());
                        tasks_.pop();
                    }

                    task(thread_id);

                    // Notify that the task is finished
                    {
                        unique_lock<mutex> lock(queue_mutex_);
                        thread_busy ^= (1 << thread_id);
                        lock.unlock();
                        if (tasks_.empty() && thread_busy == 0) {
                            cv_out.notify_all();
                        }
                    }
                }
            });
        }
    }

    // Destructor to stop the thread pool
    ~ThreadPool()
    {
        {
            // Lock the queue to update the stop flag safely
            unique_lock<mutex> lock(queue_mutex_);
            stop_ = true;
        }

        // Notify all threads
        cv_in.notify_all();

        // Joining all worker threads to ensure they have
        // completed their tasks
        for (auto& thread : threads_) {
            thread.join();
        }
    }

    // Enqueue task for execution by the thread pool
    void enqueue(function<void(int)> task)
    {
        {
            unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(move(task));
        }
        cv_in.notify_one();
    }

    // Check if the pool is busy
    bool busy()
    {
        bool busy = false;
        {
            unique_lock<mutex> lock(queue_mutex_);
            busy = !tasks_.empty();
        }
        return busy;
    }

    // Wait for all tasks to finish
    void wait()
    {
        unique_lock<mutex> lock(queue_mutex_);
        cv_out.wait(lock, [this] {
            return tasks_.empty() && thread_busy == 0;
        });
    }

    // Stop the thread pool
    void stop()
    {
        {
            unique_lock<mutex> lock(queue_mutex_);
            stop_ = true;
        }
        cv_in.notify_all();

        for (auto& thread : threads_) {
            thread.join();
        }
    }

    // Wait for all tasks to finish and stop the thread pool
    void wait_and_stop()
    {
        wait();
        stop();
    }

    void lock()
    {
        queue_mutex_.lock();
    }

    void unlock()
    {
        queue_mutex_.unlock();
    }

    // Check if the thread pool is stopped
    bool is_stopped() const
    {
        return stop_;
    }

    // Get the number of threads in the pool
    size_t get_num_threads() const
    {
        return num_threads;
    }

private:
    // Number of threads in the pool
    size_t num_threads = thread::hardware_concurrency();

    // Vector to store worker threads
    vector<thread> threads_;

    // Vector to store the state of each thread
    uint32_t thread_busy = 0;

    // Queue of tasks
    queue<function<void(int)>> tasks_;

    // Mutex to synchronize access to shared data
    mutex queue_mutex_;

    // Condition variable to signal changes in the state of
    // the tasks queue
    condition_variable cv_in;
    condition_variable cv_out;

    // Flag to indicate whether the thread pool should stop
    // or not
    bool stop_ = false;
};