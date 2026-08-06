# Changelog

## 3.0.5 (current)

### Added
- **Pluggable Transport Layer (`nizaw::remote`)**:
  - Abstract `Transport` interface with pluggable backends
  - `TransportType` enum: Http, WebSocket, Mqtt, Grpc, Zmq, Quic
  - `create_transport()` factory function
  - Transport-specific options in `ServerConfig` (websocket_path, mqtt_topic, mqtt_qos, zmq_endpoint, grpc_use_http2)
  - Backward compatibility functions: `https_post()`, `https_get()`
- **NIZAW_PORTABLE Build Profile**:
  - CMake option `NIZAW_PORTABLE` (default OFF) for self-contained releases
  - Static linking of OpenSSL (libssl.a, libcrypto.a), Zlib (libz.a), SQLite3 (libsqlite3.a), systemd (libsystemd.a)
  - Verification function `verify_static_library()` with clear error messages if static archives missing
  - Release optimizations: `-ffunction-sections`, `-fdata-sections`, `--gc-sections`, optional LTO/IPO
  - Built-in verification target `verify-portable` using ldd/readelf/objdump
  - Script `scripts/verify-portable.sh` for automated forbidden dependency detection
- **Transport CMake Options**:
  - `NIZAW_TRANSPORT_WEBSOCKET=OFF` - Enable WebSocket transport
  - `NIZAW_TRANSPORT_MQTT=OFF` - Enable MQTT transport
  - `NIZAW_TRANSPORT_GRPC=OFF` - Enable gRPC transport
  - `NIZAW_TRANSPORT_ZMQ=OFF` - Enable ZeroMQ transport
  - `NIZAW_TRANSPORT_QUIC=OFF` - Enable QUIC/HTTP3 transport
- **Documentation**:
  - `docs/c2-transport-alternatives.md` - Comprehensive transport comparison and implementation strategy
  - Updated architecture docs with transport evolution plan
  - Updated wiki pages (Home, Development, Installation, Architecture) with portable build and transport options

### Changed
- `src/remote/transport.cpp` refactored from single `HttpTransport` to abstract transport layer with multiple backends
- `include/nizaw/remote/transport.hpp` now defines `Transport` as pure virtual interface
- All internal Nizaw libraries (core, system, process, filesystem, storage, network, service, security, plugin, remote, agent) built as static by default
- Portable build disables PIC for internal libraries
- systemd support remains optional with `unsupported.cpp` fallback when unavailable

### Fixed
- Version consistency across all project files (CMakeLists.txt, headers, source, docs, packaging)
- Test plugin fixture intentionally remains SHARED to test dynamic loading mechanism
- Portable mode correctly handles transitive dependencies without introducing shared-library dependencies

## 3.0.0 (released)

### Added
- **Agent Module (`nizaw::agent`)**:
  - `AgentConfig` struct with id, server_url, TLS cert paths, heartbeat interval, jitter, max parallel tasks, temp dir, and security permissions
  - `TelemetryData` struct with hostname, kernel, arch, uptime, loadavg, IP address, agent version
  - `TaskResult` struct with task_id, success, output, exit_code, error_message
  - Agent lifecycle: `start_daemon()`, `stop_daemon()`, `is_running()`, `get_pid()`
  - Configuration: `load_config()` (TOML parsing), `validate_config()`
  - Telemetry: `collect_telemetry()`
  - `TaskExecutor` class with exec_cmd, exec_script, fetch_file, push_file, sys_probe, sleep, upgrade task types
  - `TaskQueue` class with SQLite-backed persistence, enqueue/dequeue, retry support
- **Remote Module (`nizaw::remote`)**:
  - `ServerConfig` struct with URL, TLS cert paths, server fingerprint, timeouts
  - `TaskType` enum: ExecCmd, ExecScript, FetchFile, PushFile, SysProbe, Sleep, Upgrade
  - `RemoteTask` and `TaskResponse` structs
  - `Transport` class with mTLS connection, heartbeat, send_result, disconnect
  - `https_post()` and `https_get()` HTTP/2 client functions
  - Certificate utilities: `load_cert_fingerprint()`, `verify_server_fingerprint()`
- **CLI Agent Commands**:
  - `agent start [--config <PATH>]` - start agent daemon
  - `agent stop` - stop agent daemon
  - `agent status` - check agent status (with `--json` support)
  - `agent config [--validate] [--config <PATH>]` - display/validate agent config
- **CMake**: `NIZAW_BUILD_AGENT` option (default ON) to build agent/remote modules
- **Tests**: `tests/agent/test_agent.cpp` with AgentConfig defaults, validation, telemetry, executor permissions, and task queue tests
- **Docker**: `Dockerfile.agent` and `docker-compose.yml` for multi-agent simulation
- **Documentation**: `docs/agent-architecture.md` for agent architecture and configuration

### Changed
- `CMakeLists.txt` now links `nizaw::agent` and `nizaw::remote` to CLI and test targets
- `src/cli/cli.cpp` includes agent commands when `NIZAW_BUILD_AGENT` is defined
- `tests/support/main.cpp` includes `run_agent_tests()`
- `include/nizaw/agent/agent.hpp` declares `collect_telemetry()`
- `src/remote/transport.cpp` rewritten to fix structural issues with `Transport::Impl` and SSL context management
- CLI argument parsing for `agent config` and `agent start` now handles flags in any order

