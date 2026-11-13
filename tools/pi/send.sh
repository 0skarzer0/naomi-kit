#!/bin/bash
# send.sh IMAGE.bin.gz NAME LIVE_ADDR [WAIT]: copy an image to the Pi as /boot/roms/NAME.bin.gz, send it to the
# NetDIMM, wait WAIT seconds (default 30) and print the console (live.py log). Never kill a peek half way (an
# unanswered peek keeps the NetDIMM busy up to a minute: dimm_wait.sh first); never `pkill -f` a pattern that is in
# the command line itself (it kills the ssh's own shell).
. "$(dirname "$0")/pi_env.sh"
"$(dirname "$0")/dimm_wait.sh" || exit 1
pi_scp "$1" "$PI:/boot/roms/$2.bin.gz"
pi_ssh "md5sum /boot/roms/$2.bin.gz; cd /sbin/piforce && timeout 110 python3 -u dc2naomi_send.py $DIMM /boot/roms/$2.bin.gz > /tmp/send.log 2>&1; echo send rc=\$?; sleep ${4:-30}; NK_DIMM=$DIMM timeout 45 python3 -u /sbin/piforce/naomi-kit/live.py $3 log 2>&1 | tail -40"
