#!/usr/bin/env bash

set -e

SCRIPT_DIR="$(dirname "${BASH_SOURCE}")"

# Verify formatting
# "$SCRIPT_DIR/../../../../format_code.sh" --verify

# Full rebuild
"$SCRIPT_DIR/../../../../build.sh" -x -r -d --platform-target linux-aarch64

# Package all
"$SCRIPT_DIR/../../../../repo.sh" package -m main_package -c release --platform-target linux-aarch64

# Package all
"$SCRIPT_DIR/../../../../repo.sh" package -m main_package -c debug --platform-target linux-aarch64

# publish artifacts to teamcity
echo "##teamcity[publishArtifacts '_build/packages']"
