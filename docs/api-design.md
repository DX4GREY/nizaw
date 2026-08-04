# Nizaw — API Design

Status: Stable API design and implementation

## 1. `Result<T>` / `Error` shape

C++20 has no `std::expected`, so `nizaw::core` defines its own:

```cpp
namespace nizaw {

enum class ErrorCode {
    Unknown = 0,
    NotFound,
    PermissionDenied,
    InvalidArgument,
    Unsupported,
    IoError,
    ParseError,
    ResourceUnavailable,
    AlreadyExists,
    CapabilityRequired,
    OperationNotPermitted,
    ResourceBusy,
    WouldBlock,
    InvalidState,
    PartialFailure,
    ConfirmationRequired,
};

class Error {
public:
    Error(ErrorCode code, std::string message, std::string_view source,
          std::optional<int> errno_value = std::nullopt);

    ErrorCode code() const noexcept;
    const std::string& message() const noexcept;
    const std::string& source() const noexcept;      // e.g. "storage", "process"
    std::optional<int> errno_value() const noexcept;

    static Error from_errno(int errno_value, ErrorCode code, std::string_view source,
                            std::optional<std::string> message = std::nullopt);

private:
    ErrorCode code_;
    std::string message_;
    std::string source_;
    std::optional<int> errno_value_;
};

template <typename T>
class Result {
public:
    Result(T value);
    Result(Error error);

    bool has_value() const noexcept;
    explicit operator bool() const noexcept;  // true if holds T
    const T& value() const&;                  // throws std::logic_error if error (programmer bug, not expected-path)
    T& value() &;
    T&& value() &&;
    const Error& error() const&;
    T value_or(U&& fallback) const&;
    const T& operator*() const&;
    T& operator*() &;
    const T* operator->() const;
    T* operator->();

private:
    std::variant<T, Error> storage_;
};

// Specialization for void-returning fallible operations:
template <> class Result<void> { /* holds either monostate or Error */ };

} // namespace nizaw
```

Usage contract: every public function that can fail for a reason a caller
should handle (missing device, permission denied, ESRCH, unsupported
telemetry, malformed `/proc` entry, etc.) returns `Result<T>`. Functions that
cannot fail don't wrap their return type.

## 2. Module namespaces and current signatures

These are the public surfaces of the current release.

### `nizaw::core`

```cpp
namespace nizaw::core {

// --- error handling ---
// (see section 1 above)

// --- logging ---
enum class LogLevel { Trace = 0, Debug, Info, Warn, Error, Fatal, Off };
std::string_view to_string(LogLevel level) noexcept;

class Logger {
public:
    static Logger& instance() noexcept;
    void set_level(LogLevel level) noexcept;
    LogLevel level() const noexcept;
    void write(LogLevel level, std::string_view source, std::string_view message);
};

template <typename... Args>
void log(LogLevel level, std::string_view source, std::format_string<Args...> fmt, Args&&... args);

// Convenience macros: NIZAW_LOG_TRACE, NIZAW_LOG_DEBUG, NIZAW_LOG_INFO,
// NIZAW_LOG_WARN, NIZAW_LOG_ERROR, NIZAW_LOG_FATAL

// --- platform detection ---
struct PlatformInfo {
    std::string distro_id;       // "ubuntu", "debian", "arch", "fedora", "kali"
    std::string distro_name;     // "Ubuntu 24.04.1 LTS"
    std::string distro_version;  // "24.04"
    bool has_systemd = false;
};

PlatformInfo detect() noexcept;

// --- environment helpers ---
namespace env {
    std::optional<std::string> get(std::string_view name) noexcept;
    std::string get_or(std::string_view name, std::string_view fallback) noexcept;
    bool exists(std::string_view name) noexcept;
}

// --- version metadata ---
inline constexpr unsigned kVersionMajor = 2;
inline constexpr unsigned kVersionMinor = 0;
inline constexpr unsigned kVersionPatch = 1;

struct Version { unsigned major; unsigned minor; unsigned patch; };
std::string version_string();
Version version() noexcept;

// --- write operations infrastructure ---
struct WriteOptions {
    bool dry_run = false;
    bool force = false;
    bool recursive = false;
    std::optional<std::chrono::seconds> timeout;
    std::optional<std::string> confirm_prompt;
};

class CapabilitySet {
public:
    [[nodiscard]] bool check(int capability) const noexcept;
    [[nodiscard]] bool has_admin() const noexcept;
    [[nodiscard]] bool has_network_admin() const noexcept;
    [[nodiscard]] bool has_dac_override() const noexcept;
    [[nodiscard]] bool has_setuid() const noexcept;
    [[nodiscard]] bool has_setgid() const noexcept;
    [[nodiscard]] bool has_kill() const noexcept;
    [[nodiscard]] bool has_sys_ptrace() const noexcept;
    [[nodiscard]] bool has_sys_time() const noexcept;
    [[nodiscard]] bool has_setpcap() const noexcept;
    [[nodiscard]] bool has_setfcaps() const noexcept;
    [[nodiscard]] bool is_root() const noexcept;
    [[nodiscard]] uid_t real_uid() const noexcept;
    [[nodiscard]] uid_t effective_uid() const noexcept;
    static CapabilitySet from_current() noexcept;
};

[[nodiscard]] CapabilitySet current_capabilities() noexcept;

class AuditLogger {
public:
    static AuditLogger& instance() noexcept;
    void log(const std::string& module, const std::string& operation,
             const std::string& target, bool success,
             const std::string& details = {});
    void set_level(int level) noexcept;
};

} // namespace nizaw::core
```

