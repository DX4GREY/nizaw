#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>

#include "nizaw/result.hpp"
#include "nizaw/agent/agent.hpp"

namespace nizaw::remote {
    enum class TaskType;
    struct RemoteTask;
}

namespace nizaw::agent {

class TaskExecutor {
public:
    explicit TaskExecutor(const AgentConfig& config);
    ~TaskExecutor();

    // Disable copy
    TaskExecutor(const TaskExecutor&) = delete;
    TaskExecutor& operator=(const TaskExecutor&) = delete;

    // Execute a task and return result
    [[nodiscard]] TaskResult execute(const nizaw::remote::RemoteTask& task);

    // Check if a task type is allowed by current configuration
    [[nodiscard]] bool is_allowed(nizaw::remote::TaskType type) const;

    // Register custom task handler
    using CustomHandler = std::function<TaskResult(const nizaw::remote::RemoteTask&)>;
    void register_handler(nizaw::remote::TaskType type, CustomHandler handler);

private:
    // Task implementations
    [[nodiscard]] TaskResult exec_cmd(const nizaw::remote::RemoteTask& task);
    [[nodiscard]] TaskResult exec_script(const nizaw::remote::RemoteTask& task);
    [[nodiscard]] TaskResult fetch_file(const nizaw::remote::RemoteTask& task);
    [[nodiscard]] TaskResult push_file(const nizaw::remote::RemoteTask& task);
    [[nodiscard]] TaskResult sys_probe(const nizaw::remote::RemoteTask& task);
    [[nodiscard]] TaskResult sleep_task(const nizaw::remote::RemoteTask& task);
    [[nodiscard]] TaskResult upgrade(const nizaw::remote::RemoteTask& task);

    // Helpers
    [[nodiscard]] bool is_path_restricted(std::string_view path) const;
    [[nodiscard]] std::string compute_sha256(std::string_view file_path);
    [[nodiscard]] TaskResult system_info_probe();
    [[nodiscard]] TaskResult process_list_probe();

    AgentConfig config_;
    std::vector<std::pair<nizaw::remote::TaskType, CustomHandler>> custom_handlers_;
};

}  // namespace nizaw::agent