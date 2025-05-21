#!/bin/bash

set -e

BUNDLE_URL="https://raw.githubusercontent.com/dennisppaul/umfeld/refs/heads/main/Brewfile"

check_brew() {
    if command -v brew >/dev/null 2>&1; then
        echo "already installed."
    else
        echo -n "not installed. installing homebrew now..."
        install_brew
        # ensure brew is usable in this shell 
        if [ -f /opt/homebrew/bin/brew ]; then
            eval "$(/opt/homebrew/bin/brew shellenv)"
        elif [ -f /usr/local/bin/brew ]; then
            eval "$(/usr/local/bin/brew shellenv)"
        else
            echo "Homebrew installation failed or unknown path."
            exit 1
        fi
    fi
}

install_brew() {
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    if [ $? -eq 0 ]; then
        echo " OK"
    else
        echo " FAILED"
        exit 1
    fi
}

echo -n "+++ checking for homebrew: "
check_brew

echo "+++ installing packages"
brew update
curl -fsSL "$BUNDLE_URL" -o Brewfile
brew bundle --file=Brewfile
rm Brewfile

echo "+++ done"
