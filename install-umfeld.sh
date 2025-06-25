#!/usr/bin/env bash

set -e

echo "---------------------"
echo "+++ cloning umfeld repositories"

# Check if git is installed
if ! command -v git >/dev/null 2>&1; then
    echo "ERROR: 'git' is not installed. Please install git and try again."
    echo
    echo "on macOS            : 'brew install git'"
    echo "on Debian/Ubuntu    : 'sudo apt-get install git'"
    echo "on Windows (MSYS2)  : 'pacman -S git'"
    exit 1
fi

# Clone repositories
git clone https://github.com/dennisppaul/umfeld
git clone https://github.com/dennisppaul/umfeld-examples
git clone --recurse-submodules https://github.com/dennisppaul/umfeld-libraries.git

echo "+++ done"
