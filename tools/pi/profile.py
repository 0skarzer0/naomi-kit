#!/usr/bin/env python3
"""profile.py ADDR: wait for the profiler's ring to fill, then print its pcs as hex, one a line (for
`sh-elf-addr2line -f -e game.elf < pcs.txt | sort | uniq -c | sort -rn`)."""
import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from nk_live import Live

live = Live(os.environ.get('NK_DIMM', '10.0.0.4'), sys.argv[1])
pcs, size, start = live.head('profile_pcs'), live.head('profile_size'), live.head('profile_count')
while live.head('profile_count') - start < size:
    time.sleep(1)
print('# samples', live.head('profile_count'), file=sys.stderr)
for i in range(size):
    print('%08x' % live.peek(pcs + 4 * i))
