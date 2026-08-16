#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
IGNORE_FILE="$SCRIPT_DIR/packages/ignore.txt"

# Drop packages that a fresh Arch install already provides (base system,
# kernel, bootloader listed in packages/ignore.txt) and auto-generated
# *-debug packages, so the lists only track what we add on top.
filter() {
  grep -vE -- '-debug$' | grep -vxFf "$IGNORE_FILE"
}

pacman -Qqen | filter > "$SCRIPT_DIR/packages/native.txt"
pacman -Qqem | filter > "$SCRIPT_DIR/packages/aur.txt"

printf 'Updated %s\n' "$SCRIPT_DIR/packages/native.txt"
printf 'Updated %s\n' "$SCRIPT_DIR/packages/aur.txt"
