#!/usr/bin/env bash

set -e

TAG=""
DEPTH=""

# parse optional arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --tag)
            TAG="$2"
            shift 2
            ;;
        --depth)
            DEPTH="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--tag <tag-name>] [--depth <depth>]"
            exit 1
            ;;
    esac
done

echo "-------------------------------"
echo "+++ cloning umfeld repositories"
echo "-------------------------------"

# check if git is installed

if ! command -v git >/dev/null 2>&1; then
    echo "ERROR: 'git' is not installed. Please install git and try again."
    echo
    echo "on macOS            : 'brew install git'"
    echo "on Debian/Ubuntu    : 'sudo apt-get install git'"
    echo "on Windows (MSYS2)  : 'pacman -S git'"
    exit 1
fi

# helper: compare semantic version numbers
version_ge() {
    # returns 0 (true) if $1 >= $2
    [ "$(printf '%s\n' "$1" "$2" | sort -V | head -n1)" = "$2" ]
}

# prepare clone arguments
UMFELD_CLONE_ARGS=()
EXAMPLES_CLONE_ARGS=()
LIBRARIES_CLONE_ARGS=(--recurse-submodules)

# only use tag for all repos if it's >= v2.2.0
if [[ -n "$TAG" ]]; then
    TAG_CLEAN="${TAG#v}"  # remove 'v' prefix
    if version_ge "$TAG_CLEAN" "2.2.0"; then
        echo "Tag '$TAG' is >= v2.2.0 — using for all repositories"
        UMFELD_CLONE_ARGS+=(--branch "$TAG")
        EXAMPLES_CLONE_ARGS+=(--branch "$TAG")
        LIBRARIES_CLONE_ARGS+=(--branch "$TAG")
    else
        echo "Tag '$TAG' is < v2.2.0 — using only for 'umfeld'"
        UMFELD_CLONE_ARGS+=(--branch "$TAG")
    fi
fi

if [[ -n "$DEPTH" ]]; then
    echo "Using depth '$DEPTH' for all repositories..."
    UMFELD_CLONE_ARGS+=(--depth "$DEPTH")
    EXAMPLES_CLONE_ARGS+=(--depth "$DEPTH")
    LIBRARIES_CLONE_ARGS+=(--depth "$DEPTH" --shallow-submodules)
fi

# clone repositories
git clone "${UMFELD_CLONE_ARGS[@]}" https://github.com/dennisppaul/umfeld
git clone "${EXAMPLES_CLONE_ARGS[@]}" https://github.com/dennisppaul/umfeld-examples
git clone "${LIBRARIES_CLONE_ARGS[@]}" https://github.com/dennisppaul/umfeld-libraries.git

echo "-------------------------------"
echo "+++ download complete"
echo "-------------------------------"