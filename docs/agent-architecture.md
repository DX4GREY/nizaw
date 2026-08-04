# Nizaw Agent v3.0 - Distributed Agentic Orchestration & Telemetry Backhaul

## Overview

The Nizaw Agent (nizawd) transforms nizaw from a local CLI inspection tool into a distributed orchestration platform. The agent runs as a background daemon, maintains persistent mTLS connections to a central orchestrator, and executes remote tasks asynchronously.

## Architecture

### Components

```
┌─────────────────────────────────────────────────────┐
│                   Agent Daemon                       │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────┐│
│  │   Agent     │  │   Task       │  │   Task     ││
│  │   Core      │◄─│   Executor   │◄─│   Queue    ││
│  └─────────────┘  └──────────────┘  └────────────┘│
│         │                  │                  │     │
│         └──────────────────┼──────────────────┘     │
│                            │                        │
│                    ┌───────▼───────┐               │
│                    │   Transport   │               │
│                    │   (mTLS)      │               │
│                    └───────┬───────┘               │
└────────────────────────────┼───────────────────────┘
                             │
                       HTTPS/2
                             │
┌────────────────────────────▼───────────────────────┐
│              Central Orchestrator                    │
│  ┌────────────┐  ┌────────────┐  ┌───────────────┐ │
│  │   API      │  │   Task     │  │   Telemetry   │ │
│  │   Gateway  │  │   Scheduler│  │   Collector   │ │
│  └────────────┘  └────────────┘  └───────────────┘ │
└─────────────────────────────────────────────────────┘
```

### Module Structure

```
src/
├── agent/
│   ├── agent.cpp      # Agent lifecycle, config, telemetry
│   ├── executor.cpp   # Task execution engine
│   └── queue.cpp      # SQLite-backed task queue
└── remote/
    └── transport.cpp  # mTLS transport layer

include/nizaw/
├── agent/
│   ├── agent.hpp      # AgentConfig, TelemetryData, TaskResult
│   ├── executor.hpp   # TaskExecutor class
│   └── queue.hpp      # TaskQueue class
├── remote/
│   └── transport.hpp  # Transport, ServerConfig, RemoteTask
└── agent.hpp          # Umbrella header
```

## Configuration

### agent.toml

```toml
[agent]
id = "auto-generate-uuid"  # Auto-generated if not specified
server_url = "https://orchestrator.example.com"
ca_cert = "/etc/nizaw/ca.crt"
client_cert = "/etc/nizaw/client.crt"
client_key = "/etc/nizaw/client.key"

[behavior]
heartbeat_interval_sec = 60    # Base interval (seconds)
jitter_percent = 30            # Random jitter (±30%)
max_parallel_tasks = 4         # Worker thread pool size
temp_dir = "/var/cache/nizaw/" # Temporary file storage

[security]
allow_exec = true              # Allow exec_cmd and exec_script
allow_fetch = true             # Allow fetch_file tasks
allow_push = true              # Allow push_file tasks
restricted_paths = [           # Blocked file paths
    "/etc/shadow",
    "/root/",
    "/boot/"
]
```

## Security Model

### Mutual TLS (mTLS)

- **Certificate-based authentication**: Each agent has a unique client certificate signed by the central CA
- **TLS 1.3 only**: Minimum and maximum protocol version enforced
- **Certificate pinning**: Server certificate fingerprint can be verified to prevent MITM attacks
- **Egress-only connections**: Agent initiates all connections (no inbound ports required)

### Task Restrictions

- Path-based access control for file operations
- Configurable execution permissions (exec, fetch, push)
- Restricted paths prevent access to sensitive system files

### Stealth Features

- Randomized heartbeat jitter (±30% by default)
- Browser-like User-Agent headers
- Process name hiding via `prctl(PR_SET_NAME)`
- Low-privilege operation (non-root when possible)

## Task Types

### 1. exec_cmd
Execute a shell command and return stdout/stderr.

**Payload**: Shell command string  
**Result**: stdout, exit code

### 2. exec_script
Receive base64-encoded script, execute, and cleanup.

**Payload**: Base64-encoded script content  
**Result**: stdout, exit code

### 3. fetch_file
Read a file, compress with zlib, encode base64.

**Payload**: Absolute file path  
**Result**: Base64-encoded compressed file content

### 4. push_file
Receive base64 file, write to path with backup.

**Payload**: `target_path|base64_data`  
**Result**: Success/failure status

### 5. sys_probe
Run system information collection.

**Payload**: `system_info` or `process_list`  
**Result**: JSON-formatted system data

### 6. sleep
Dynamic heartbeat interval adjustment.

**Payload**: New interval in seconds  
**Result**: Success/failure status

### 7. upgrade
Binary self-upgrade capability.

**Payload**: `download_url|expected_sha256`  
**Result**: Upgrade status

## Communication Protocol

### Heartbeat Flow

