#!/usr/bin/env bash

set -e

# Check if curl is installed
if ! command -v curl >/dev/null 2>&1; then
    echo "ERROR: 'curl' is not installed. Please install curl and try again."
    echo 
    echo "on Linux           : 'sudo apt-get install curl'"
    echo "on Windows (UCRT64): 'sudo pacman -Syu curl'"
    exit 1
fi

BASE_URL="https://raw.githubusercontent.com/dennisppaul/umfeld/main"

echo "---------------------"
echo -n "+++ platform: "

case "$(uname -s)" in
    Darwin)
        echo "detected macOS"
        exec /bin/bash -c "$(curl -fsSL ${BASE_URL}/install-macOS.sh)"
        ;;
    Linux)
        if grep -qi raspberry /proc/device-tree/model 2>/dev/null; then
            echo "detected Raspberry Pi"
            curl -fsSL ${BASE_URL}/install-raspberry-pi.sh | bash
        else
            echo "detected Linux"
            exec /bin/bash -c "$(curl -fsSL ${BASE_URL}/install-linux.sh)"
        fi
        ;;
    MINGW* | MSYS* | CYGWIN*)
        if [[ "$MSYSTEM" == "UCRT64" ]]; then
            echo "detected windows (MSYS2 UCRT64)"
            exec /bin/bash -c "$(curl -fsSL ${BASE_URL}/install-windows-ucrt64.sh)"
        else
            echo "unsupported windows environment. Please use MSYS2 UCRT64."
            exit 1
        fi
        ;;
    *)
        echo "unsupported platform: $(uname -s)"
        exit 1
        ;;
esac
