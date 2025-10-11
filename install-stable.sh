#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

readonly BASE_URL="https://raw.githubusercontent.com/dennisppaul/umfeld/main/installation"
readonly UMFELD_VERSION="v2.5.0"

# Always clean up temp files, even on error/CTRL-C
TMP_DEP=$(mktemp) ; TMP_UMF=$(mktemp)
cleanup() { rm -f "$TMP_DEP" "$TMP_UMF"; }
trap cleanup EXIT

echo "-------------------------------"
echo "--- installing dependencies"
echo "-------------------------------"
curl -fsSL "${BASE_URL}/install-dependencies.sh" > "$TMP_DEP"
bash "$TMP_DEP"

echo "-------------------------------"
echo "--- installing umfeld"
echo "-------------------------------"
# latest with full history:
#   curl -fsSL "${BASE_URL}/install-umfeld.sh" > "$TMP_UMF" && bash "$TMP_UMF"
# pinned + shallow (v2.2.0+):
curl -fsSL "${BASE_URL}/install-umfeld.sh" > "$TMP_UMF"
bash "$TMP_UMF" --tag "$UMFELD_VERSION" --depth 1

echo "-------------------------------"
echo "--- installation complete"
echo "-------------------------------"