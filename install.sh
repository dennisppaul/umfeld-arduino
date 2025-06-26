#!/usr/bin/env bash

set -e

BASE_URL="https://raw.githubusercontent.com/dennisppaul/umfeld/main/installation"
UMFELD_VERSION="v2.1.0"

echo "---------------------------"
echo "+++ installing dependencies"
echo "---------------------------"
/bin/bash -c "$(curl -fsSL ${BASE_URL}/install-dependencies.sh)"

echo "---------------------"
echo "+++ installing umfeld"
echo "---------------------"
# NOTE this command will clone the latest version of the umfeld repositories with full commit history
# /bin/bash -c "$(curl -fsSL ${BASE_URL}/install-umfeld.sh)"
# NOTE this command will install a specific version with shallow history ( only works with v2.2.0 and later ).
#      if the repositories were cloned with the `--depth` flag, the full commit history can be retrieved later using `git fetch --unshallow`.
/bin/bash -c "$(curl -fsSL ${BASE_URL}/install-umfeld.sh)" -- --tag $UMFELD_VERSION --depth 1

echo "-------------------------"
echo "+++ installation complete"
echo "-------------------------"
