# Nizaw — API Design

Status: Phase 0 — Draft for review

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
};

class Error {
public:
    Error(ErrorCode code, std::string message, std::string_view source,
          std::optional<int> errno_value = std::nullopt);

    ErrorCode code() const noexcept;
    const std::string& message() const noexcept;
    std::string_view source() const noexcept;      // e.g. "storage", "process"
    std::optional<int> errno_value() const noexcept;

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

    explicit operator bool() const noexcept;  // true if holds T
    const T& value() const&;                  // throws std::logic_error if error (programmer bug, not expected-path)
    T& value() &;
    T&& value() &&;
    const Error& error() const&;

    // monadic helpers, added only if a real use case appears (avoid overengineering):
    // and_then, map, or_else
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

## 2. Module namespaces and initial signatures

These are the **initial** public surfaces sketched in Phase 0 for planning
purposes. Each is only actually implemented in its corresponding phase
(Phase 2 for `system`, Phase 3 for `process`, etc.) and may be refined at
that point — Phase 0 does not freeze exact struct fields, only the shape of
the API.

```cpp
namespace nizaw::system {

struct SystemInfo {
    std::string hostname;
    std::string kernel_name;
    std::string kernel_release;
    std::string kernel_version;
    std::string architecture;
    std::chrono::seconds uptime;
    std::optional<std::chrono::system_clock::time_point> boot_time;
    long page_size{};
    unsigned cpu_count{};
};

Result<SystemInfo> info();
Result<std::chrono::seconds> uptime();
Result<std::string> hostname();
}

namespace nizaw::process {

enum class ProcessState { Running, Sleeping, DiskSleep, Zombie, Stopped, Unknown };

struct ProcessInfo {
    pid_t pid{};
    pid_t ppid{};
    uid_t uid{};
    gid_t gid{};
    std::string command;
    std::optional<std::filesystem::path> executable;
    ProcessState state{ProcessState::Unknown};
    unsigned thread_count{};
    uint64_t memory_bytes{};
    std::optional<std::chrono::microseconds> cpu_time;
    std::chrono::system_clock::time_point start_time;
    std::vector<std::string> arguments;  // empty if unavailable (permission/zombie)
};

Result<std::vector<ProcessInfo>> list();
Result<ProcessInfo> inspect(pid_t pid);
}

namespace nizaw::filesystem {

struct DiskUsage {
    uint64_t total_bytes{};
    uint64_t free_bytes{};
    uint64_t available_bytes{};
};

struct PathInfo {
    std::filesystem::file_type type;
    std::filesystem::perms permissions;
    uid_t owner{};
    gid_t group{};
    std::optional<uint64_t> inode;
    std::string mount_point;
    bool is_symlink{};
    bool broken_symlink{};
};

Result<DiskUsage> disk_usage(const std::filesystem::path& path);
Result<PathInfo> info(const std::filesystem::path& path);
}

namespace nizaw::storage {

enum class DeviceKind { Nvme, Sata, MmcEmmc, UsbStorage, Virtual, Unknown };

class Device {
public:
    const std::string& name() const;       // "nvme0n1"
    const std::string& model() const;
    const std::string& vendor() const;
    uint64_t size_bytes() const;
    uint64_t logical_block_size() const;
    uint64_t physical_block_size() const;
    bool removable() const;
    bool read_only() const;
    bool rotational() const;
    DeviceKind kind() const;
};

enum class HealthStatus { Ok, Warning, Critical, Unsupported };

struct StorageHealth {
    HealthStatus status;
    std::optional<std::string> unsupported_reason;  // set iff status == Unsupported
    std::optional<uint64_t> power_on_hours;
    std::optional<double> percentage_used;           // only when device actually reports it
};

Result<std::vector<Device>> enumerate();
Result<Device> info(const std::string& device_name);
Result<StorageHealth> health(const std::string& device_name);
}

namespace nizaw::network {

struct Interface {
    std::string name;
    unsigned index{};
    bool is_up{};
    std::string mac_address;
    unsigned mtu{};
    std::vector<std::string> ipv4_addresses;
    std::vector<std::string> ipv6_addresses;
};

Result<std::vector<Interface>> interfaces();
Result<Interface> info(const std::string& interface_name);
}

namespace nizaw::service {

enum class ActiveState { Active, Inactive, Activating, Deactivating, Failed, Unknown };
enum class LoadState { Loaded, NotFound, Error, Unknown };

struct ServiceInfo {
    std::string name;
    ActiveState active_state{};
    LoadState load_state{};
    bool enabled{};
    std::optional<pid_t> main_pid;
    std::string description;
};

// Abstracted so a non-systemd backend could implement this interface later.
class ServiceManager {
public:
    virtual ~ServiceManager() = default;
    virtual Result<std::vector<ServiceInfo>> list() = 0;
    virtual Result<ServiceInfo> status(const std::string& unit_name) = 0;
};

Result<std::unique_ptr<ServiceManager>> default_manager();  // systemd-backed in Phase 7
}

namespace nizaw::security {

struct Identity {
    uid_t real_uid{};
    uid_t effective_uid{};
    gid_t real_gid{};
    gid_t effective_gid{};
    std::vector<gid_t> groups;
    bool is_root{};
};

Result<Identity> identity();
Result<std::vector<std::string>> capabilities();  // human-readable capability names
}
```

## 3. API design principles (recap, enforced across all modules)

- Public headers never leak `/proc`/`/sys` parsing details. There is no
  public `ProcParser` type — only `nizaw::process::inspect(pid)`.
- All public types are const-correct; accessor methods are `const`.
- No public API requires the caller to go through the CLI, and no CLI
  command does anything the library API can't also do standalone.
- `Device`/`Interface`/`ServiceInfo` etc. are plain, copyable value-ish
  types or thin accessor classes — not polymorphic unless there's a proven
  need for substitutability (`ServiceManager` is the one deliberate
  exception, because "swap the backend" is a named goal).

## 4. JSON output

JSON serialization lives in the `cli` layer (or a thin `nizaw::core::json`
helper used by `cli`), not baked into the domain structs themselves — domain
modules stay serialization-agnostic so library consumers aren't forced to
pull in a JSON dependency they don't want. See `cli-design.md` §4 for the
exact JSON shape per command.

## 5. Shared library / ABI considerations (forward-looking, not a Phase 0
commitment)

For a future ABI-stable release:
- Public classes that need ABI stability would move to a pimpl pattern
  (`Device`, `ProcessInfo` as currently sketched are plain structs, which is
  fine for source compatibility but not ABI-stable across struct layout
  changes — this is an accepted trade-off pre-1.0).
- No virtual dispatch across the shared-library boundary except through
  explicitly-versioned interfaces (`ServiceManager` is designed with this in
  mind — a version tag will be added before it's considered ABI-frozen).
- `NIZAW_BUILD_SHARED` (CMake option) builds `libnizaw.so`; a static build
  remains supported and is the default for the CLI binary itself, to keep
  `nizaw` a self-contained executable.
