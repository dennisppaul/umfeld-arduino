#!/usr/bin/env bash

set -e

BASE_URL="https://raw.githubusercontent.com/dennisppaul/umfeld/main/installation"

echo "-------------------------------"
echo "--- installing dependencies"
echo "-------------------------------"
/bin/bash -c "$(curl -fsSL ${BASE_URL}/install-dependencies.sh)"

echo "-------------------------------"
echo "--- installing umfeld"
echo "-------------------------------"
/bin/bash -c "$(curl -fsSL ${BASE_URL}/install-umfeld.sh)"

echo "-------------------------------"
echo "--- installation complete"
echo "-------------------------------"
