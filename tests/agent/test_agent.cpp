#include "nizaw/agent.hpp"
#include "nizaw/remote/transport.hpp"

#include <iostream>
#include <string>

int run_agent_tests() {
    int failures = 0;
    
    // Test AgentConfig default values
    {
        nizaw::agent::AgentConfig config;
        
        if (config.heartbeat_interval.count() != 60) {
            std::cerr << "AgentConfig default heartbeat_interval failed" << std::endl;
            failures++;
        }
        
        if (config.jitter_percent != 30) {
            std::cerr << "AgentConfig default jitter_percent failed" << std::endl;
            failures++;
        }
        
        if (config.max_parallel_tasks != 4) {
            std::cerr << "AgentConfig default max_parallel_tasks failed" << std::endl;
            failures++;
        }
        
        if (config.temp_dir != "/var/cache/nizaw/") {
            std::cerr << "AgentConfig default temp_dir failed" << std::endl;
            failures++;
        }
        
        if (!config.allow_exec || !config.allow_fetch || !config.allow_push) {
            std::cerr << "AgentConfig default permissions failed" << std::endl;
            failures++;
        }
    }
    
    // Test validation
    {
        nizaw::agent::AgentConfig config;
        config.server_url = "https://example.com";
        config.ca_cert = "/etc/nizaw/ca.crt";
        config.client_cert = "/etc/nizaw/client.crt";
        config.client_key = "/etc/nizaw/client.key";
        
        auto result = nizaw::agent::validate_config(config);
        if (!result) {
            std::cerr << "Valid config failed validation" << std::endl;
            failures++;
        }
        
        config.server_url = "";
        result = nizaw::agent::validate_config(config);
        if (result) {
            std::cerr << "Empty server_url should fail validation" << std::endl;
            failures++;
        }
    }
    
    // Test telemetry collection
    {
        auto telemetry = nizaw::agent::collect_telemetry();
        
        if (telemetry.hostname.empty()) {
            std::cerr << "Telemetry hostname is empty" << std::endl;
            failures++;
        }
        
        if (telemetry.kernel.empty()) {
            std::cerr << "Telemetry kernel is empty" << std::endl;
            failures++;
        }
        
        if (telemetry.arch.empty()) {
            std::cerr << "Telemetry arch is empty" << std::endl;
            failures++;
        }
        
        if (telemetry.agent_version != "3.0.5") {
            std::cerr << "Telemetry agent_version mismatch" << std::endl;
            failures++;
        }
    }
    
    // Test task executor permissions
    {
        nizaw::agent::AgentConfig config;
        config.allow_exec = true;
        config.allow_fetch = false;
        config.allow_push = true;
        config.restricted_paths = {"/etc/shadow", "/root/", "/boot/"};
        
        nizaw::agent::TaskExecutor executor(config);
        
        if (!executor.is_allowed(nizaw::remote::TaskType::ExecCmd)) {
            std::cerr << "ExecCmd should be allowed" << std::endl;
            failures++;
        }
        
        if (executor.is_allowed(nizaw::remote::TaskType::FetchFile)) {
            std::cerr << "FetchFile should not be allowed" << std::endl;
            failures++;
        }
    }
    
    // Test task queue
    {
        nizaw::agent::TaskQueue queue("/tmp/test_nizaw_queue.db");
        
        nizaw::agent::TaskResult result;
        result.task_id = "task-1";
        result.success = true;
        result.output = "test output";
        result.exit_code = 0;
        
        if (!queue.enqueue(result)) {
            std::cerr << "Failed to enqueue task" << std::endl;
            failures++;
        }
        
        if (queue.pending_count() != 1) {
            std::cerr << "Queue count should be 1 after enqueue" << std::endl;
            failures++;
        }
        
        auto dequeued = queue.dequeue();
        if (!dequeued) {
            std::cerr << "Failed to dequeue task" << std::endl;
            failures++;
        } else if (!dequeued.value().has_value()) {
            std::cerr << "Dequeued task is empty" << std::endl;
            failures++;
        } else {
            if (dequeued.value().value().task_id != "task-1") {
                std::cerr << "Dequeued task_id mismatch" << std::endl;
                failures++;
            }
        }
        
        if (queue.pending_count() != 0) {
            std::cerr << "Queue count should be 0 after dequeue" << std::endl;
            failures++;
        }
    }
    
    return failures;
}