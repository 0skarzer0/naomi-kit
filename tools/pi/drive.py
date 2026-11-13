#!/usr/bin/env python3
"""drive.py ADDR [SECONDS]: play with no one at the cabinet, through the live block's test_inputs: a coin, START a
few times (through the screens before a race), then full gas with the wheel swinging a little; the game's words once
a second. At the end, the cabinet's own controls again."""
import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from nk_live import Live

live = Live(os.environ.get('NK_DIMM', '10.0.0.4'), sys.argv[1])
seconds = int(sys.argv[2]) if len(sys.argv) > 2 else 60
ON = 0x80000000
live.set_head('test_inputs', ON | 1)        # a coin
time.sleep(1.5)
for _ in range(12):                         # START through the screens
    live.set_head('test_inputs', ON | 2); time.sleep(0.3)
    live.set_head('test_inputs', ON); time.sleep(0.7)
start, last = time.time(), None
try:
    while time.time() - start < seconds:
        t = time.time() - start
        wheel = 0x80 + int(40 * ((t // 3) % 3 - 1))
        live.set_head('test_inputs', ON | 0xff << 8 | wheel << 16)
        s = live.head('seconds')
        if s != last:
            last = s
            print('%4ds  %s' % (s, '  '.join('%s %d' % item for item in live.game().items())), flush=True)
        time.sleep(0.25)
finally:
    live.set_head('test_inputs', 0)
