# Changelog

## 2.1.0 (current)

### Added
- Network interface statistics: `rx_bytes`, `tx_bytes`, `rx_packets`, `tx_packets`, `rx_errors`, `tx_errors`, `rx_dropped`, `tx_dropped` in `InterfaceInfo`
- Process resource limits: `process::resource_limits(pid)` returning `ResourceLimits` struct with all RLIMIT_* values
- Process I/O statistics: `process::io_stats(pid)` returning `IoStats` with read/write bytes, syscalls, cancelled writes
- Storage filesystem enumeration: `storage::filesystems()` returning list of mounted filesystems with type detection
- `FilesystemType` enum with 30+ filesystem types (ext4, xfs, btrfs, tmpfs, nfs, cifs, overlay, etc.)
- `FilesystemInfo` struct with device, mount_point, fs_type, type, and options

### Changed
- Network `list()` and `inspect()` now populate statistics fields from `/sys/class/net/*/statistics`

## 2.0.1 (current)

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
