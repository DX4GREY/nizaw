#!/usr/bin/env bash
set -euo pipefail

# Build a Fedora/RPM (.rpm) package for nizaw
# Usage: build-rpm.sh <version> <staging_dir> <output_dir>
#   version     - e.g. 3.0.5
#   staging_dir - directory containing the installed files (usr/...)
#   output_dir  - where to place the resulting .rpm

VERSION="${1:?Usage: build-rpm.sh <version> <staging_dir> <output_dir>}"
STAGING_DIR="${2:?Usage: build-rpm.sh <version> <staging_dir> <output_dir>}"
OUTPUT_DIR="${3:?Usage: build-rpm.sh <version> <staging_dir> <output_dir>}"

ARCH="${RPM_ARCH:-x86_64}"
PKG_NAME="nizaw"
PKG_VERSION="${VERSION#v}"
PKG_RELEASE="${PKG_RELEASE:-1}"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

# Set up rpmbuild directory structure
RPMBUILD_DIR="${WORK_DIR}/rpmbuild"
mkdir -p "${RPMBUILD_DIR}"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

# Create source tarball from staging dir.
# %setup -c -n %{name}-%{version} already creates the top-level
# directory, so the tarball must contain the installed files directly
# (no ${PKG_NAME}-${PKG_VERSION}/ prefix) to avoid double nesting.
TARBALL="${RPMBUILD_DIR}/SOURCES/${PKG_NAME}-${PKG_VERSION}.tar.gz"
tar -czf "${TARBALL}" -C "${STAGING_DIR}" .

# Generate %files list dynamically from the staging directory.
# This ensures only files that actually exist are packaged.
FILES_LIST="${RPMBUILD_DIR}/SPECS/files.list"
(
    cd "${STAGING_DIR}"
    find . \( -type f -o -type l \) | while read -r f; do
        echo "/${f#./}"
    done
    # Include directories that contain files (needed for ownership)
    find . -type d | while read -r d; do
        if [ "$d" != "." ]; then
            echo "%dir /${d#./}"
        fi
    done
) | sort -u > "${FILES_LIST}"

# Write spec file
cat > "${RPMBUILD_DIR}/SPECS/${PKG_NAME}.spec" <<EOF
%define _enable_debug_package 0
%define debug_package %{nil}

Name:           ${PKG_NAME}
Version:        ${PKG_VERSION}
Release:        ${PKG_RELEASE}%{?dist}
Summary:        Modern Linux System & CLI Framework written in C++20

License:        MIT
URL:            https://github.com/DX4GREY/nizaw
Source0:        %{name}-%{version}.tar.gz

BuildArch:      ${ARCH}
BuildRoot:      %{_topdir}/BUILDROOT

Requires:       glibc >= 2.34
Requires:       libgcc
Requires:       libstdc++ >= 11
Requires:       systemd-libs
Requires:       openssl-libs
Requires:       zlib
Requires:       sqlite-libs

%description
Nizaw is a modern C++20 framework providing a unified API for system
information, process management, filesystem operations, storage, network,
services (systemd), security, and plugin development. It includes a CLI
application and an optional remote agent for orchestration.

%prep
%setup -q -c -n %{name}-%{version}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
cp -a . %{buildroot}/

%files -f ${RPMBUILD_DIR}/SPECS/files.list
EOF

# Build the RPM
rpmbuild --define "_topdir ${RPMBUILD_DIR}" \
         --define "_build_id_links none" \
         -bb "${RPMBUILD_DIR}/SPECS/${PKG_NAME}.spec"

# Copy resulting RPM(s) to output
mkdir -p "${OUTPUT_DIR}"
find "${RPMBUILD_DIR}/RPMS" -name "*.rpm" -exec cp {} "${OUTPUT_DIR}/" \;

echo "Built RPMs:"
ls -lh "${OUTPUT_DIR}"/*.rpm