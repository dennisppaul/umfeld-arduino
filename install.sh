#!/bin/bash

set -e

echo "---------------------"
echo -n "+++ platform: "

case "$(uname -s)" in
    Darwin)
        echo "detected macOS"
        exec ./install-macOS.sh
        ;;
    Linux)
        if grep -qi raspberry /proc/device-tree/model 2>/dev/null; then
            echo "detected Raspberry Pi"
            exec ./install-RPI.sh
        else
            echo "detected Linux"
            exec ./install-linux.sh
        fi
        ;;
    MINGW* | MSYS* | CYGWIN*)
        if [[ "$MSYSTEM" == "UCRT64" ]]; then
            echo "detected windows (MSYS2 UCRT64)"
            exec ./install-windows-ucrt64.sh
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
