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

install_file() {
  local source_file="$1"
  local target_file="$2"

  if [[ ! -f "$source_file" ]]; then
    return
  fi

  mkdir -p "$(dirname "$target_file")"
  cp -a "$source_file" "$target_file"
}

config_dirs=(
  alacritty
  flameshot
  sway
  waybar
  wlogout
  wofi
)

for dir in "${config_dirs[@]}"; do
  install_tree "$SCRIPT_DIR/$dir" "$HOME/.config/$dir"
done

install_file "$SCRIPT_DIR/mimeapps.list" "$HOME/.config/mimeapps.list"
install_tree "$SCRIPT_DIR/applications" "$HOME/.local/share/applications"

find "$HOME/.config/sway/scripts" -type f -exec chmod +x {} \; 2>/dev/null || true

if [[ -f "$HOME/.config/sway/lock-screen/meson.build" ]]; then
  if command -v meson >/dev/null 2>&1 && command -v ninja >/dev/null 2>&1; then
    (
      cd "$HOME/.config/sway/lock-screen"
      if [[ ! -d build ]]; then
        meson setup build
      else
        meson setup build --reconfigure
      fi
      ninja -C build
    )
  else
    printf "Skipping Sway lock-screen module build: meson or ninja is missing.\n" >&2
  fi
fi

printf 'Dotfiles installed into %s\n' "$HOME"