### Fixed
- `collect_telemetry()` was defined in `agent.cpp` but not declared in `agent.hpp`
- `src/remote/transport.cpp` had conflicting `Transport::Impl` definitions, incompatible SSL context types, and invalid `Result<std::string>` returns
- `tests/agent/test_agent.cpp` had incorrect `dequeue()` result access and missing `TaskType` include
- `CMakeLists.txt` was missing `nizaw::agent`/`nizaw::remote` link for test target and `NIZAW_BUILD_AGENT` compile definition for CLI
- `find_library(ZLIB_LIBRARY zlib)` failed to find zlib; replaced with `find_package(ZLIB REQUIRED)` + `ZLIB::ZLIB`

## 2.1.0 (released)

### Added
- Network interface statistics: `rx_bytes`, `tx_bytes`, `rx_packets`, `tx_packets`, `rx_errors`, `tx_errors`, `rx_dropped`, `tx_dropped` in `InterfaceInfo`
- Process resource limits: `process::resource_limits(pid)` returning `ResourceLimits` struct with all RLIMIT_* values
- Process I/O statistics: `process::io_stats(pid)` returning `IoStats` with read/write bytes, syscalls, cancelled writes
- Storage filesystem enumeration: `storage::filesystems()` returning list of mounted filesystems with type detection
- `FilesystemType` enum with 30+ filesystem types (ext4, xfs, btrfs, tmpfs, nfs, cifs, overlay, etc.)
- `FilesystemInfo` struct with device, mount_point, fs_type, type, and options

### Changed
- Network `list()` and `inspect()` now populate statistics fields from `/sys/class/net/*/statistics`

## 2.0.1 (released)

### Added
- Network connections listing: `network::connections()` to enumerate TCP/UDP connections
- Storage I/O statistics: `storage::iostat()` for per-device I/O metrics
- Hardware monitoring: `system::hwmon()` for temperature, fan speed, voltage, power
- Process environment inspection: `process::environment(pid)` to read process environment variables
- Network connection info in CLI: `network connections` command

### Changed
- Version bumped from 2.0.0 to 2.0.1
- All documentation updated to reflect v2.0.1 features
- API design documentation synchronized with current headers

### Fixed
- Minor documentation inconsistencies across wiki and main docs

## 2.0.0 (released)

### Added
- **Write Operations Infrastructure**:
  - `WriteOptions` struct with dry-run, force, recursive, timeout, and confirmation prompt support
  - `CapabilitySet` class for Linux capability checking (CAP_SYS_ADMIN, CAP_NET_ADMIN, etc.)
  - `AuditLogger` for structured logging of all mutating operations
- **Filesystem Write Operations**:
  - `create_directory()` - create directories with recursive support
  - `remove()` - remove files/directories
  - `rename()` - move/rename files and directories
  - `copy()` - copy files and directories
  - `set_permissions()` - change file permissions (chmod)
  - `set_owner()` - change file owner/group (chown)
  - `create_symlink()` - create symbolic links
  - `write_file()` - write content to files
  - `read_file()` - read file contents
  - `truncate()` - truncate files to specific size
- **Process Write Operations**:
  - `send_signal()` - send arbitrary signals to processes
  - `terminate()` - gracefully terminate processes (SIGTERM/SIGKILL)
  - `suspend()` - suspend processes (SIGSTOP)
  - `resume()` - resume suspended processes (SIGCONT)
  - `set_nice()` - adjust process priority
- **Service Write Operations**:
  - `control()` - start/stop/restart/reload systemd services
  - `enable()` - enable services at boot
  - `disable()` - disable services at boot
- **Network Module Enhancements**:
  - `ConnectionInfo` struct with protocol, addresses, ports, UID, inode
  - Network connection enumeration via `/proc/net/tcp` and `/proc/net/tcp6`
- **Storage Module Enhancements**:
  - `IoStats` struct with read/write operations, sectors, bytes, timing
  - I/O statistics via `/sys/block/*/stat`
- **System Module Enhancements**:
  - `HwmonData` struct for hardware monitoring
  - Hardware sensor reading from `/sys/class/hwmon`
- **New Error Codes**: CapabilityRequired, OperationNotPermitted, ResourceBusy, WouldBlock, InvalidState, PartialFailure, ConfirmationRequired
- **CLI Enhancements**:
  - `--yes` / `-y` flag to skip confirmation prompts
  - Complete write command support for all modules
  - `system hwmon`, `network connections`, `storage iostat` commands
  - Process environment command: `process environment <PID>`
- **Documentation**: Comprehensive usage examples for write operations in docs/wiki/Usage.md
- **README Updates**: Complete feature list and CLI command reference

### Changed
- Updated all module headers to include write operation declarations
- Enhanced error handling with new error codes for write operations
- Updated API design documentation to reflect new write APIs
- Updated architecture documentation to include write operations in scope
- CLI command tree expanded from ~20 to ~50 commands

### Fixed
- Compilation warnings and linking errors for new write operation implementations
