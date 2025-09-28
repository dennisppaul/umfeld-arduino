#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

# defaults (overridable by flags)
AUTO_YES=false
TARGET_DIR="$(pwd)"
TAG=""
DEPTH=""

usage() {
  cat <<'EOF'
Usage: install-umfeld.sh [options]

Options:
  -y, --yes                 Non-interactive; accept prompts
      --install-dir <path>  Installation directory (default: current directory)
      --tag <vX.Y.Z|ref>    Tag/ref to clone (v2.2.0+ applies to all repos)
      --depth <N>           Shallow clone depth (positive integer)
  -h, --help                Show this help and exit

Examples:
  ./install-umfeld.sh
  ./install-umfeld.sh --tag v2.4.0 --depth 1 -y
  ./install-umfeld.sh --install-dir /opt/umfeld -y
EOF
}

# ---- parse args -------------------------------------------------------------
if [[ $# -gt 0 ]]; then
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -y|--yes) AUTO_YES=true; shift ;;
      --install-dir)
        [[ $# -ge 2 ]] || { echo "ERROR: --install-dir needs a path" >&2; exit 1; }
        TARGET_DIR=$2; AUTO_YES=true; shift 2 ;;
      --tag)
        [[ $# -ge 2 ]] || { echo "ERROR: --tag needs a value" >&2; exit 1; }
        TAG=$2; shift 2 ;;
      --depth)
        [[ $# -ge 2 ]] || { echo "ERROR: --depth needs a value" >&2; exit 1; }
        DEPTH=$2
        if ! [[ "$DEPTH" =~ ^[1-9][0-9]*$ ]]; then
          echo "ERROR: --depth must be a positive integer" >&2
          exit 1
        fi
        shift 2 ;;
      -h|--help) usage; exit 0 ;;
      *)
        echo "Unknown option: $1" >&2
        usage; exit 1 ;;
    esac
  done
fi

# ---- prerequisites ----------------------------------------------------------
if ! command -v git >/dev/null 2>&1; then
  echo "ERROR: 'git' is not installed. Please install git and try again." >&2
  echo "  macOS            : brew install git"
  echo "  Debian/Ubuntu    : sudo apt-get install git"
  echo "  Windows (MSYS2)  : pacman -S git"
  exit 1
fi

# ---- confirm/install-dir ----------------------------------------------------
if [[ "$AUTO_YES" == false ]]; then
  printf "Install Umfeld into: '%s' ? [Y/n] ( waiting for 20sec )" "$TARGET_DIR"
  if read -r -t 20 confirm; then
    confirm=$(printf "%s" "${confirm:-}" | tr '[:upper:]' '[:lower:]')
  else
    confirm="y"
    echo    # move to next line after timeout
  fi

  if [[ "$confirm" =~ ^(n|no)$ ]]; then
    # -e (readline) if available on this bash
    if [[ ${BASH_VERSINFO[0]} -gt 4 || ( ${BASH_VERSINFO[0]} -eq 4 && ${BASH_VERSINFO[1]} -ge 4 ) ]]; then
      read -erp "Enter a different installation path: " custom_path
    else
      read -rp  "Enter a different installation path: " custom_path
    fi
    [[ -n "${custom_path:-}" ]] && TARGET_DIR=$custom_path
  fi
fi

# Create target dir if needed
if [[ ! -d "$TARGET_DIR" ]]; then
  echo "Creating directory: $TARGET_DIR"
  mkdir -p "$TARGET_DIR"
fi

# Abort if a non-empty 'umfeld' dir already exists
if [[ -d "$TARGET_DIR/umfeld" ]] && [[ -n "$(ls -A "$TARGET_DIR/umfeld" 2>/dev/null || true)" ]]; then
  echo "Warning: '$TARGET_DIR/umfeld' already exists and is not empty. Aborting." >&2
  exit 1
fi

cd "$TARGET_DIR"

echo "-------------------------------"
echo "--- cloning umfeld repositories"
echo "-------------------------------"

# ---- helpers ---------------------------------------------------------------
version_ge() {
  # returns 0 (true) if $1 >= $2 (semver-ish)
  [[ "$(printf '%s\n' "$1" "$2" | sort -V | head -n1)" == "$2" ]]
}

# Prepare common clone speedups (safe for old gits; unknown options are errors, hence guarded)
FILTER_OPTS=(--filter=blob:none)
SINGLE_BRANCH=("--single-branch")

# Prepare per-repo args
UMFELD_CLONE_ARGS=("${FILTER_OPTS[@]}" "${SINGLE_BRANCH[@]}")
EXAMPLES_CLONE_ARGS=("${FILTER_OPTS[@]}" "${SINGLE_BRANCH[@]}")
LIBRARIES_CLONE_ARGS=("${FILTER_OPTS[@]}" "--recurse-submodules")
ARDUINO_CLONE_ARGS=("${FILTER_OPTS[@]}" "${SINGLE_BRANCH[@]}")

# Apply tag/ref
if [[ -n "$TAG" ]]; then
  TAG_CLEAN="${TAG#v}"
  if version_ge "$TAG_CLEAN" "2.2.0"; then
    echo "Tag '$TAG' is >= v2.2.0 — applying to all repositories"
    UMFELD_CLONE_ARGS+=(--branch "$TAG")
    EXAMPLES_CLONE_ARGS+=(--branch "$TAG")
    LIBRARIES_CLONE_ARGS+=(--branch "$TAG")
    ARDUINO_CLONE_ARGS+=(--branch "$TAG")
  else
    echo "Tag '$TAG' is < v2.2.0 — applying only to 'umfeld'"
    UMFELD_CLONE_ARGS+=(--branch "$TAG")
  fi
fi

# Apply depth/shallow options
if [[ -n "$DEPTH" ]]; then
  echo "Using depth '$DEPTH' for all repositories"
  UMFELD_CLONE_ARGS+=(--depth "$DEPTH")
  EXAMPLES_CLONE_ARGS+=(--depth "$DEPTH")
  LIBRARIES_CLONE_ARGS+=(--depth "$DEPTH" --shallow-submodules)
  ARDUINO_CLONE_ARGS+=(--depth "$DEPTH")
fi

# Environment to avoid interactive prompts
export GIT_TERMINAL_PROMPT=0
export GIT_ASKPASS=echo

# If empty directories already exist (e.g., created earlier), remove them so clone can proceed
for d in umfeld umfeld-examples umfeld-libraries; do
  if [[ -d "$d" ]] && [[ -z "$(ls -A "$d" 2>/dev/null || true)" ]]; then
    rmdir "$d" || true
  fi
done

# ---- clone -----------------------------------------------------------------
git clone "${UMFELD_CLONE_ARGS[@]}"      https://github.com/dennisppaul/umfeld
git clone "${EXAMPLES_CLONE_ARGS[@]}"    https://github.com/dennisppaul/umfeld-examples
git clone "${LIBRARIES_CLONE_ARGS[@]}"   https://github.com/dennisppaul/umfeld-libraries
git clone "${ARDUINO_CLONE_ARGS[@]}"     https://github.com/dennisppaul/umfeld-arduino

echo "-------------------------------"
echo "--- download complete"
echo "-------------------------------"

# Optional: quick visibility check
echo "Installed into: $TARGET_DIR"
printf "Repos:\n  - %s\n  - %s\n  - %s\n" \
  "$TARGET_DIR/umfeld" "$TARGET_DIR/umfeld-examples" "$TARGET_DIR/umfeld-libraries"
  
echo "-------------------------------"
echo "--- test run example"
echo "-------------------------------"

RUN_DURATION=3
: "${CHECK:=✅}"
: "${CROSS:=❌}"
: "${REL_PATH:=umfeld-examples/Basics/load-image}"
: "${EXECUTABLE:=./build/load-image}"

cd $REL_PATH
cmake -B build ; cmake --build build


# start and capture PID (send stdout/stderr somewhere predictable; nohup defaults to nohup.out)
nohup "$EXECUTABLE" > /dev/null 2> /dev/tty &
# nohup "$EXECUTABLE" >> nohup.out 2>&1 &
PID=$!

# run for a limited time
sleep "$RUN_DURATION"

# terminate politely; ignore errors if already exited
kill "$PID" 2>/dev/null || true

# collect exit status of the process
wait "$PID"
EXIT_CODE=$?

# consider SIGTERM (143) as success as you already do
if [ "$EXIT_CODE" -eq 0 ] || [ "$EXIT_CODE" -eq 143 ]; then
  echo -e "--- $CHECK $REL_PATH test ran OK"
else
  echo -e "--- $CROSS $REL_PATH test failed: exit code $EXIT_CODE)"
fi