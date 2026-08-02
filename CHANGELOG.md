# Changelog

## 1.1.0 (unreleased)

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
- **New Error Codes**: CapabilityRequired, OperationNotPermitted, ResourceBusy, WouldBlock, InvalidState, PartialFailure, ConfirmationRequired
- **Documentation**: Comprehensive usage examples for write operations in docs/wiki/Usage.md
- **README Updates**: Added write operations examples and CLI commands

### Changed
- Updated all module headers to include write operation declarations
- Enhanced error handling with new error codes for write operations
- Updated API design documentation to reflect new write APIs
- Updated architecture documentation to include write operations in scope

### Fixed
- Compilation warnings and linking errors for new write operation implementations

## 0.1.0 (unreleased)

- Added core architecture and dependency documentation.
- Added core foundation with Result, Error, logging, platform detection, environment helpers, and version metadata.
- Added system information module and tests.
- Added process enumeration and inspection via /proc with resilient error handling.
- Added filesystem usage and metadata inspection with std::filesystem and Linux stat-backed error handling.
- Added storage enumeration for block devices under /sys/block with size, queue, and device metadata.
- Added network interface enumeration with getifaddrs and ioctl-backed MTU, MAC, flags, and addresses.
