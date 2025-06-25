#!/bin/bash

set -e

BUNDLE_URL="https://raw.githubusercontent.com/dennisppaul/umfeld/refs/heads/main/Brewfile"

check_brew() {
    if command -v brew >/dev/null 2>&1; then
        echo "already installed."
    else
        echo "not installed. installing homebrew now..."
        install_brew
    fi

    # Ensure brew is usable in this shell
    if [ -x /opt/homebrew/bin/brew ]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    elif [ -x /usr/local/bin/brew ]; then
        eval "$(/usr/local/bin/brew shellenv)"
    else
        echo "Homebrew installation failed or brew not found in expected paths."
        exit 1
    fi

    append_homebrew_env
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

append_homebrew_env() {
    local profile_file="$HOME/.zshrc"
    local marker="# >>> homebrew minimal shellenv >>>"

    local brew_prefix
    brew_prefix="$(/usr/bin/env brew --prefix 2>/dev/null)" || return

    if ! grep -q "$marker" "$profile_file"; then
        cat >> "$profile_file" <<EOF
$marker
export HOMEBREW_PREFIX="$brew_prefix"
export HOMEBREW_CELLAR="\$HOMEBREW_PREFIX/Cellar"
export HOMEBREW_REPOSITORY="\$HOMEBREW_PREFIX"

case ":\$PATH:" in
  *":\$HOMEBREW_PREFIX/bin:"*) ;;
  *) export PATH="\$HOMEBREW_PREFIX/bin:\$HOMEBREW_PREFIX/sbin:\$PATH" ;;
esac

export INFOPATH="\$HOMEBREW_PREFIX/share/info:\$INFOPATH"
# <<< homebrew minimal shellenv <<<
EOF
    fi
}

echo -n "+++ checking for homebrew: "
check_brew

echo "+++ installing packages"
brew update
curl -sS -fsSL "$BUNDLE_URL" -o Brewfile
brew bundle --file=Brewfile

echo "+++ setup complete. please restart your terminal or run 'source ~/.zshrc' to apply environment changes."