### `nizaw::system`

```cpp
namespace nizaw::system {

struct SystemInfo {
    std::string hostname;
    std::string kernel_name;
    std::string kernel_release;
    std::string kernel_version;
    std::string architecture;
    std::string uptime;
    std::string boot_time;
    std::size_t page_size = 0;
    std::size_t cpu_count = 0;
};

Result<SystemInfo> info();

} // namespace nizaw::system
```

### `nizaw::process`

```cpp
namespace nizaw::process {

struct ProcessInfo {
    pid_t pid = 0;
    pid_t ppid = 0;
    uid_t uid = 0;
    gid_t gid = 0;
    std::string name;
    std::string state;
    std::string command;
    std::string executable;
    std::string arguments;
    std::size_t threads = 0;
    std::uint64_t memory_kb = 0;
    double cpu_time_seconds = 0.0;
    std::string start_time;
    std::map<std::string, std::string> environment;
};

struct ResourceLimits {
    std::uint64_t max_cpu_time_seconds = 0;
    std::uint64_t max_file_size_bytes = 0;
    std::uint64_t max_data_size_bytes = 0;
    std::uint64_t max_stack_size_bytes = 0;
    std::uint64_t max_core_file_size_bytes = 0;
    std::uint64_t max_resident_set_bytes = 0;
    std::uint64_t max_processes = 0;
    std::uint64_t max_open_files = 0;
    std::uint64_t max_locked_memory_bytes = 0;
    std::uint64_t max_address_space_bytes = 0;
    std::uint64_t max_file_locks = 0;
    std::uint64_t max_pending_signals = 0;
    std::uint64_t max_msgqueue_size_bytes = 0;
    std::uint64_t max_nice_priority = 0;
    std::uint64_t max_realtime_priority = 0;
    std::uint64_t max_realtime_timeout_us = 0;
};

struct IoStats {
    std::uint64_t read_bytes = 0;
    std::uint64_t write_bytes = 0;
    std::uint64_t cancelled_write_bytes = 0;
    std::uint64_t syscr = 0;  // read syscalls
    std::uint64_t syscw = 0;  // write syscalls
};

Result<std::vector<ProcessInfo>> list();
Result<ProcessInfo> inspect(pid_t pid);
Result<std::map<std::string, std::string>> environment(pid_t pid);
Result<ResourceLimits> resource_limits(pid_t pid);
Result<IoStats> io_stats(pid_t pid);

// --- write operations ---
Result<void> send_signal(pid_t pid, int signal,
                         const core::WriteOptions& options = {});
Result<void> terminate(pid_t pid,
                       const core::WriteOptions& options = {});
Result<void> suspend(pid_t pid,
                     const core::WriteOptions& options = {});
Result<void> resume(pid_t pid,
                    const core::WriteOptions& options = {});
Result<void> set_nice(pid_t pid, int nice_value,
                      const core::WriteOptions& options = {});

} // namespace nizaw::process
```

### `nizaw::filesystem`

