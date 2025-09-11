#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

COMMON_TMP=$(mktemp)
curl -fsSL "https://raw.githubusercontent.com/${REPO:-dennisppaul/umfeld}/${UMFELD_REF:-main}/installation/common.sh" -o "$COMMON_TMP"
# shellcheck source=/dev/null
. "$COMMON_TMP"
rm -f "$COMMON_TMP"

# --- Helpers ------------------------------------------------------------------
log(){ printf "%s\n" "$*"; }

fetch() {
  # usage: fetch <url> <outfile>
  local url=$1 out=$2
  curl --fail --show-error --silent --location \
       --connect-timeout 10 --max-time 300 \
       --retry 3 --retry-delay 2 --retry-connrefused \
       -o "$out" "$url"
  [[ -s "$out" ]] || { echo "ERROR: empty download from $url" >&2; exit 1; }
}

detect_profile_file() {
  # prefer zsh (default on modern macOS); fallback to bash profile
  if [[ -n "${ZDOTDIR:-}" ]]; then
    echo "${ZDOTDIR}/.zshrc"
  elif [[ -n "${SHELL:-}" && "${SHELL##*/}" == "zsh" ]]; then
    echo "${HOME}/.zshrc"
  elif [[ -n "${SHELL:-}" && "${SHELL##*/}" == "bash" ]]; then
    echo "${HOME}/.bash_profile"
  else
    echo "${HOME}/.zshrc"
  fi
}

append_homebrew_env() {
  local marker="# >>> homebrew minimal shellenv >>>"
  local profile_file
  profile_file="$(detect_profile_file)"
  touch "$profile_file"

  local brew_prefix
  brew_prefix="$(/usr/bin/env brew --prefix 2>/dev/null)" || return

  if ! grep -qF "$marker" "$profile_file"; then
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

ensure_brew_in_shell() {
  # Ensure 'brew' works in *this* shell (Apple Silicon or Intel)
  if [[ -x /opt/homebrew/bin/brew ]]; then
    eval "$(/opt/homebrew/bin/brew shellenv)"
  elif [[ -x /usr/local/bin/brew ]]; then
    eval "$(/usr/local/bin/brew shellenv)"
  else
    echo "ERROR: Homebrew install completed but 'brew' not found in expected paths." >&2
    exit 1
  fi
}

install_brew() {
  # Xcode CLT is required; if missing, inform the user
  if ! xcode-select -p >/dev/null 2>&1; then
    log "NOTE: Command Line Tools not found. macOS may prompt to install them."
  fi
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
}

# --- Main ---------------------------------------------------------------------
TMP_BREWFILE="$(mktemp)"; trap 'rm -f "$TMP_BREWFILE"' EXIT

log "--- checking for Homebrew"
if command -v brew >/dev/null 2>&1; then
  log "Homebrew already installed."
else
  log "Homebrew not installed. Installing..."
  install_brew
fi

# make brew available in this shell and persist for future shells
ensure_brew_in_shell
append_homebrew_env
hash -r

log "--- updating Homebrew"
brew update

log "--- downloading Brewfile"
fetch "$BUNDLE_URL" "$TMP_BREWFILE"

log "--- installing packages from Brewfile"
# Brewfile is explicit; let bundle manage deps. It exits non-zero on failures.
brew bundle --file="$TMP_BREWFILE"

log "--- verifying key tools"
command -v cmake >/dev/null || { echo "ERROR: cmake missing after brew bundle"; exit 1; }
command -v pkg-config >/dev/null || { echo "ERROR: pkg-config missing after brew bundle"; exit 1; }
pkg-config --exists sdl3 || { echo "ERROR: SDL3 not found via pkg-config"; exit 1; }

log "--- setup complete."
log "If this is a new Homebrew install, make sure to follow the Homebrew install instructions or run:"
log
log "    source \"$(detect_profile_file)\""
