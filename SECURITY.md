# Security Policy

## Overview

Nizaw is a Linux system introspection and management framework. As such, it
interacts with sensitive system information and can perform privileged
operations. This document outlines security considerations for users and
contributors.

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 2.0.x   | Yes (current)      |
| 1.x     | No                 |

## Reporting a Vulnerability

If you discover a security vulnerability in Nizaw, please report it via
GitHub Security Advisory (GHSA) on the repository:
https://github.com/DX4GREY/nizaw/security/advisories

Do not open a public issue for security vulnerabilities.

## Security Considerations for Users

### Privileged Operations

Nizaw includes write operations that can affect system state:
- Filesystem manipulation (create, delete, modify files/directories)
- Process control (send signals, terminate, suspend/resume, change priority)
- Service management (start/stop/enable/disable systemd units)

These operations require appropriate Linux capabilities or root privileges:
- `CAP_SYS_ADMIN` for many filesystem operations
- `CAP_KILL` for sending signals to processes
- `CAP_SYS_PTRACE` for certain process operations
- `CAP_NET_ADMIN` for network configuration (future)

### Safety Features

Nizaw implements multiple safety layers for write operations:

1. **Confirmation prompts**: Destructive operations prompt for confirmation by default
2. **`--yes` flag**: Explicit opt-in to skip prompts for automation
3. **Capability checks**: Operations verify required capabilities before execution
4. **`WriteOptions`**: Granular control over operation behavior
5. **`AuditLogger`**: Structured logging of all mutating operations

### Best Practices

- Run Nizaw with the minimum required privileges
- Use `--dry-run` to validate operations before executing
- Review `AuditLogger` output for compliance and security auditing
- Do not use `--force` flag in production without proper review
- Keep Nizaw updated to receive security fixes

### Plugin Security

Plugins are loaded as shared libraries (`.so` files) and execute with the same
privileges as the Nizaw process. Consider:

- Only load plugins from trusted sources
- Verify plugin API version compatibility
- Review plugin code before loading
- Use separate user accounts for different plugin sets

## Security Considerations for Contributors

### Code Review

- All changes undergo code review with security in mind
- Pay special attention to:
  - Syscall wrappers (validate all inputs)
  - Capability checks (fail closed, not open)
  - Path handling (prevent directory traversal)
  - Buffer operations (prevent overflows)

### Prohibited Patterns

- Never use `system()`, `popen()`, or similar shell-execution functions
- Never parse shell command output instead of using Linux APIs directly
- Never log sensitive data (credentials, private keys, full environment dumps)
- Never ignore error returns from syscalls without explicit justification

### Secure Coding Guidelines

- Use `Result<T>` for all fallible operations
- Validate all user inputs (PIDs, paths, signals)
- Check capabilities before privileged operations
- Use RAII for resource management (file descriptors, memory)
- Keep the CLI thin; security logic belongs in the library layer

## Dependencies

Nizaw maintains a minimal dependency footprint. See
[docs/dependency-policy.md](docs/dependency-policy.md) for details.

Current dependencies:
- C++20 standard library (required)
- libsystemd/sd-bus (optional, for service module only)

When adding dependencies, consider:
- License compatibility
- Maintenance status and bus factor
- Security track record
- Binary size and transitive dependencies

## Incident Response

In case of a security incident:

1. Assess the impact and affected versions
2. Develop and test a fix in a private branch
3. Coordinate disclosure timeline with reporter
4. Release patched versions with security advisory
5. Notify users through GitHub Security Advisory

## License

Nizaw is currently prepared for a future explicit license choice.
