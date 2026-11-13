#!/usr/bin/env python3
"""watch.py ADDR [POKE_ADDR VALUE]: (a poke first, if given), then the console's new lines as they come, until the
peeks stop answering or 60 s pass: the line before a crash is the last thing the program said."""
import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from nk_live import Live

live = Live(os.environ.get('NK_DIMM', '10.0.0.4'), sys.argv[1])
t0 = time.time()
seen = live.peek(live.head('log_next')) - 1
if len(sys.argv) > 3:
    live.poke(int(sys.argv[2], 0), int(sys.argv[3], 0))
    print('poked', flush=True)
while time.time() - t0 < 60:
    for index, line in live.log_lines(seen + 1):
        print('%6.1f | %s' % (time.time() - t0, line), flush=True)
        seen = index
    time.sleep(0.2)
