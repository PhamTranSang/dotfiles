#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

# Symlink a repo folder to its XDG location so edits in ~/.config edit the repo
# directly. An existing real file/dir is moved aside to <target>.bak-<ts> first;
# an already-correct symlink is left untouched.
link_path() {
  local source="$1"
  local target="$2"

  if [[ ! -e "$source" ]]; then
    return
  fi

  # Already the right symlink? Nothing to do.
  if [[ -L "$target" && "$(readlink -f "$target")" == "$(readlink -f "$source")" ]]; then
    return
  fi

  mkdir -p "$(dirname "$target")"

  if [[ -e "$target" || -L "$target" ]]; then
    if [[ -L "$target" ]]; then
      rm -f "$target"
    else
      local backup="${target}.bak-$(date +%Y%m%d%H%M%S)"
      printf 'Backing up existing %s -> %s\n' "$target" "$backup" >&2
      mv "$target" "$backup"
    fi
  fi

  ln -s "$source" "$target"
}

# Copy (not symlink) — for apps that rewrite their own config at runtime.
copy_tree() {
  local source_dir="$1"
  local target_dir="$2"

  if [[ ! -d "$source_dir" ]]; then
    return
  fi

  mkdir -p "$target_dir"
  cp -a "$source_dir/." "$target_dir/"
}

copy_file() {
  local source_file="$1"
  local target_file="$2"

  if [[ ! -f "$source_file" ]]; then
    return
  fi

  mkdir -p "$(dirname "$target_file")"
  cp -a "$source_file" "$target_file"
}

# Config folders symlinked into ~/.config/<app>.
symlink_dirs=(
  alacritty
  fontconfig
  sway
  tmux
  waybar
  wlogout
  wofi
)

for dir in "${symlink_dirs[@]}"; do
  link_path "$SCRIPT_DIR/$dir" "$HOME/.config/$dir"
done

# flameshot rewrites flameshot.ini at runtime, so keep it a plain copy to avoid
# polluting the repo. Update it manually when you change a setting.
copy_tree "$SCRIPT_DIR/flameshot" "$HOME/.config/flameshot"

# mimeapps.list is rewritten by file managers / xdg-mime when you change default
# apps, so keep it a copy (same reasoning as flameshot). Desktop entries below
# are static, so they get symlinked.
copy_file "$SCRIPT_DIR/mimeapps.list" "$HOME/.config/mimeapps.list"

mkdir -p "$HOME/.local/share/applications"
for entry in "$SCRIPT_DIR/applications"/*; do
  [[ -e "$entry" ]] || continue
  link_path "$entry" "$HOME/.local/share/applications/$(basename "$entry")"
done

# Home-level dotfiles.
link_path "$SCRIPT_DIR/zsh/zshrc" "$HOME/.zshrc"

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

# --- Apply changes to the running system --------------------------------------
# Refresh the font cache so fontconfig changes take effect.
command -v fc-cache >/dev/null 2>&1 && fc-cache -f >/dev/null 2>&1 || true

# Reload Sway (re-reads its config) if we're inside a running session.
if command -v swaymsg >/dev/null 2>&1 && swaymsg -t get_version >/dev/null 2>&1; then
  swaymsg reload >/dev/null 2>&1 || true
fi

# Reload Waybar (SIGUSR2 = reload) if it is running.
if command -v killall >/dev/null 2>&1 && pgrep -x waybar >/dev/null 2>&1; then
  killall -SIGUSR2 waybar 2>/dev/null || true
fi

printf 'Dotfiles symlinked into %s\n' "$HOME"
# The shell config can't be reloaded from here — a script runs in a child
# process, so it can't re-source into your current shell. Do it yourself:
printf 'To reload your shell config, run: exec zsh  (or open a new terminal)\n'
