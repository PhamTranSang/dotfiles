#!/usr/bin/env bash
# Toggle wofi. Khi MO: warp con tro vao giua output dang focus (= giua panel,
# vi wofi dat location=center) va cho sway tu exec de wofi nhan keyboard focus.
pkill wofi && exit 0

read -r x y w h < <(swaymsg -t get_outputs \
    | jq -r '.[] | select(.focused) | "\(.rect.x) \(.rect.y) \(.rect.width) \(.rect.height)"')

# warp con tro vao tam output truoc, roi mo wofi ngay do
[ -n "$w" ] && swaymsg "seat seat0 cursor set $(( x + w/2 )) $(( y + h/2 ))" >/dev/null
swaymsg exec wofi >/dev/null
