#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

install_tree() {
  local source_dir="$1"
  local target_dir="$2"

  if [[ ! -d "$source_dir" ]]; then
    return
  fi

  mkdir -p "$target_dir"
  cp -a "$source_dir/." "$target_dir/"
}

install_tree "$SCRIPT_DIR/.config" "$HOME/.config"
install_tree "$SCRIPT_DIR/.local" "$HOME/.local"

find "$HOME/.config/sway/scripts" -type f -exec chmod +x {} \; 2>/dev/null || true

printf 'Dotfiles installed into %s\n' "$HOME"
