#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
using namespace std;

// Class that represents a simple thread pool
class SearcherThreads {
public:
    // // Constructor to creates a thread pool with given
    // number of threads
    SearcherThreads(size_t num_threads
               = thread::hardware_concurrency())
    {
        if (num_threads < 1) {
            fprintf(stderr, "Invalid number of threads, using 1 thread.\n");
            num_threads = 1;
        }

        this->num_threads = num_threads;
        this->threads_.reserve(num_threads);
    }

    // Destructor to stop the thread pool
    ~SearcherThreads()
    {
        {
            // Lock the queue to update the stop flag safely
            unique_lock<mutex> lock(queue_mutex_);
            stop_ = true;
        }

        // Notify all threads
        cv.notify_all();

        // Joining all worker threads to ensure they have
        // completed their tasks
        for (auto& thread : threads_) {
            thread.join();
        }
    }

    // Enqueue task for execution by the thread pool
    void enqueue(function<void()> task)
    {
        threads_.emplace_back([this, task]() {
            {
                unique_lock<mutex> lock(queue_mutex_);
                thread_busy <<= 1;
                thread_busy |= 1;
            }

            task();

            {
                unique_lock<mutex> lock(queue_mutex_);
                thread_busy >>= 1;
                if (thread_busy == 0) {
                    cv.notify_all();
                }
            }
        });
    }

    // Check if the pool is busy
    bool busy()
    {
        bool busy = false;
        {
            unique_lock<mutex> lock(queue_mutex_);
            busy = thread_busy != 0;
        }
        return busy;
    }

    // Wait for all tasks to finish
    void wait()
    {
        unique_lock<mutex> lock(queue_mutex_);
        cv.wait(lock, [this] {
            return thread_busy == 0;
        });
    }

    // Stop the thread pool
    void stop()
    {
        {
            unique_lock<mutex> lock(queue_mutex_);
            stop_ = true;
        }
        
        cv.notify_all();

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

    // Mutex to synchronize access to shared data
    mutex queue_mutex_;

    // Condition variable to signal changes in the state of
    // the tasks queue
    condition_variable cv;

    // Flag to indicate whether the thread pool should stop
    // or not
    bool stop_ = false;
};