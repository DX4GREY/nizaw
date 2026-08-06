#!/usr/bin/env bash
set -euo pipefail

# Build an Arch Linux (.pkg.tar.zst) package for nizaw
# Usage: build-arch.sh <version> <staging_dir> <output_dir>
#   version     - e.g. 3.0.5
#   staging_dir - directory containing the installed files (usr/...)
#   output_dir  - where to place the resulting .pkg.tar.zst

VERSION="${1:?Usage: build-arch.sh <version> <staging_dir> <output_dir>}"
STAGING_DIR="${2:?Usage: build-arch.sh <version> <staging_dir> <output_dir>}"
OUTPUT_DIR="${3:?Usage: build-arch.sh <version> <staging_dir> <output_dir>}"

# makepkg refuses to run as root - it can cause catastrophic damage
if [ "$(id -u)" -eq 0 ]; then
    echo "ERROR: build-arch.sh must NOT be run as root. Use a regular user (e.g. runuser -u builder)." >&2
    exit 1
fi

PKG_NAME="nizaw"
PKG_VERSION="${VERSION#v}"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

# Copy PKGBUILD template and substitute version
sed "s/__VERSION__/${PKG_VERSION}/g" "$(dirname "$0")/PKGBUILD" > "${WORK_DIR}/PKGBUILD"

# Place staging files into ${srcdir}/staging where the PKGBUILD package() expects them.
# makepkg sets SRCDEST/srcdir to ${WORK_DIR}/src by default.
mkdir -p "${WORK_DIR}/src/staging"
cp -a "${STAGING_DIR}/." "${WORK_DIR}/src/staging/"

# Build the package
cd "${WORK_DIR}"
makepkg -f --noconfirm

# Copy resulting package to output
mkdir -p "${OUTPUT_DIR}"
find "${WORK_DIR}" -maxdepth 1 -name "*.pkg.tar.zst" -exec cp {} "${OUTPUT_DIR}/" \;

echo "Built:"
ls -lh "${OUTPUT_DIR}"/*.pkg.tar.zst