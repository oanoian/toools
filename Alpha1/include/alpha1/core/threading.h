/**
 * Alpha1 Core Threading System
 * 
 * Thread pool, task scheduling, and synchronization primitives
 * optimized for game development workloads.
 * 
 * @file threading.h
 */

#pragma once

#include "alpha1/core/types.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>
#include <atomic>
#include <vector>

namespace Alpha1::Core {

/**
 * Thread Pool for parallel task execution
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency()) 
        : m_stop(false) {
        
        for (size_t i = 0; i < numThreads; ++i) {
            m_workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(m_queueMutex);
                        m_condition.wait(lock, [this] {
                            return m_stop || !m_tasks.empty();
                        });
                        
                        if (m_stop && m_tasks.empty()) {
                            return;
                        }
                        
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                    
                    task();
                    --m_activeTasks;
                }
            });
        }
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_stop = true;
        }
        m_condition.notify_all();
        
        for (std::thread& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    /**
     * Submit a task to the thread pool
     */
    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args) 
        -> std::future<decltype(f(args...))> {
        
        using ReturnType = decltype(f(args...));
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_tasks.emplace([task]() { (*task)(); });
            ++m_activeTasks;
        }
        
        m_condition.notify_one();
        return result;
    }
    
    /**
     * Get number of active tasks
     */
    size_t ActiveTasks() const {
        return m_activeTasks.load();
    }
    
    /**
     * Wait for all tasks to complete
     */
    void WaitAll() {
        while (m_activeTasks.load() > 0) {
            std::this_thread::yield();
        }
    }
    
    /**
     * Get number of worker threads
     */
    size_t Size() const {
        return m_workers.size();
    }
    
private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    
    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;
    
    std::atomic<bool> m_stop;
    std::atomic<size_t> m_activeTasks{0};
};

/**
 * Spinlock for low-latency synchronization
 */
class Spinlock {
public:
    void lock() {
        while (m_flag.test_and_set(std::memory_order_acquire)) {
            // Spin
        }
    }
    
    void unlock() {
        m_flag.clear(std::memory_order_release);
    }
    
    bool try_lock() {
        return !m_flag.test_and_set(std::memory_order_acquire);
    }
    
private:
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
};

/**
 * Read-Write lock for concurrent read access
 */
class RWLock {
public:
    void ReadLock() {
        m_readCount.fetch_add(1, std::memory_order_acquire);
        m_writeLock.lock();
        m_writeLock.unlock();
    }
    
    void ReadUnlock() {
        m_readCount.fetch_sub(1, std::memory_order_release);
    }
    
    void WriteLock() {
        m_writeLock.lock();
    }
    
    void WriteUnlock() {
        m_writeLock.unlock();
    }
    
private:
    std::atomic<int> m_readCount{0};
    Spinlock m_writeLock;
};

/**
 * Barrier for thread synchronization
 */
class Barrier {
public:
    explicit Barrier(size_t count) 
        : m_count(count), m_waiting(0), m_generation(0) {}
    
    void Wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        size_t gen = m_generation;
        
        if (++m_waiting == m_count) {
            ++m_generation;
            m_waiting = 0;
            m_cond.notify_all();
        } else {
            m_cond.wait(lock, [this, gen] {
                return gen != m_generation;
            });
        }
    }
    
private:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    size_t m_count;
    size_t m_waiting;
    size_t m_generation;
};

/**
 * Thread-safe queue for producer-consumer patterns
 */
template<typename T>
class ConcurrentQueue {
public:
    void Push(const T& item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(item);
        m_cond.notify_one();
    }
    
    bool TryPop(T& item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }
    
    bool WaitAndPop(T& item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] { return !m_queue.empty(); });
        item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }
    
    bool Empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    
    size_t Size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    
private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
    std::queue<T> m_queue;
};

/**
 * Get current thread ID
 */
inline ThreadID GetCurrentThreadID() {
    static thread_local ThreadID id = 0;
    static std::atomic<ThreadID> nextID{1};
    
    if (id == 0) {
        id = nextID.fetch_add(1, std::memory_order_relaxed);
    }
    
    return id;
}

/**
 * Set thread name (platform-specific)
 */
inline void SetThreadName(const char* name) {
#if defined(A1_PLATFORM_WINDOWS)
    // Windows implementation
    #ifdef _MSC_VER
        __pragma(warning(push))
        __pragma(warning(disable: 4996))
    #endif
    SetThreadDescription(GetCurrentThread(), 
        std::multi_byte_to_wide_char(name).c_str());
    #ifdef _MSC_VER
        __pragma(warning(pop))
    #endif
#elif defined(A1_PLATFORM_LINUX)
    pthread_setname_np(pthread_self(), name);
#elif defined(A1_PLATFORM_MACOS)
    pthread_setname_np(name);
#endif
}

} // namespace Alpha1::Core
