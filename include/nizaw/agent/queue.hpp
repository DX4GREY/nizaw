#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

#include "nizaw/result.hpp"
#include "nizaw/agent/agent.hpp"

namespace nizaw::agent {

struct QueuedTask {
    std::string task_id;
    TaskResult result;
    std::chrono::system_clock::time_point queued_at;
    int retry_count = 0;
};

class TaskQueue {
public:
    explicit TaskQueue(std::string_view db_path = "/var/cache/nizaw/tasks_queue.db");
    ~TaskQueue();

    // Disable copy
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;

    // Add a task result to the queue for later sending
    [[nodiscard]] Result<void> enqueue(const TaskResult& result);

    // Try to get the next task result to send
    [[nodiscard]] Result<std::optional<TaskResult>> dequeue();

    // Mark a task as successfully sent (remove from queue)
    [[nodiscard]] Result<void> mark_sent(const std::string& task_id);

    // Increase retry count for a task
    [[nodiscard]] Result<void> increment_retry(const std::string& task_id);

    // Get number of pending tasks
    [[nodiscard]] std::size_t pending_count() const;

    // Clear all pending tasks
    [[nodiscard]] Result<void> clear();

    // Persist queue to SQLite
    [[nodiscard]] Result<void> persist();

    // Load queue from SQLite
    [[nodiscard]] Result<void> load();

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<QueuedTask> queue_;
    std::string db_path_;
    std::atomic<bool> dirty_{false};

    [[nodiscard]] Result<void> init_database();
    [[nodiscard]] Result<void> create_tables();
    [[nodiscard]] Result<void> insert_task(const QueuedTask& task);
    [[nodiscard]] Result<void> delete_task(const std::string& task_id);
    [[nodiscard]] Result<void> update_retry_count(const std::string& task_id, int count);
};

}  // namespace nizaw::agent