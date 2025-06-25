#!/usr/bin/env bash

set -e

BASE_URL="https://raw.githubusercontent.com/dennisppaul/umfeld/main"

echo "---------------------"
echo "+++ installing dependencies"
/bin/bash -c "$(curl -fsSL ${BASE_URL}/install-dependencies.sh)"

echo "---------------------"
echo "+++ installing umfeld"
/bin/bash -c "$(curl -fsSL ${BASE_URL}/install-umfeld.sh)"

echo "+++ installation complete"
