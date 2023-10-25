#!/usr/bin/env bash

set -e

SCRIPT_DIR="$(dirname "${BASH_SOURCE}")"

# Pull kit sdk to do the publishing
"$SCRIPT_DIR/../../../../build.sh" -r --fetch-only

"$SCRIPT_DIR/../../../../repo.sh" publish_exts -a --from-package $*

echo "##teamcity[publishArtifacts '_build/**/*.log']"
