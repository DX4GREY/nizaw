#!/bin/bash
# Portable build verification script
# Checks that the nizaw executable does not depend on forbidden shared libraries

set -e

NIZAW_BIN="${1:-build/nizaw}"

if [ ! -f "$NIZAW_BIN" ]; then
    echo "ERROR: Binary not found: $NIZAW_BIN"
    exit 1
fi

echo "=== Verifying portable binary: $NIZAW_BIN ==="
echo ""

# Check with ldd
if command -v ldd >/dev/null 2>&1; then
    echo "Running ldd check..."
    LDD_OUTPUT=$(ldd "$NIZAW_BIN" 2>/dev/null || true)
    if echo "$LDD_OUTPUT" | grep -qE 'libssl|libcrypto|libz|libsqlite3|libsystemd'; then
        echo "FORBIDDEN: Found Nizaw-specific shared dependencies in ldd output!"
        echo "$LDD_OUTPUT" | grep -E 'libssl|libcrypto|libz|libsqlite3|libsystemd'
        exit 1
    else
        echo "OK: No forbidden shared dependencies found in ldd"
    fi
    echo ""
else
    echo "WARNING: ldd not found, skipping ldd check"
    echo ""
fi

# Check ELF dynamic section with readelf
if command -v readelf >/dev/null 2>&1; then
    echo "Running readelf dynamic section check..."
    if readelf -d "$NIZAW_BIN" 2>/dev/null | grep -qE 'libssl|libcrypto|libz|libsqlite3|libsystemd'; then
        echo "FORBIDDEN: Found Nizaw-specific shared dependencies in ELF dynamic section!"
        readelf -d "$NIZAW_BIN" | grep -E 'libssl|libcrypto|libz|libsqlite3|libsystemd'
        exit 1
    else
        echo "OK: No forbidden shared dependencies in ELF dynamic section"
    fi
    echo ""
else
    echo "WARNING: readelf not found, skipping readelf check"
    echo ""
fi

# Check imported libraries with objdump
if command -v objdump >/dev/null 2>&1; then
    echo "Running objdump import check..."
    if objdump -p "$NIZAW_BIN" 2>/dev/null | grep -qE 'NEEDED.*lib(ssl|crypto|z|sqlite3|systemd)'; then
        echo "FORBIDDEN: Found Nizaw-specific shared imports!"
        objdump -p "$NIZAW_BIN" | grep -E 'NEEDED.*lib(ssl|crypto|z|sqlite3|systemd)'
        exit 1
    else
        echo "OK: No forbidden shared imports"
    fi
    echo ""
else
    echo "WARNING: objdump not found, skipping objdump check"
    echo ""
fi

echo "=== Portable verification complete ==="
echo "Binary size: $(du -h "$NIZAW_BIN" | cut -f1)"
echo ""

# Show what dynamic libraries remain (should only be system libs)
if command -v ldd >/dev/null 2>&1; then
    echo "Remaining dynamic dependencies (expected: libc, ld-linux, etc.):"
    ldd "$NIZAW_BIN" | grep -v 'not found' || true
fi

exit 0