```
Agent                              Orchestrator
  │                                     │
  ├─ POST /api/v1/tasks/pull ─────────>│
  │  {telemetry JSON}                   │
  │                                     ├─ Validate mTLS
  │                                     ├─ Check task queue
  │                                     │
  │<───────── 200 OK ──────────────────┤
  │              {task_id, payload}      │
  │                                     │
  ├─[Execute task asynchronously]       │
  │                                     │
  ├─ POST /api/v1/tasks/result ────────>│
  │  {task_id, result, output}          │
  │                                     │
```

### Telemetry Data

```json
{
  "hostname": "agent-01",
  "kernel": "5.15.0-72-generic",
  "arch": "x86_64",
  "uptime": "123456s",
  "loadavg": "0.52 0.68 0.72",
  "ip_address": "192.168.1.100",
  "agent_version": "3.0.0"
}
```

## Build Instructions

### Prerequisites

- CMake 3.20+
- C++20 compiler
- OpenSSL 1.1.1+
- libsqlite3
- zlib

### Build with Agent Support

```bash
# Default: agent enabled
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Disable agent (CLI-only build)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DNIZAW_BUILD_AGENT=OFF
cmake --build build -j$(nproc)
```

### Install

```bash
sudo cmake --install build
```

## CLI Commands

### agent start
Start the agent daemon in background.

```bash
nizaw agent start [--config agent.toml]
```

### agent stop
Stop the running agent daemon.

```bash
nizaw agent stop
```

### agent status
Check agent daemon status.

```bash
nizaw agent status [--json]
```

Output:
```
Agent is running (PID: 12345)
```

JSON:
```json
{
  "running": true,
  "pid": 12345
}
```

### agent config
Display or validate agent configuration.

```bash
# Display configuration
nizaw agent config [--config agent.toml]

# Validate only
nizaw agent config --validate [--config agent.toml]

# JSON output
nizaw agent config --json
```

## Testing

### Unit Tests

```bash
# Build with tests
cmake -B build -DNIZAW_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

### Integration Testing

See `tests/agent/` for integration tests (to be implemented).

### Memory Leak Detection

```bash
# Build with AddressSanitizer
cmake -B build -DNIZAW_ENABLE_ASAN=ON
cmake --build build
./build/nizaw_core_tests
```

## Docker Simulation

A `docker-compose.yml` file is provided for simulating a multi-agent cluster with a mock orchestrator.

```bash
# Start simulation
docker-compose up

# View agent logs
docker-compose logs agent1

# Stop simulation
docker-compose down
```

## API Reference

### nizaw::agent::AgentConfig

Configuration structure for agent initialization.

```cpp
struct AgentConfig {
    std::string id;                    // Agent unique ID
    std::string server_url;            // Orchestrator URL
    std::string ca_cert;               // CA certificate path
    std::string client_cert;           // Client certificate path
    std::string client_key;            // Client private key path
    std::chrono::seconds heartbeat_interval;  // Base heartbeat interval
    int jitter_percent;                // Jitter percentage (0-100)
    int max_parallel_tasks;            // Worker pool size
    std::string temp_dir;              // Temporary directory
    bool allow_exec;                   // Allow command execution
    bool allow_fetch;                  // Allow file fetch
    bool allow_push;                   // Allow file push
    std::vector<std::string> restricted_paths;  // Blocked paths
};
```

### nizaw::agent::TaskResult

Task execution result.

```cpp
struct TaskResult {
    std::string task_id;
    bool success;
    std::string output;
    int exit_code;
    std::string error_message;
};
```

### nizaw::remote::Transport

mTLS transport layer for agent-orchestrator communication.

```cpp
class Transport {
public:
    explicit Transport(ServerConfig config);
    Result<void> connect();
    Result<TaskResponse> heartbeat(const TelemetryData& telemetry);
    Result<void> send_result(const TaskResult& result);
    bool is_connected() const;
    void disconnect();
};
```

## Performance Considerations

- **Connection pooling**: Persistent HTTPS/2 connections minimize handshake overhead
- **Task batching**: Multiple task results can be sent in a single heartbeat
- **Exponential backoff**: Failed deliveries retry with increasing delays
- **SQLite WAL mode**: Write-ahead logging for concurrent queue access
- **Worker threads**: Small thread pool (2-4 workers) prevents resource exhaustion

## Troubleshooting

### Agent won't start

1. Check certificate files exist and are readable
2. Verify server URL is reachable
3. Check `/var/cache/nizaw/` directory permissions
4. Review system logs: `journalctl -u nizawd`

### Connection failures

1. Verify CA certificate is valid
2. Check firewall rules (egress port 443)
3. Test with: `openssl s_client -connect orchestrator:443 -CAfile ca.crt`

### Task execution failures

1. Check agent logs for error messages
2. Verify task permissions in config
3. Ensure restricted paths don't block intended operations
4. Check disk space in temp directory

## License

Same as parent nizaw project.