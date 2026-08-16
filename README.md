# Dotfiles

Personal Arch Linux + Sway (Wayland) dotfiles.

Everything is organized as one folder per app at the repository root. There are
three helper scripts:

| Script | What it does |
| --- | --- |
| `bootstrap-arch.sh` | Installs packages and enables services on a fresh Arch box |
| `install-dotfiles.sh` | Places the config files into `~/.config` and builds the lock-screen module |
| `refresh-packages.sh` | Regenerates the package lists from what is currently installed |

---

## 1. Set up a fresh machine

Assumes Arch is already installed (e.g. via `archinstall`) and you are logged in
as your normal user with `sudo` access.

```sh
git clone <repo-url> ~/dev/dotfiles
cd ~/dev/dotfiles

./bootstrap-arch.sh      # install packages + enable services
./install-dotfiles.sh    # put configs in place + build the lock screen
```

Then reboot (or log out and back in). LightDM will start and you can pick the
Sway session.

**Install packages but don't touch systemd services:**

```sh
ENABLE_SERVICES=0 ./bootstrap-arch.sh
```

### What `bootstrap-arch.sh` installs

- Official packages from `packages/native.txt` (`pacman -Syu --needed`)
- Build tools for the lock screen: `meson`, `ninja`, `gcc`
- `yay` (built from source if missing), then AUR packages from `packages/aur.txt`
- Enables: NetworkManager, seatd, LightDM, Docker, ufw, and the `fstrim` timer

All installs use `--needed`, so anything already present is **skipped, never
re-installed or errored**. It is always safe to re-run.

### What `install-dotfiles.sh` does

- **Symlinks** each config folder to `~/.config/<app>/` (alacritty, sway,
  waybar, wlogout, wofi) so edits are live — see §2
- **Symlinks** the desktop entries in `applications/` into
  `~/.local/share/applications/`
- **Symlinks** `zsh/zshrc` to `~/.zshrc`
- **Copies** `flameshot/` and `mimeapps.list` (these get rewritten at runtime —
  see §2)
- Makes `sway/scripts/*` executable
- Builds the gtklock lock-screen module with meson + ninja
- Refreshes the font cache (`fc-cache`) and reloads Sway/Waybar if they are
  running, so changes apply immediately. **The shell is the exception** — a
  script can't re-source into your live shell, so run `exec zsh` (or open a new
  terminal) yourself.

> **Installing packages is a separate step.** `install-dotfiles.sh` only places
> config and reloads apps; it never installs packages (so it needs no sudo).
> Packages — fonts included — come from `bootstrap-arch.sh` reading
> `packages/*.txt`. To pull in a single new package now, just
> `sudo pacman -S <name>`.

It is safe to re-run: an already-correct symlink is left alone, and any real
file it needs to replace is moved to `<name>.bak-<timestamp>` first.

---

## 2. Editing configs day to day

`install-dotfiles.sh` symlinks the config folders in `~/.config` back into this
repo, so editing a config *is* editing the repo — no copying back and forth:

```
~/.config/sway      -> ~/dev/dotfiles/sway
~/.config/waybar    -> ~/dev/dotfiles/waybar
~/.config/wofi      -> ~/dev/dotfiles/wofi
~/.config/wlogout   -> ~/dev/dotfiles/wlogout
~/.config/alacritty -> ~/dev/dotfiles/alacritty
```

Typical loop:

```sh
# edit e.g. ~/.config/sway/config (same file as ~/dev/dotfiles/sway/config)
cd ~/dev/dotfiles
git add -A
git commit -m "sway: ..."
git push
```

