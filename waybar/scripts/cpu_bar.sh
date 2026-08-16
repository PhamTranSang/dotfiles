#!/usr/bin/env bash
# CPU sparkline: do thi lich su WIDTH giay, width co dinh, gia tri o tooltip.
export LC_ALL=C.UTF-8
bars=(▁ ▂ ▃ ▄ ▅ ▆ ▇ █)          # 8 muc, index 0..7
NBSP=$' '
WIDTH=12
prev="${XDG_RUNTIME_DIR:-/tmp}/waybar_cpu.prev"
state="${XDG_RUNTIME_DIR:-/tmp}/waybar_cpu.state"

read -r _ u n s idle iow irq soft steal _ < /proc/stat
total=$((u+n+s+idle+iow+irq+soft+steal))
busy=$((total-idle-iow))
if [ -f "$prev" ]; then
    read -r ptotal pbusy < "$prev"
    dt=$((total-ptotal)); db=$((busy-pbusy))
    cpu=0; [ "$dt" -gt 0 ] && cpu=$(( db*100/dt ))
else
    cpu=0
fi
echo "$total $busy" > "$prev"
[ "$cpu" -lt 0 ] && cpu=0; [ "$cpu" -gt 100 ] && cpu=100

# them cot moi vao lich su (idx 0..7), giu WIDTH cot gan nhat
idx=$(( cpu * 7 / 100 )); [ "$idx" -gt 7 ] && idx=7
hist=""; [ -f "$state" ] && hist=$(cat "$state")
hist="${hist}${bars[$idx]}"
[ "${#hist}" -gt "$WIDTH" ] && hist="${hist: -WIDTH}"
printf '%s' "$hist" > "$state"
# pad-trai bang NBSP de width co dinh tu dau (luc lich su chua day)
while [ "${#hist}" -lt "$WIDTH" ]; do hist="${NBSP}${hist}"; done

class="normal"
[ "$cpu" -ge 60 ] && class="warning"
[ "$cpu" -ge 85 ] && class="critical"

mo="<span font_family='JetBrains Mono'>"; mc="</span>"
printf '{"text":" %s%s%s","tooltip":"CPU: %d%%","class":"%s"}\n' "$mo" "$hist" "$mc" "$cpu" "$class"
