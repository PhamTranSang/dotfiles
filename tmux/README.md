# tmux guide

How this tmux setup works — install, concepts, and every keybinding configured
in [`tmux.conf`](./tmux.conf).

tmux is a **terminal multiplexer**: it splits one terminal window into multiple
panes/windows, and — its killer feature — keeps your sessions alive in the
background so you can detach, close the terminal, and reattach later with
everything still running (great for long builds and SSH).

---

## Install

```sh
sudo pacman -S tmux
```

The config lives at `~/.config/tmux/tmux.conf` (symlinked from this repo by
`install-dotfiles.sh`). The Tokyo Night status bar uses Nerd Font icons, so also
install `ttf-jetbrains-mono-nerd` — both are already tracked in
`packages/native.txt`.

Start it with `tmux`. The config loads automatically.

---

## The prefix key

Every tmux command starts with the **prefix**, then the key. Here the prefix is
remapped to **`Ctrl-a`** (easier than the default `Ctrl-b`).

> Notation below: **`Ctrl-a` `x`** means press `Ctrl-a`, release, then press `x`.
> Press `Ctrl-a` twice to send a literal `Ctrl-a` to the shell.

The mental model is a hierarchy:

```
session  ── a named workspace (survives detach)
└── window   ── like a tab
    └── pane     ── a split within the tab
```

---

## Sessions — the reason to use tmux

A session keeps running even after you detach or close the terminal.

```sh
tmux                 # start a new session
tmux new -s work     # start a named session "work"
tmux ls              # list running sessions
tmux attach          # reattach to the last session
tmux attach -t work  # reattach to "work"
tmux kill-session -t work
```

| Keys | Action |
| --- | --- |
| `Ctrl-a` `d` | **Detach** (session keeps running in the background) |
| `Ctrl-a` `s` | Switch between sessions (visual list) |
| `Ctrl-a` `$` | Rename the current session |
| `Ctrl-a` `Q` | Kill the whole session |

Typical flow: `tmux new -s dev` → work, run a server → `Ctrl-a d` to detach →
close the terminal → later `tmux attach -t dev`, the server is still running.

---

## Windows (tabs)

| Keys | Action |
| --- | --- |
| `Ctrl-a` `c` | New window (opens in the current directory) |
| `Ctrl-a` `n` / `p` | Next / previous window |
| `Ctrl-a` `1`…`9` | Jump to window by number |
| `Ctrl-a` `,` | Rename the current window |
| `Ctrl-a` `w` | Window picker (visual list) |
| `Ctrl-a` `X` | Close the current window (no confirmation) |

Windows are numbered from **1** and renumber automatically when one closes.

---

## Panes (splits)

| Keys | Action |
| --- | --- |
| `Ctrl-a` `\|` | Split **vertically** (side by side) |
| `Ctrl-a` `-` | Split **horizontally** (stacked) |
| `Ctrl-a` `h` `j` `k` `l` | Move to pane left / down / up / right (vim-style) |
| `Ctrl-a` `H` `J` `K` `L` | Resize pane (hold `Ctrl-a`, tap repeatedly) |
| `Ctrl-a` `z` | Zoom pane to fullscreen (toggle) |
| `Ctrl-a` `space` | Cycle through preset layouts |
| `Ctrl-a` `x` | Close the current pane (no confirmation) |

New splits open in the **current pane's directory**.

---

## Scrolling the history

Inside tmux the terminal's own scrollback is replaced by tmux's, so scroll with:

| Keys / action | Result |
| --- | --- |
| Mouse wheel | Scroll up/down (enters copy mode automatically) |
| `Shift+PageUp` / `Shift+PageDown` | Scroll a page up / down |
| `Ctrl-a` `Enter` | Enter copy mode, then `k`/`j` or arrows to scroll |
| `q` or `Escape` | Leave copy mode (back to the prompt) |

---

## Copy & paste

Copy goes straight into the **Wayland system clipboard** (via `wl-copy`), so it
works in every app — Firefox, editors, etc.

**Copy with the keyboard (vi keys):**

| Keys | Action |
| --- | --- |
| `Ctrl-a` `Enter` | Enter copy mode |
| `h` `j` `k` `l` / arrows | Move the cursor |
| `v` | Start selection |
| `Ctrl-v` | Toggle block (column) selection |
| `y` | Copy selection → system clipboard, exit |
| `Escape` | Cancel |

**Copy with the mouse:** drag to select, release — it copies automatically.

**Paste:**

| Keys | Pastes from |
| --- | --- |
| `Ctrl-Shift-V` (Alacritty) | System clipboard |
| Middle mouse click | Last selection |
| `Ctrl-a` `]` | tmux's own buffer |

> **Native selection:** to select with the mouse the normal way (ignoring
> tmux's pane boundaries — e.g. grabbing a URL), hold **`Shift`** while dragging.

---

## Reload the config

After editing `tmux.conf`:

```
Ctrl-a  r          # reload in a running tmux
```

(or `tmux source-file ~/.config/tmux/tmux.conf`)

---

## What's customized here

Everything else is stock tmux; this config changes:

- **Prefix** `Ctrl-b` → `Ctrl-a`
- **Splits** `|` and `-` (instead of `%` and `"`), opening in the current path
- **Vim-style** pane navigation (`h/j/k/l`) and resizing (`H/J/K/L`)
- **No-confirmation** close: `x` pane, `X` window, `Q` session
- **Mouse** on; **vi** keys in copy mode; copy piped to **`wl-copy`**
- Windows/panes indexed from **1**, auto-renumbered
- 50k-line scrollback, faster escape time, focus events, true color
- **Tokyo Night** status bar (bottom), matching the Alacritty theme

For the exact settings, read [`tmux.conf`](./tmux.conf) — it's commented.