> **`flameshot/` and `mimeapps.list` are copied, not symlinked.** Both get
> rewritten at runtime — flameshot rewrites `flameshot.ini` when you change a
> setting, and file managers / `xdg-mime` rewrite `mimeapps.list` when you change
> a default app. Symlinking them would leave the repo permanently "dirty" and
> could drop important values (e.g. flameshot's `useGrimAdapter`). Update them in
> the repo manually, then re-run `./install-dotfiles.sh`.

---

## 3. Keeping the package lists up to date

After installing or removing packages, regenerate the lists:

```sh
./refresh-packages.sh
```

This writes:

- `packages/native.txt` — explicitly installed official packages (`pacman -Qqen`)
- `packages/aur.txt` — explicitly installed AUR packages (`pacman -Qqem`)

Two kinds of entries are filtered out automatically:

- **`*-debug` packages** — noise from AUR debug builds.
- **Anything listed in `packages/ignore.txt`** — packages a fresh `archinstall`
  already provides (base system, kernel, bootloader, microcode). No point
  tracking them here.

Want to keep or drop a package from the tracked list? Edit
`packages/ignore.txt` and re-run `./refresh-packages.sh`. Removing something from
the tracked list never breaks a re-install — `bootstrap-arch.sh` uses `--needed`.

---

## 4. Cleaning up junk

```sh
./cleanup.sh            # interactive (pacman confirms before removing)
./cleanup.sh --yes      # don't ask
```

It removes, in order:

1. Leftover `*.bak` / `*.bak-*` files under `~/.config` and `~/.local` (never
   the repo itself).
2. The trash (`~/.local/share/Trash`).
3. Old packages in the pacman cache — keeps the 2 most recent versions of each
   installed package and drops everything for uninstalled ones (via `paccache`,
   installing `pacman-contrib` if needed).
4. Systemd core dumps (`/var/lib/systemd/coredump`).
5. Orphan packages (installed as dependencies, now needed by nothing). Build
   tools listed in `KEEP_ORPHANS` (default `meson rust`) are kept — override
   with e.g. `KEEP_ORPHANS="meson" ./cleanup.sh`.
6. Systemd journal logs down to 200 MB.

Safe to re-run any time.

## 5. Lock screen

Sway locks with `sway/scripts/fun-lock`, which launches `gtklock` using
`sway/lock-screen/config.ini`.

The lock screen embeds a Pacman mini-game, built as a gtklock C module. The game
plays on its own while locked; pressing any key reveals the unlock form and
pauses it.

`install-dotfiles.sh` builds the module for you. To rebuild by hand:

```sh
cd ~/.config/sway/lock-screen
meson setup build          # first time
meson setup build --reconfigure   # subsequent times
ninja -C build
```

The `build/` directory is a compiled artifact and is git-ignored.

---

## 6. Repository layout

| Repo path | Installed to | Purpose |
| --- | --- | --- |
| `sway/` | `~/.config/sway/` | Sway config, scripts, wallpapers, lock screen |
| `sway/lock-screen/` | `~/.config/sway/lock-screen/` | gtklock layout/style + Pacman game module source |
| `waybar/` | `~/.config/waybar/` | Waybar config, style, and helper scripts |
| `wofi/` | `~/.config/wofi/` | Wofi launcher config and style |
| `wlogout/` | `~/.config/wlogout/` | Power menu layout and style |
| `alacritty/` | `~/.config/alacritty/` | Terminal config |
| `fontconfig/` | `~/.config/fontconfig/` | Font preferences (Noto + emoji fallback) |
| `tmux/` | `~/.config/tmux/` | tmux config + usage guide ([`tmux/README.md`](tmux/README.md)) |
| `zsh/zshrc` | `~/.zshrc` | Zsh config (oh-my-zsh + zsh-autosuggestions) |
| `flameshot/` | `~/.config/flameshot/` | Screenshot tool config (copy only, see §2) |
| `applications/` | `~/.local/share/applications/` | Desktop entries and launcher overrides |
| `mimeapps.list` | `~/.config/mimeapps.list` | Default application associations |
| `packages/native.txt` | — | Official Arch packages to install |
| `packages/aur.txt` | — | AUR packages to install |
| `packages/ignore.txt` | — | Packages excluded from the tracked lists |

---

## Quick reference

| I want to… | Run |
| --- | --- |
| Set up a brand new machine | `./bootstrap-arch.sh && ./install-dotfiles.sh` |
| Install packages, skip services | `ENABLE_SERVICES=0 ./bootstrap-arch.sh` |
| Save a config change | edit → `git add -A && git commit && git push` |
| Update package lists | `./refresh-packages.sh` |
| Clean junk (cache, orphans, logs) | `./cleanup.sh` |
| Rebuild the lock screen | `ninja -C ~/.config/sway/lock-screen/build` |
