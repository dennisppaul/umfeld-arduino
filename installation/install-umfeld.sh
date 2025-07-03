#!/usr/bin/env bash

set -e

AUTO_YES=false
TARGET_DIR="$(pwd)"
TAG=""
DEPTH=""

# parse optional arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -y|--yes)
            AUTO_YES=true
            shift
            ;;
        --install-dir)
            TARGET_DIR="$2"
            AUTO_YES=true
            shift 2
            ;;
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
            echo "Usage: $0 [--tag <tag-name>] [--depth <depth>] [--install-dir <path>] [--yes|-y]"
            exit 1
            ;;
    esac
done

if [ "$AUTO_YES" = false ]; then
    echo "Do you want to install into the current folder: $TARGET_DIR ? [Y/n]"
    read -r confirm
    confirm=$(printf "%s" "$confirm" | tr '[:upper:]' '[:lower:]')
    if [[ "$confirm" =~ ^(n|no)$ ]]; then
        if [[ $BASH_VERSINFO -ge 4 && ${BASH_VERSINFO[1]} -ge 4 ]]; then
            read -erp "Enter a different installation path: " custom_path
        else
            read -rp "Enter a different installation path: " custom_path
        fi
        if [ -n "$custom_path" ]; then
            TARGET_DIR="$custom_path"
        fi
    fi
fi

# abort if the target directory already exists and is not empty
if [ -d "$TARGET_DIR/umfeld" ] && [ "$(ls -A "$TARGET_DIR/umfeld")" ]; then
    echo "Warning: '$TARGET_DIR/umfeld' already exists and is not empty. Aborting."
    exit 1
fi

if [ ! -d "$TARGET_DIR" ]; then
    echo "Creating directory: $TARGET_DIR"
    mkdir -p "$TARGET_DIR" || { echo "Failed to create directory: $TARGET_DIR"; exit 1; }
fi

cd "$TARGET_DIR" || { echo "Failed to change directory to $TARGET_DIR"; exit 1; }

echo "-------------------------------"
echo "--- cloning umfeld repositories"
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
echo "--- download complete"
echo "-------------------------------"