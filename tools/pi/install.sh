#!/bin/bash
# install.sh: naomi-kit's Raspberry tools into /sbin/piforce/naomi-kit on the cabinet's Pi (pi_env.sh: PI, PI_PASS)
. "$(dirname "$0")/pi_env.sh"
pi_ssh "mkdir -p /sbin/piforce/naomi-kit"
pi_scp "$(dirname "$0")"/*.py "$PI:/sbin/piforce/naomi-kit/"
# live.py is the one reader of every project, kept in naomi-pi-tools (it reads the kit's "NKLV" block and the others):
# from a checkout next to this one, or from GitHub
LIVE="$(dirname "$0")/../../../naomi-pi-tools/pi/live.py"
if [ ! -f "$LIVE" ]; then
    LIVE=$(mktemp --suffix=.py)
    curl -fsSL https://raw.githubusercontent.com/0skarzer0/naomi-pi-tools/main/pi/live.py -o "$LIVE" || exit 1
fi
pi_scp "$LIVE" "$PI:/sbin/piforce/naomi-kit/live.py"
echo "on the Pi: python3 /sbin/piforce/naomi-kit/live.py 0x8c...... fps"
