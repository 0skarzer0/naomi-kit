#!/bin/bash
# install.sh: naomi-kit's Raspberry tools into /sbin/piforce/naomi-kit on the cabinet's Pi (pi_env.sh: PI, PI_PASS)
. "$(dirname "$0")/pi_env.sh"
pi_ssh "mkdir -p /sbin/piforce/naomi-kit"
pi_scp "$(dirname "$0")"/*.py "$PI:/sbin/piforce/naomi-kit/"
echo "on the Pi: python3 /sbin/piforce/naomi-kit/live.py 0x8c...... fps"
