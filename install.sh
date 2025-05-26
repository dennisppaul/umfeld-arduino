#!/usr/bin/env bash

set -e

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
            exec /bin/bash -c "$(curl -fsSL ${BASE_URL}/install-raspberry-pi.sh)"
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
