#!/usr/bin/env python3
"""live.py ADDR [fps [SECONDS] | log | words]: any naomi-kit game's live figures through the NetDIMM.

On the netbooter Raspberry, while the game runs. ADDR: the console's line "naomi-kit: ... live figures at 0x8c......"
or `sh-elf-nm game.elf | grep <the game's live block>`. NK_DIMM sets the NetDIMM's address (default 10.0.0.4).

  fps [N]  a line a second for N seconds (default 30): the game's own words, then the kit's
  log      the console's ring
  words    every word of the block once, by name
"""
import os
import sys
import time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from nk_live import Live


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    live = Live(os.environ.get('NK_DIMM', '10.0.0.4'), sys.argv[1])
    what = sys.argv[2] if len(sys.argv) > 2 else 'fps'
    if what == 'log':
        for _, line in live.log_lines():
            if line:
                print(line)
    elif what == 'words':
        version = live.head('version')
        print('version %08x%s  game %s' % (version, '  (AN EXCEPTION: %03x)' % (version & 0xffff) if version >> 16 == 0xdead
              else '', live.head('game').to_bytes(4, 'little').decode('ascii', 'replace')))
        for name in ('seconds', 'dimm_requests', 'heap_left', 'misaligned', 'profile_count', 'inputs', 'frames_alive'):
            print('%-14s %d' % (name, live.head(name)))
        for name, value in live.game().items():
            print('%-14s %d' % (name, value))
    else:
        seconds = int(sys.argv[3]) if len(sys.argv) > 3 else 30
        last = None
        for _ in range(seconds * 4):
            s = live.head('seconds')
            if s != last:
                last = s
                i = live.head('inputs')
                words = '  '.join('%s %d' % item for item in live.game().items())
                print('%5ds  %s  | misaligned %d  heap %d KB  wheel %02x gas %02x brake %02x buttons %02x' % (
                    s, words, live.head('misaligned'), live.head('heap_left') // 1024, i & 255, i >> 8 & 255,
                    i >> 16 & 255, i >> 24), flush=True)
            time.sleep(0.25)


if __name__ == '__main__':
    main()
