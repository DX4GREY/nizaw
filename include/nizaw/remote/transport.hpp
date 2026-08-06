#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <chrono>
#include <memory>
#include <variant>

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

    // Transport-specific options
    std::string websocket_path{"/ws"};           // For WebSocket
    std::string mqtt_topic{"nizaw/agent"};        // For MQTT
    std::string mqtt_client_id;                   // For MQTT (auto-generated if empty)
    int mqtt_qos{1};                              // For MQTT (0, 1, or 2)
    std::string zmq_endpoint;                     // For ZeroMQ (e.g., "tcp://orchestrator:5555")
    bool grpc_use_http2{true};                    // For gRPC
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

// Transport type enumeration
enum class TransportType {
    Http,        // HTTPS/1.1 polling (default)
    WebSocket,   // WebSocket full-duplex
    Mqtt,        // MQTT pub/sub
    Grpc,        // gRPC streaming
    Zmq,         // ZeroMQ
    Quic         // QUIC/HTTP3
};

// Abstract transport interface
class Transport {
public:
    virtual ~Transport() = default;

    // Connect and perform handshake
    [[nodiscard]] virtual Result<void> connect() = 0;

    // Send heartbeat with telemetry and poll for tasks
    [[nodiscard]] virtual Result<TaskResponse> heartbeat(const nizaw::agent::TelemetryData& telemetry) = 0;

    // Send task result back to server
    [[nodiscard]] virtual Result<void> send_result(const nizaw::agent::TaskResult& result) = 0;

    // Check connection health
    [[nodiscard]] virtual bool is_connected() const noexcept = 0;

    // Disconnect gracefully
    virtual void disconnect() noexcept = 0;

    // Get transport type
    [[nodiscard]] virtual TransportType type() const noexcept = 0;
};

// Factory function
[[nodiscard]] std::unique_ptr<Transport> create_transport(ServerConfig config, TransportType type = TransportType::Http);

// Convenience functions for HTTP transport (backward compatibility)
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