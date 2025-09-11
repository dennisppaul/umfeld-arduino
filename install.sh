#!/usr/bin/env bash

set -e

BASE_URL="https://raw.githubusercontent.com/dennisppaul/umfeld/main/installation"
TMP=$(mktemp)

echo "-------------------------------"
echo "--- installing dependencies"
echo "-------------------------------"
curl -fsSL "${BASE_URL}/install-dependencies.sh" > "$TMP" && bash "$TMP"

echo "-------------------------------"
echo "--- installing umfeld"
echo "-------------------------------"
curl -fsSL "${BASE_URL}/install-umfeld.sh"       > "$TMP" && bash "$TMP"

rm -f "$TMP"

echo "-------------------------------"
echo "--- installation complete"
echo "-------------------------------"