```cpp
namespace nizaw::filesystem {

struct DiskUsage {
    std::uint64_t total_bytes = 0;
    std::uint64_t free_bytes = 0;
    std::uint64_t available_bytes = 0;
    std::uint64_t used_bytes = 0;
};

struct EntryInfo {
    std::string path;
    std::string type;
    std::string permissions;
    std::string owner;
    std::string group;
    std::uintmax_t size_bytes = 0;
    std::uintmax_t inode = 0;
    bool exists = false;
    bool is_symlink = false;
    std::string mount_point;
};

Result<DiskUsage> usage(const std::filesystem::path& path);
Result<EntryInfo> info(const std::filesystem::path& path);

// --- write operations ---
Result<void> create_directory(const std::filesystem::path& path,
                              const core::WriteOptions& options = {},
                              mode_t permissions = 0755);
Result<void> remove(const std::filesystem::path& path,
                    const core::WriteOptions& options = {});
Result<void> rename(const std::filesystem::path& from,
                    const std::filesystem::path& to,
                    const core::WriteOptions& options = {});
Result<void> copy(const std::filesystem::path& from,
                  const std::filesystem::path& to,
                  const core::WriteOptions& options = {});
Result<void> set_permissions(const std::filesystem::path& path,
                             mode_t permissions,
                             const core::WriteOptions& options = {});
Result<void> set_owner(const std::filesystem::path& path,
                       uid_t uid, gid_t gid,
                       const core::WriteOptions& options = {});
Result<void> create_symlink(const std::filesystem::path& target,
                            const std::filesystem::path& link,
                            const core::WriteOptions& options = {});
Result<void> write_file(const std::filesystem::path& path,
                        std::string_view content,
                        const core::WriteOptions& options = {},
                        mode_t permissions = 0644);
Result<std::string> read_file(const std::filesystem::path& path);
Result<void> truncate(const std::filesystem::path& path,
                      std::uintmax_t size,
                      const core::WriteOptions& options = {});

} // namespace nizaw::filesystem
```

### `nizaw::storage`

```cpp
namespace nizaw::storage {

enum class DeviceType { Unknown, Disk, Partition, Loop, Ram };

struct Device {
    std::string name;
    std::string sys_path;
    std::string dev_node;
    std::string model;
    std::string vendor;
    std::uint64_t size_bytes = 0;
    std::uint32_t logical_block_size = 0;
    std::uint32_t physical_block_size = 0;
    bool removable = false;
    bool read_only = false;
    bool rotational = false;
    DeviceType type = DeviceType::Unknown;
};

Result<std::vector<Device>> enumerate();
Result<Device> inspect(const std::string& device);
Result<IoStats> iostat(const std::string& device);

enum class FilesystemType {
    Unknown,
    Ext2,
    Ext3,
    Ext4,
    Xfs,
    Btrfs,
    Tmpfs,
    Proc,
    Sysfs,
    Devtmpfs,
    Devpts,
    Securityfs,
    Cgroup,
    Cgroup2,
    Pstore,
    Efivarfs,
    Debugfs,
    Tracefs,
    Nfs,
    Cifs,
    Smb3,
    Fuse,
    Overlay,
    Squashfs,
    Iso9660,
    Udf,
    Fat,
    Vfat,
    Exfat,
    Ntfs,
    Hfs,
    HfsPlus,
    Other
};

struct FilesystemInfo {
    std::string device;
    std::string mount_point;
    std::string fs_type;
    FilesystemType type = FilesystemType::Unknown;
    std::string options;
};

Result<std::vector<FilesystemInfo>> filesystems();

} // namespace nizaw::storage
```

### `nizaw::network`

```cpp
namespace nizaw::network {

struct InterfaceAddress {
    std::string family;
    std::string address;
    std::string netmask;
    std::string broadcast;
};

struct InterfaceInfo {
    std::string name;
    unsigned index = 0;
    std::string state;
    std::string mac_address;
    int mtu = 0;
    std::vector<std::string> flags;
    std::vector<InterfaceAddress> addresses;
    std::uint64_t rx_bytes = 0;
    std::uint64_t tx_bytes = 0;
    std::uint64_t rx_packets = 0;
    std::uint64_t tx_packets = 0;
    std::uint64_t rx_errors = 0;
    std::uint64_t tx_errors = 0;
    std::uint64_t rx_dropped = 0;
    std::uint64_t tx_dropped = 0;
};

Result<std::vector<InterfaceInfo>> list();
Result<InterfaceInfo> inspect(std::string_view interface_name);

} // namespace nizaw::network
```

### `nizaw::service`

```cpp
namespace nizaw::service {

struct ServiceInfo {
    std::string name;
    std::string description;
    std::string load_state;
    std::string active_state;
    std::string sub_state;
    bool loaded = false;
    bool active = false;
    std::optional<bool> enabled;
    std::optional<pid_t> main_pid;
};

Result<std::vector<ServiceInfo>> list();
Result<ServiceInfo> inspect(std::string_view unit_name);

// --- write operations ---
enum class ServiceAction { Start, Stop, Restart, Reload };

Result<void> control(std::string_view unit_name,
                     ServiceAction action,
                     const core::WriteOptions& options = {});
Result<void> enable(std::string_view unit_name,
                    const core::WriteOptions& options = {});
Result<void> disable(std::string_view unit_name,
                     const core::WriteOptions& options = {});

} // namespace nizaw::service
```

### `nizaw::security`

```cpp
namespace nizaw::security {

struct Identity {
    uid_t real_uid{};
    uid_t effective_uid{};
    gid_t real_gid{};
    gid_t effective_gid{};
    std::vector<gid_t> groups;
    bool is_root = false;
};

Result<Identity> identity();
Result<std::vector<std::string>> capabilities();  // human-readable capability names

} // namespace nizaw::security
```

