#!/usr/bin/env bash
#
# Clean user + system junk on Arch Linux.
#
#   ./cleanup.sh            # interactive (pacman asks before removing)
#   ./cleanup.sh --yes      # non-interactive (assume yes)
#   KEEP_ORPHANS="meson rust cargo" ./cleanup.sh   # keep specific orphans
#
# Safe to re-run. Needs sudo for the system parts.
set -euo pipefail

# Orphan packages to KEEP even when nothing depends on them (build tools /
# toolchains you still want around). Override via the KEEP_ORPHANS env var.
read -r -a KEEP_ORPHANS <<< "${KEEP_ORPHANS:-meson rust}"

ASSUME_YES=0
[[ "${1:-}" == "--yes" || "${1:-}" == "-y" ]] && ASSUME_YES=1

log()  { printf '\n\033[1;34m==>\033[0m %s\n' "$*"; }
note() { printf '    %s\n' "$*"; }

pac_noconfirm() { ((ASSUME_YES)) && printf -- '--noconfirm'; }

# --- 1. User junk: leftover backup files (never touch the dotfiles repo) --------
clean_user_junk() {
  log "Removing leftover *.bak / *.bak-* files under ~/.config and ~/.local"
  local found
  found="$(find "$HOME/.config" "$HOME/.local" \
             \( -name '*.bak' -o -name '*.bak-*' \) \
             -not -path '*/dotfiles/*' 2>/dev/null || true)"
  if [[ -z "$found" ]]; then
    note "none found"
    return
  fi
  printf '%s\n' "$found"
  printf '%s\n' "$found" | xargs -r rm -f
}

# --- 2. Trash ------------------------------------------------------------------
clean_trash() {
  log "Emptying the trash"
  local trash="$HOME/.local/share/Trash"
  if [[ -d "$trash" ]] && [[ -n "$(ls -A "$trash" 2>/dev/null)" ]]; then
    note "before: $(du -sh "$trash" 2>/dev/null | cut -f1)"
    rm -rf "${trash:?}/"* 2>/dev/null || true
  else
    note "already empty"
  fi
}

# --- 3. Package cache: keep the last 2 versions, drop uninstalled ---------------
clean_pkg_cache() {
  log "Trimming pacman package cache"
  note "before: $(du -sh /var/cache/pacman/pkg 2>/dev/null | cut -f1)"
  if ! command -v paccache >/dev/null 2>&1; then
    note "installing pacman-contrib (provides paccache)"
    sudo pacman -S --needed $(pac_noconfirm) pacman-contrib
  fi
  sudo paccache -rk2      # keep 2 most recent versions of installed packages
  sudo paccache -ruk0     # remove every cached version of uninstalled packages
  note "after:  $(du -sh /var/cache/pacman/pkg 2>/dev/null | cut -f1)"
}

# --- 4. Core dumps: kept crash dumps, pure junk once you're done debugging ------
clean_coredumps() {
  log "Removing systemd core dumps"
  local dir=/var/lib/systemd/coredump
  if [[ -n "$(sudo sh -c "ls -A $dir 2>/dev/null")" ]]; then
    note "before: $(sudo du -sh "$dir" 2>/dev/null | cut -f1)"
    sudo find "$dir" -type f -delete
  else
    note "none found"
  fi
}

# --- 5. Orphan packages: installed as deps, now needed by nothing ---------------
clean_orphans() {
  log "Removing orphan packages"
  local orphans
  if ((${#KEEP_ORPHANS[@]})); then
    mapfile -t orphans < <(pacman -Qtdq 2>/dev/null \
      | grep -vxF -f <(printf '%s\n' "${KEEP_ORPHANS[@]}") || true)
  else
    mapfile -t orphans < <(pacman -Qtdq 2>/dev/null || true)
  fi

  if ((${#orphans[@]} == 0)); then
    note "none to remove"
    return
  fi
  note "removing: ${orphans[*]}"
  ((${#KEEP_ORPHANS[@]})) && note "keeping:  ${KEEP_ORPHANS[*]}"
  sudo pacman -Rns $(pac_noconfirm) "${orphans[@]}"
}

# --- 6. Systemd journal: cap the on-disk size -----------------------------------
clean_journal() {
  log "Vacuuming systemd journal to 200M"
  sudo journalctl --vacuum-size=200M
}

main() {
  clean_user_junk
  clean_trash
  clean_pkg_cache
  clean_coredumps
  clean_orphans
  clean_journal
  log "Cleanup done."
}

main "$@"
