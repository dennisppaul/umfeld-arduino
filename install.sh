#!/usr/bin/env bash

# examples:
# - install dev branch with '--ref'         : /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/dennisppaul/umfeld/main/install.sh)" -- --ref dev
# - install dev branch with env var override: UMFELD_REF=dev /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/dennisppaul/umfeld/main/install.sh)"
# - install specific tag                    : /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/dennisppaul/umfeld/main/install.sh)" -- --ref v2.4.0

set -euo pipefail
IFS=$'\n\t'

# --- early requirements -------------------------------------------------------
if ! command -v curl >/dev/null 2>&1; then
  echo "ERROR: 'curl' is not installed. Please install curl and try again." >&2
  exit 1
fi

# --- optional: parse --ref <ref> before loading common.sh ---------------------
if [[ $# -ge 2 && "$1" == "--ref" ]]; then
  UMFELD_REF="$2"
  shift 2
fi
: "${REPO:=dennisppaul/umfeld}"      # allow override via env
: "${UMFELD_REF:=main}"

# --- load common config (builds BASE_URL/BUNDLE_URL) --------------------------
COMMON_TMP=$(mktemp)
curl -fsSL "https://raw.githubusercontent.com/${REPO}/${UMFELD_REF}/installation/common.sh" -o "$COMMON_TMP"
# shellcheck source=/dev/null
. "$COMMON_TMP"
rm -f "$COMMON_TMP"

# --- temp files + cleanup -----------------------------------------------------
TMP_DEP=$(mktemp)
TMP_UMF=$(mktemp)
cleanup() { rm -f "$TMP_DEP" "$TMP_UMF"; }
trap cleanup EXIT

fetch() {
  local url=$1 out=$2
  curl --fail --show-error --silent --location \
       --connect-timeout 10 --max-time 300 \
       --retry 3 --retry-delay 2 --retry-connrefused \
       -o "$out" "$url"
  [[ -s "$out" ]] || { echo "ERROR: downloaded file is empty: $url" >&2; exit 1; }
  head -n1 "$out" | grep -qE '^#!' || { echo "ERROR: not a script (missing shebang): $url" >&2; exit 1; }
}

run_script() {
  local path=$1
  hash -r
  bash "$path"
}

echo "-------------------------------"
echo "--- installing dependencies"
echo "-------------------------------"
echo 
fetch "${BASE_URL}/install-dependencies.sh" "$TMP_DEP"
run_script "$TMP_DEP"
echo

echo "-------------------------------"
echo "--- installing umfeld"
echo "-------------------------------"
echo

if [[ "$UMFELD_REF" != "main" ]]; then
  bash "$TMP_UMF" --tag "$UMFELD_REF"
else
  run_script "$TMP_UMF"
fi

echo "-------------------------------"
echo "--- installation complete"
echo "-------------------------------"