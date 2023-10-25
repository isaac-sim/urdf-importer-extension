#!/usr/bin/env bash

set -e

SCRIPT_DIR="$(dirname "${BASH_SOURCE}")"

# Verify formatting
# "$SCRIPT_DIR/../../../../format_code.sh" --verify

# Full rebuild
"$SCRIPT_DIR/../../../../build.sh" -x -r -d

# Package all
"$SCRIPT_DIR/../../../../repo.sh" package -m main_package -c release

# Package all
"$SCRIPT_DIR/../../../../repo.sh" package -m main_package -c debug

# publish artifacts to teamcity
echo "##teamcity[publishArtifacts '_build/packages']"
echo "##teamcity[publishArtifacts '_build/**/*.log']"
