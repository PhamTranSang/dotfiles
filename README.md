# Dotfiles

Personal Arch Linux + Sway dotfiles.

## Install On A Fresh Arch System

```sh
git clone <repo-url> ~/dev/dotfiles
cd ~/dev/dotfiles
bash bootstrap-arch.sh
bash install-dotfiles.sh
```

Skip service enablement:

```sh
ENABLE_SERVICES=0 bash bootstrap-arch.sh
```

Refresh package lists after package changes:

```sh
bash refresh-packages.sh
```

## Repository Layout

This repo keeps app config folders at the top level. `install-dotfiles.sh` maps each folder to its real XDG destination instead of copying the whole repo.

| Repo path | Installed to | Purpose |
| --- | --- | --- |
| `sway/` | `~/.config/sway/` | Sway config, scripts, wallpapers, gtklock Pacman lock screen |
| `sway/lock-screen/` | `~/.config/sway/lock-screen/` | gtklock layout/style and Pacman auto-game module source |
| `waybar/` | `~/.config/waybar/` | Waybar config and style |
| `wofi/` | `~/.config/wofi/` | Wofi launcher config and style |
| `wlogout/` | `~/.config/wlogout/` | Power menu layout and style |
| `alacritty/` | `~/.config/alacritty/` | Terminal config |
| `flameshot/` | `~/.config/flameshot/` | Screenshot tool config |
| `applications/` | `~/.local/share/applications/` | User desktop entries and hidden launcher overrides |
| `mimeapps.list` | `~/.config/mimeapps.list` | Default application associations |
| `packages/native.txt` | n/a | Official Arch packages, including gtklock and lock-screen build dependencies |
| `packages/aur.txt` | n/a | AUR packages |

## Lock Screen

Sway uses `sway/scripts/fun-lock` as the lock command. The script runs `gtklock` with the config in `sway/lock-screen/config.ini`.

The lock screen contains a Pacman mini-game implemented as a gtklock C module. The game runs automatically while the session is locked. Pressing any key shows the unlock form and pauses the game.

`install-dotfiles.sh` builds the module after copying files:

```sh
cd ~/.config/sway/lock-screen
meson setup build   # first run
ninja -C build
```

On later installs it runs `meson setup build --reconfigure` and rebuilds with `ninja -C build`.

## Package Install

`bootstrap-arch.sh` installs packages from:

- `packages/native.txt` with `pacman`
- `packages/aur.txt` with `yay`

Disable service enablement with:

```sh
ENABLE_SERVICES=0 bash bootstrap-arch.sh
```
