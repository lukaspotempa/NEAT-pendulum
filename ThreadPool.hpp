#pragma once

#include <functional>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <cstdint>

namespace tp {

struct TaskQueue {
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<uint32_t> m_remaining_tasks = 0;
    std::atomic<bool> m_shutdown = false;

    template<typename TCallback>
    void addTask(TCallback&& callback) {
        {
            std::lock_guard<std::mutex> lock_guard{ m_mutex };
            m_tasks.push(std::forward<TCallback>(callback));
            m_remaining_tasks++;
        }
        m_cv.notify_one();
    }

    bool getTask(std::function<void()>& target_callback) {
        std::unique_lock<std::mutex> lock{ m_mutex };
        
        // Wait until task
        m_cv.wait(lock, [this]() {
            return !m_tasks.empty() || m_shutdown.load();
        });
        
        if (m_shutdown.load() && m_tasks.empty()) {
            return false;
        }
        
        if (!m_tasks.empty()) {
            target_callback = std::move(m_tasks.front());
            m_tasks.pop();
            return true;
        }
        return false;
    }

    void waitForCompletion() const {
        while (m_remaining_tasks > 0) {
            std::this_thread::yield();
        }
    }

    void workDone() {
        m_remaining_tasks--;
    }
    
    void shutdown() {
        m_shutdown = true;
        m_cv.notify_all();
    }
};

struct Worker {
    uint32_t m_id = 0;
    std::thread m_thread;
    std::function<void()> m_task = nullptr;
    std::atomic<bool> m_running = true;
    TaskQueue* m_queue = nullptr;

    Worker() = default;

    Worker(TaskQueue& queue, uint32_t id)
        : m_id{ id }
        , m_queue{ &queue }
    {
        m_thread = std::thread([this]() {
            run();
        });
    }

    Worker(Worker&& other) noexcept
        : m_id{ other.m_id }
        , m_thread{ std::move(other.m_thread) }
        , m_task{ std::move(other.m_task) }
        , m_running{ other.m_running.load() }
        , m_queue{ other.m_queue }
    {
        other.m_running = false;
        other.m_queue = nullptr;
    }

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    Worker& operator=(Worker&&) = delete;

    void run() {
        while (m_running) {
            if (m_queue->getTask(m_task)) {
                m_task();
                m_queue->workDone();
                m_task = nullptr;
            } else if (!m_running) {
                break;
            }
        }
    }

    void stop() {
        m_running = false;
    }
};

struct ThreadPool {
    uint32_t m_thread_count = 0;
    TaskQueue m_queue;
    std::vector<Worker> m_workers;

    explicit ThreadPool(uint32_t thread_count = 0)
        : m_thread_count{ thread_count == 0 ? std::thread::hardware_concurrency() : thread_count }
    {
        if (m_thread_count == 0) m_thread_count = 4; // Fallback
        m_workers.reserve(m_thread_count);
        for (uint32_t i{ m_thread_count }; i--;) {
            m_workers.emplace_back(m_queue, static_cast<uint32_t>(m_workers.size()));
        }
    }

    ~ThreadPool() {
        // Signal shutdown
        m_queue.shutdown();
        
        // Stop all workers
        for (Worker& worker : m_workers) {
            worker.stop();
        }
        
        // Join threads
        for (Worker& worker : m_workers) {
            if (worker.m_thread.joinable()) {
                worker.m_thread.join();
            }
        }
    }


    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template<typename TCallback>
    void addTask(TCallback&& callback) {
        m_queue.addTask(std::forward<TCallback>(callback));
    }

    void waitForCompletion() const {
        m_queue.waitForCompletion();
    }

    template<typename TCallback>
    void dispatch(uint32_t element_count, TCallback&& callback) {
        if (element_count == 0) return;
        
        const uint32_t batch_size = element_count / m_thread_count;
        
        if (batch_size == 0) {
            callback(0, element_count);
            return;
        }
        
        for (uint32_t i{ 0 }; i < m_thread_count; ++i) {
            const uint32_t start = batch_size * i;
            const uint32_t end = start + batch_size;
            addTask([start, end, &callback]() { callback(start, end); });
        }

        // Handle remainder on main thread
        if (batch_size * m_thread_count < element_count) {
            const uint32_t start = batch_size * m_thread_count;
            callback(start, element_count);
        }

        waitForCompletion();
    }

    uint32_t getThreadCount() const { return m_thread_count; }
};

}