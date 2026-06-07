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

## Layout

- `.config/sway`: Sway config, scripts, generated wallpapers
- `.config/waybar`: Waybar config and style
- `.config/wofi`: Wofi launcher config and style
- `.config/wlogout`: Power menu layout and style
- `.config/alacritty`: Terminal config
- `.config/flameshot`: Flameshot config
- `.local/share/applications`: User desktop entries and hidden launcher overrides
- `packages/native.txt`: official Arch packages
- `packages/aur.txt`: AUR packages
