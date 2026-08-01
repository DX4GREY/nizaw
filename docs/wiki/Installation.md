# Installation

## Supported environment

- Linux (Ubuntu, Debian, Arch, Fedora, Kali, and similar distributions)
- C++20 compiler (GCC or Clang)
- CMake 3.20 or newer
- Ninja (recommended)
- pkg-config

Optional:
- `libsystemd` / `sd-bus` for the service module when available

## Install prerequisites

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config
```

### Fedora

```bash
sudo dnf install -y gcc-c++ cmake ninja-build pkgconfig
```

### Arch

```bash
sudo pacman -S --needed base-devel cmake ninja pkgconf
```

## Build from source

```bash
git clone https://github.com/DX4GREY/nizaw.git
cd nizaw
cmake -S . -B build -G Ninja -DNIZAW_BUILD_CLI=ON
cmake --build build --parallel --target nizaw
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
