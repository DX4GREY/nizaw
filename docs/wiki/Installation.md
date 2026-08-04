# Installation

## Supported environment

- Linux (Ubuntu, Debian, Arch, Fedora, Kali, and similar distributions)
- C++20 compiler (GCC or Clang)
- CMake 3.20 or newer
- Ninja (recommended)
- pkg-config

Optional:
- `libsystemd` / `sd-bus` for the service module when available
- OpenSSL, zlib, SQLite3 for the agent/remote modules (required when `NIZAW_BUILD_AGENT=ON`)

## Install prerequisites

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config libssl-dev zlib1g-dev libsqlite3-dev
```

### Fedora

```bash
sudo dnf install -y gcc-c++ cmake ninja-build pkgconfig openssl-devel zlib-devel sqlite-devel
```

### Arch

```bash
sudo pacman -S --needed base-devel cmake ninja pkgconf openssl zlib sqlite
```

## Build from source

```bash
git clone https://github.com/DX4GREY/nizaw.git
cd nizaw
cmake -S . -B build -G Ninja -DNIZAW_BUILD_CLI=ON
cmake --build build --parallel --target nizaw
```

## Build options

| Option | Default | Description |
| --- | --- | --- |
| `NIZAW_BUILD_CLI` | `ON` | Build the `nizaw` CLI executable |
| `NIZAW_BUILD_TESTS` | `ON` | Build unit/integration tests |
| `NIZAW_BUILD_EXAMPLES` | `OFF` | Build example programs |
| `NIZAW_BUILD_SHARED` | `OFF` | Build `libnizaw` as a shared library |
| `NIZAW_ENABLE_WARNINGS` | `ON` | Enable strict compiler warnings |
| `NIZAW_ENABLE_ASAN` | `OFF` | Enable AddressSanitizer |
| `NIZAW_ENABLE_UBSAN` | `OFF` | Enable UndefinedBehaviorSanitizer |
| `NIZAW_ENABLE_SYSTEMD` | `ON` | Enable systemd/sd-bus backed `nizaw::service` |
| `NIZAW_BUILD_AGENT` | `ON` | Build the nizaw agent (remote orchestration) |

## Build examples (optional)

```bash
cmake -S . -B build -G Ninja -DNIZAW_BUILD_EXAMPLES=ON
cmake --build build --parallel --target nizaw_example
./build/examples/nizaw_example
./build/examples/nizaw_example --json
```

## Verify the build

```bash
ctest --test-dir build --output-on-failure
./build/nizaw --help
./build/nizaw system info
```

## Install locally (optional)

If you want to expose the CLI from a standard location, you can install it into the chosen prefix:

```bash
cmake --install build --prefix /tmp/nizaw-install
```

## Notes about optional systemd support

If `libsystemd` is installed, the service module will build with systemd-backed support. If it is not installed, the build falls back to a stub implementation so the rest of the project still compiles.

## Notes about agent/remote support

The agent and remote modules require OpenSSL, zlib, and SQLite3. These are enabled by default via `NIZAW_BUILD_AGENT=ON`. To build without agent support (minimal dependency-free build):

```bash
cmake -S . -B build -G Ninja -DNIZAW_BUILD_AGENT=OFF
cmake --build build --parallel
```
