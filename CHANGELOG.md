# Changelog

## 0.1.0 (unreleased)

- Added Phase 0 architecture and dependency documentation.
- Added Phase 1 core foundation with Result, Error, logging, platform detection, environment helpers, and version metadata.
- Added Phase 2 system information module and tests.
- Added Phase 3 process enumeration and inspection via /proc with resilient error handling.
- Added Phase 4 filesystem usage and metadata inspection with std::filesystem and Linux stat-backed error handling.
- Added Phase 5 storage enumeration for block devices under /sys/block with size, queue, and device metadata.
- Added Phase 6 network interface enumeration with getifaddrs and ioctl-backed MTU, MAC, flags, and addresses.
