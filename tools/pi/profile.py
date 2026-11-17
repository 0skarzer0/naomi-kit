#!/usr/bin/env python3
"""profile.py ADDR [prs]: wait for the profiler's ring to fill, then print its pcs as hex, one a line (for
`sh-elf-addr2line -f -e game.elf < pcs.txt | sort | uniq -c | sort -rn`); with 'prs', "pc pr" a line (the
callers). Only the samples when the thread worked: the share it waited goes to stderr."""
import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from nk_live import Live

live = Live(os.environ.get('NK_DIMM', '10.0.0.4'), sys.argv[1])
pcs, size, start = live.head('profile_pcs'), live.head('profile_size'), live.head('profile_count')
prs = live.head('profile_prs') if len(sys.argv) > 2 and sys.argv[2] == 'prs' else 0
waiting = live.head('profile_waiting')
while live.head('profile_count') - start < size:
    time.sleep(1)
worked, waited = live.head('profile_count') - start, live.head('profile_waiting') - waiting
print('# samples %d worked, %d waited (%.0f %% idle)' % (worked, waited, 100.0 * waited / max(1, worked + waited)),
      file=sys.stderr)
for i in range(size):
    if prs:
        print('%08x %08x' % (live.peek(pcs + 4 * i), live.peek(prs + 4 * i)))
    else:
        print('%08x' % live.peek(pcs + 4 * i))
