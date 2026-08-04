#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <chrono>
#include <memory>

#include "nizaw/result.hpp"
#include "nizaw/agent/agent.hpp"

#ifdef __linux__
#include <openssl/ssl.h>
#endif

namespace nizaw::remote {

struct ServerConfig {
    std::string url;
    std::string ca_cert_path;
    std::string client_cert_path;
    std::string client_key_path;
    std::string server_fingerprint;
    std::chrono::seconds connect_timeout{10};
    std::chrono::seconds request_timeout{30};
};

enum class TaskType {
    ExecCmd,
    ExecScript,
    FetchFile,
    PushFile,
    SysProbe,
    Sleep,
    Upgrade
};

struct RemoteTask {
    std::string task_id;
    TaskType type;
    std::string payload;  // JSON or base64 depending on type
    std::chrono::seconds timeout{300};
};

struct TaskResponse {
    bool has_task = false;
    RemoteTask task;
};

using TaskHandler = std::function<nizaw::agent::TaskResult(const RemoteTask&)>;

// Transport layer for mTLS communication
class Transport {
public:
    explicit Transport(ServerConfig config);
    ~Transport();

    // Disable copy, enable move
    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;
    Transport(Transport&&) noexcept;
    Transport& operator=(Transport&&) noexcept;

    // Connect and perform handshake
    [[nodiscard]] Result<void> connect();

    // Send heartbeat with telemetry and poll for tasks
    [[nodiscard]] Result<TaskResponse> heartbeat(const nizaw::agent::TelemetryData& telemetry);

    // Send task result back to server
    [[nodiscard]] Result<void> send_result(const nizaw::agent::TaskResult& result);

    // Check connection health
    [[nodiscard]] bool is_connected() const noexcept;

    // Disconnect gracefully
    void disconnect() noexcept;

private:
    struct Impl {
        ServerConfig config;
#ifdef __linux__
        struct SSLContextDeleter {
            void operator()(void* p) const noexcept {
                if (p) SSL_CTX_free(static_cast<SSL_CTX*>(p));
            }
        };
        std::unique_ptr<void, Impl::SSLContextDeleter> ctx{nullptr, Impl::SSLContextDeleter()};
        SSL* ssl = nullptr;
        int sockfd = -1;
        bool connected = false;
#else
        bool connected = false;
#endif
    };
    std::unique_ptr<Impl> impl_;
};

// HTTP/2 client with mTLS
[[nodiscard]] Result<std::string> https_post(
    const ServerConfig& config,
    std::string_view endpoint,
    std::string_view json_payload
);

[[nodiscard]] Result<std::string> https_get(
    const ServerConfig& config,
    std::string_view endpoint
);

// Certificate utilities
[[nodiscard]] Result<std::string> load_cert_fingerprint(std::string_view cert_path);
[[nodiscard]] Result<bool> verify_server_fingerprint(
    std::string_view cert_path,
    std::string_view expected_fingerprint
);

}  // namespace nizaw::remote