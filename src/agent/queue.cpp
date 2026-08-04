#include "nizaw/agent/queue.hpp"

#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>

#ifdef __linux__
#include <sqlite3.h>
#endif

namespace nizaw::agent {

namespace {

std::string serialize_task(const QueuedTask& task) {
    std::stringstream ss;
    ss << task.task_id << "|"
       << (task.result.success ? "1" : "0") << "|"
       << task.result.exit_code << "|"
       << task.retry_count;
    return ss.str();
}

}  // namespace

TaskQueue::TaskQueue(std::string_view db_path) : db_path_(db_path) {
}

TaskQueue::~TaskQueue() = default;

Result<void> TaskQueue::enqueue(const TaskResult& result) {
    std::unique_lock lock(mutex_);

    QueuedTask task;
    task.task_id = result.task_id;
    task.result = result;
    task.queued_at = std::chrono::system_clock::now();
    task.retry_count = 0;

    queue_.push(std::move(task));
    dirty_ = true;

    cv_.notify_one();
    return {};
}

Result<std::optional<TaskResult>> TaskQueue::dequeue() {
    std::unique_lock lock(mutex_);

    if (queue_.empty()) {
        return std::optional<TaskResult>{std::nullopt};
    }

    auto task = std::move(queue_.front());
    queue_.pop();

    lock.unlock();

    return std::optional<TaskResult>{std::move(task.result)};
}

Result<void> TaskQueue::mark_sent(const std::string& task_id) {
    std::unique_lock lock(mutex_);

    std::queue<QueuedTask> new_queue;
    while (!queue_.empty()) {
        if (queue_.front().task_id != task_id) {
            new_queue.push(std::move(queue_.front()));
        }
        queue_.pop();
    }
    queue_ = std::move(new_queue);
    dirty_ = true;

    return {};
}

Result<void> TaskQueue::increment_retry(const std::string& task_id) {
    std::unique_lock lock(mutex_);

    std::queue<QueuedTask> new_queue;
    while (!queue_.empty()) {
        auto task = std::move(queue_.front());
        queue_.pop();
        if (task.task_id == task_id) {
            task.retry_count++;
            new_queue.push(std::move(task));
        } else {
            new_queue.push(std::move(task));
        }
    }
    queue_ = std::move(new_queue);
    dirty_ = true;

    return {};
}

std::size_t TaskQueue::pending_count() const {
    std::unique_lock lock(mutex_);
    return queue_.size();
}

Result<void> TaskQueue::clear() {
    std::unique_lock lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
    dirty_ = true;
    return {};
}

Result<void> TaskQueue::persist() {
#ifdef __linux__
    std::unique_lock lock(mutex_);

    if (!dirty_) {
        return {};
    }

    auto rc = init_database();
    if (!rc) {
        return rc;
    }

    rc = create_tables();
    if (!rc) {
        return rc;
    }

    // Rebuild queue from database
    queue_ = {};
    dirty_ = false;

    return {};
#else
    return Error(ErrorCode::Unsupported,
                 "TaskQueue persistence not supported on this platform",
                 "persist");
#endif
}

Result<void> TaskQueue::load() {
#ifdef __linux__
    std::unique_lock lock(mutex_);

    auto rc = init_database();
    if (!rc) {
        return rc;
    }

    rc = create_tables();
    if (!rc) {
        return rc;
    }

    // TODO: Load tasks from database into queue_

    return {};
#else
    return Error(ErrorCode::Unsupported,
                 "TaskQueue persistence not supported on this platform",
                 "load");
#endif
}

#ifdef __linux__
Result<void> TaskQueue::init_database() {
    sqlite3* db = nullptr;
    int rc = sqlite3_open(db_path_.c_str(), &db);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return Error(ErrorCode::IoError,
                     "Failed to open SQLite database: " + std::string(sqlite3_errmsg(db)),
                     "init_database");
    }

    // Enable WAL mode for better concurrency
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_close(db);

    return {};
}

Result<void> TaskQueue::create_tables() {
    sqlite3* db = nullptr;
    int rc = sqlite3_open(db_path_.c_str(), &db);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return Error(ErrorCode::IoError,
                     "Failed to open SQLite database",
                     "create_tables");
    }

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS task_queue (
            task_id TEXT PRIMARY KEY,
            result_json TEXT NOT NULL,
            queued_at TEXT NOT NULL,
            retry_count INTEGER DEFAULT 0
        )
    )";

    char* err_msg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string error = err_msg ? err_msg : "unknown error";
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return Error(ErrorCode::IoError,
                     "Failed to create tables: " + error,
                     "create_tables");
    }

    sqlite3_close(db);
    return {};
}

Result<void> TaskQueue::insert_task(const QueuedTask& task) {
    (void)task;
    return {};
}

Result<void> TaskQueue::delete_task(const std::string& task_id) {
    (void)task_id;
    return {};
}

Result<void> TaskQueue::update_retry_count(const std::string& task_id, int count) {
    (void)task_id;
    (void)count;
    return {};
}
#else
Result<void> TaskQueue::init_database() {
    return {};
}

Result<void> TaskQueue::create_tables() {
    return {};
}

Result<void> TaskQueue::insert_task(const QueuedTask& task) {
    (void)task;
    return {};
}

Result<void> TaskQueue::delete_task(const std::string& task_id) {
    (void)task_id;
    return {};
}

Result<void> TaskQueue::update_retry_count(const std::string& task_id, int count) {
    (void)task_id;
    (void)count;
    return {};
}
#endif

}  // namespace nizaw::agent