### `nizaw::plugin`

```cpp
namespace nizaw::plugin {

inline constexpr const char* PluginApiVersion = "nizaw-plugin-v1";

struct PluginDescriptor {
    const char* name;
    const char* version;
    const char* description;
    const char* api_version;
    const char* commands;
};

struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string api_version;
    std::vector<std::string> commands;
    std::string path;
};

class Registry {
public:
    Registry() = default;
    ~Registry();
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) noexcept = default;
    Registry& operator=(Registry&&) noexcept = default;

    Result<void> load_directory(std::string_view directory);
    const std::vector<PluginInfo>& plugins() const noexcept;
    bool empty() const noexcept;
};

Result<std::vector<PluginInfo>> discover(std::string_view directory);

} // namespace nizaw::plugin
```

### `nizaw::agent`

```cpp
namespace nizaw::agent {

struct AgentConfig {
    std::string id;
    std::string server_url;
    std::string ca_cert;
    std::string client_cert;
    std::string client_key;
    std::chrono::seconds heartbeat_interval{60};
    int jitter_percent = 30;
    int max_parallel_tasks = 4;
    std::string temp_dir = "/var/cache/nizaw/";
    bool allow_exec = true;
    bool allow_fetch = true;
    bool allow_push = true;
    std::vector<std::string> restricted_paths;
};

struct TelemetryData {
    std::string hostname;
    std::string kernel;
    std::string arch;
    std::string uptime;
    std::string loadavg;
    std::string ip_address;
    std::string agent_version;
};

struct TaskResult {
    std::string task_id;
    bool success = false;
    std::string output;
    int exit_code = -1;
    std::string error_message;
};

// Agent lifecycle
Result<void> start_daemon(const AgentConfig& config);
Result<void> stop_daemon();
Result<bool> is_running();
Result<std::string> get_pid();

// Configuration
Result<AgentConfig> load_config(std::string_view path);
Result<void> validate_config(const AgentConfig& config);

// Telemetry
TelemetryData collect_telemetry();

// Foreground mode for debugging
Result<int> run_foreground(const AgentConfig& config);

} // namespace nizaw::agent
```

### `nizaw::remote`

```cpp
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

class Transport {
public:
    explicit Transport(ServerConfig config);
    ~Transport();

    Result<void> connect();
    Result<TaskResponse> heartbeat(const nizaw::agent::TelemetryData& telemetry);
    Result<void> send_result(const nizaw::agent::TaskResult& result);
    bool is_connected() const noexcept;
    void disconnect() noexcept;
};

// HTTP/2 client with mTLS
Result<std::string> https_post(const ServerConfig& config,
                               std::string_view endpoint,
                               std::string_view json_payload);
Result<std::string> https_get(const ServerConfig& config,
                              std::string_view endpoint);

// Certificate utilities
Result<std::string> load_cert_fingerprint(std::string_view cert_path);
Result<bool> verify_server_fingerprint(std::string_view cert_path,
                                       std::string_view expected_fingerprint);

} // namespace nizaw::remote
```

## 3. API design principles (recap, enforced across all modules)

- Public headers never leak `/proc`/`/sys` parsing details. There is no
  public `ProcParser` type — only `nizaw::process::inspect(pid)`.
- All public types are const-correct; accessor methods are `const`.
- No public API requires the caller to go through the CLI, and no CLI
  command does anything the library API can't also do standalone.
- `Device`/`InterfaceInfo`/`ServiceInfo` etc. are plain, copyable value
  types — not polymorphic unless there's a proven need for substitutability.
- All fallible public APIs return `Result<T>`; functions that cannot fail
  (e.g. `nizaw::core::detect()`) don't wrap their return type.

## 4. JSON output

JSON serialization lives in the `cli` layer, not baked into the domain
structs themselves — domain modules stay serialization-agnostic so library
consumers aren't forced to pull in a JSON dependency they don't want. See
`cli-design.md` §4 for the exact JSON shape per command.

## 5. Shared library / ABI considerations (forward-looking, not a current
commitment)

For a future ABI-stable release:
- Public classes that need ABI stability would move to a pimpl pattern
  (`Device`, `ProcessInfo` as currently sketched are plain structs, which is
  fine for source compatibility but not ABI-stable across struct layout
  changes — this is an accepted trade-off pre-1.0).
- No virtual dispatch across the shared-library boundary except through
  explicitly-versioned interfaces.
- `NIZAW_BUILD_SHARED` (CMake option) builds `libnizaw.so`; a static build
  remains supported and is the default for the CLI binary itself, to keep
  `nizaw` a self-contained executable.