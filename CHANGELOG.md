# Changelog

## 0.1.0 (unreleased)

- Added core architecture and dependency documentation.
- Added core foundation with Result, Error, logging, platform detection, environment helpers, and version metadata.
- Added system information module and tests.
- Added process enumeration and inspection via /proc with resilient error handling.
- Added filesystem usage and metadata inspection with std::filesystem and Linux stat-backed error handling.
- Added storage enumeration for block devices under /sys/block with size, queue, and device metadata.
- Added network interface enumeration with getifaddrs and ioctl-backed MTU, MAC, flags, and addresses.
