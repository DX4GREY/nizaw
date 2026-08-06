#!/usr/bin/env bash
set -euo pipefail

# Build a Debian (.deb) package for nizaw
# Usage: build-deb.sh <version> <staging_dir> <output_dir>
#   version     - e.g. 3.0.5
#   staging_dir - directory containing the installed files (usr/...)
#   output_dir  - where to place the resulting .deb

VERSION="${1:?Usage: build-deb.sh <version> <staging_dir> <output_dir>}"
STAGING_DIR="${2:?Usage: build-deb.sh <version> <staging_dir> <output_dir>}"
OUTPUT_DIR="${3:?Usage: build-deb.sh <version> <staging_dir> <output_dir>}"

ARCH="${DEB_ARCH:-amd64}"
PKG_NAME="nizaw"
PKG_VERSION="${VERSION#v}"
PKG_RELEASE="${PKG_RELEASE:-1}"
FULL_VERSION="${PKG_VERSION}-${PKG_RELEASE}"

# Normalize version for Debian (no leading 'v', replace '-' with '~')
DEB_VERSION="${PKG_VERSION//-/\~}"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

PKG_ROOT="${WORK_DIR}/pkg"
mkdir -p "${PKG_ROOT}/DEBIAN"

# Copy staged files into package root
cp -a "${STAGING_DIR}/." "${PKG_ROOT}/"

# Compute installed size in KB
INSTALLED_SIZE="$(du -sk "${PKG_ROOT}" | awk '{print $1}')"

# Determine dependencies
# libsystemd0 for service module, libssl3 for remote/agent, zlib1g, libsqlite3-0
DEPENDS="libc6 (>= 2.34), libgcc-s1, libstdc++6 (>= 11), libsystemd0, libssl3, zlib1g, libsqlite3-0"

# Write control file
cat > "${PKG_ROOT}/DEBIAN/control" <<EOF
Package: ${PKG_NAME}
Version: ${DEB_VERSION}
Section: libs
Priority: optional
Architecture: ${ARCH}
Depends: ${DEPENDS}
Installed-Size: ${INSTALLED_SIZE}
Maintainer: DX4GREY <dx4grey@users.noreply.github.com>
Description: Modern Linux System & CLI Framework written in C++20
 Nizaw is a modern C++20 framework providing a unified API for system
 information, process management, filesystem operations, storage, network,
 services (systemd), security, and plugin development. It includes a CLI
 application and an optional remote agent for orchestration.
Homepage: https://github.com/DX4GREY/nizaw
EOF

# Write conffiles if any exist in /etc
if [ -d "${PKG_ROOT}/etc" ]; then
    find "${PKG_ROOT}/etc" -type f | sed "s|^${PKG_ROOT}||" > "${PKG_ROOT}/DEBIAN/conffiles"
fi

# Build the package
mkdir -p "${OUTPUT_DIR}"
dpkg-deb --build --root-owner-group "${PKG_ROOT}" "${OUTPUT_DIR}/${PKG_NAME}_${DEB_VERSION}_${ARCH}.deb"

echo "Built: ${OUTPUT_DIR}/${PKG_NAME}_${DEB_VERSION}_${ARCH}.deb"