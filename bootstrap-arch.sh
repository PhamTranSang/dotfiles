#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
NATIVE_PACKAGES="$SCRIPT_DIR/packages/native.txt"
AUR_PACKAGES="$SCRIPT_DIR/packages/aur.txt"

log() {
  printf '\n==> %s\n' "$*"
}

require_arch() {
  if [[ ! -f /etc/arch-release ]]; then
    printf 'This bootstrap script is intended for Arch Linux.\n' >&2
    exit 1
  fi
}

require_files() {
  if [[ ! -f "$NATIVE_PACKAGES" || ! -f "$AUR_PACKAGES" ]]; then
    printf 'Package list files are missing under %s/packages.\n' "$SCRIPT_DIR" >&2
    exit 1
  fi
}

install_native_packages() {
  log "Installing official repository packages"
  sudo pacman -Syu --needed - < "$NATIVE_PACKAGES"
}

install_yay_if_missing() {
  if command -v yay >/dev/null 2>&1; then
    return
  fi

  log "Installing yay from AUR"
  tmpdir="$(mktemp -d)"
  trap 'rm -rf "$tmpdir"' EXIT

  git clone https://aur.archlinux.org/yay.git "$tmpdir/yay"
  (
    cd "$tmpdir/yay"
    makepkg -si --needed --noconfirm
  )
}

install_aur_packages() {
  log "Installing AUR packages"
  mapfile -t packages < <(grep -vE '^(yay|yay-debug)$' "$AUR_PACKAGES")
  if (( ${#packages[@]} > 0 )); then
    yay -S --needed "${packages[@]}"
  fi
}

enable_services() {
  log "Enabling common services"
  sudo systemctl enable NetworkManager.service
  sudo systemctl enable seatd.service
  sudo systemctl enable lightdm.service
  sudo systemctl enable docker.service
  sudo systemctl enable ufw.service
  sudo systemctl enable fstrim.timer
}

main() {
  require_arch
  require_files
  install_native_packages
  install_yay_if_missing
  install_aur_packages

  if [[ "${ENABLE_SERVICES:-1}" == "1" ]]; then
    enable_services
  fi

  log "Done. Run ./install-dotfiles.sh, then reboot or log out/in before starting Sway."
}

main "$@"
