#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

pacman -Qqen > "$SCRIPT_DIR/packages/native.txt"
pacman -Qqem > "$SCRIPT_DIR/packages/aur.txt"

printf 'Updated %s\n' "$SCRIPT_DIR/packages/native.txt"
printf 'Updated %s\n' "$SCRIPT_DIR/packages/aur.txt"
