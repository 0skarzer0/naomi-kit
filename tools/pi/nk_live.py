"""nk_live.py: naomi-kit's live block read through the NetDIMM, for the other tools here (and yours).

Runs on the netbooter Raspberry (WiPi), with DragonMinded's netdimm.py in /sbin/piforce/dm_netboot. The block starts
with naomi_kit.h's nk_live_head; the game's own words follow, named by the string the head points at."""
import struct
import sys
sys.path.append('/sbin/piforce/dm_netboot')
from netdimm import NetDimm, PeekPokeTypeEnum as T

MAGIC = 0x564c4b4e  # "NKLV"
HEAD = ('magic version game names count seconds dimm_requests log_lines log_count log_width log_next heap_left '
        'misaligned profile_pcs profile_count profile_size test_inputs inputs frames_alive profile_prs '
        'profile_waiting').split()
HEAD_WORDS = 24     # 0x60 bytes
FIELD = {name: 4 * index for index, name in enumerate(HEAD)}


class Live:
    def __init__(self, dimm_ip, address):
        self.dimm, self.base = NetDimm(dimm_ip), int(address, 0) if isinstance(address, str) else address
        if self.word(0) != MAGIC:
            sys.exit('no naomi-kit live block at 0x%08x (magic %08x): wrong address, or the game is not running'
                     % (self.base, self.word(0)))
        count, names = self.word(FIELD['count']), self.word(FIELD['names'])
        self.names = self.string(names).split()[:count] if names else []

    def peek(self, address):
        return self.dimm.peek(address & 0x01ffffff, T.TYPE_LONG)    # firmware 3.17: an offset into the 32 MB of RAM

    def poke(self, address, value):
        self.dimm.poke(address & 0x01ffffff, T.TYPE_LONG, value)

    def word(self, offset):
        return self.peek(self.base + offset)

    def head(self, name):
        return self.word(FIELD[name])

    def set_head(self, name, value):
        self.poke(self.base + FIELD[name], value)

    def game_offset(self, name):
        return 4 * HEAD_WORDS + 4 * self.names.index(name)

    def game(self):
        """The game's words, by name."""
        return {name: self.word(self.game_offset(name)) for name in self.names}

    def string(self, address, limit=1024):
        out = b''
        while len(out) < limit:
            out += struct.pack('<I', self.peek(address + len(out)))
            if b'\0' in out:
                break
        return out.split(b'\0', 1)[0].decode('ascii', 'replace')

    def log_lines(self, first=None):
        """The console's lines (index, text) from 'first' on (default: all the ring holds)."""
        n, width, written = self.head('log_count'), self.head('log_width'), self.peek(self.head('log_next'))
        start = max(0, written - n + 1) if first is None else max(first, written - n + 1)
        for index in range(start, written):
            at = self.head('log_lines') + (index % n) * width
            raw = b''.join(struct.pack('<I', self.peek(at + 4 * k)) for k in range(width // 4))
            yield index, raw.split(b'\0', 1)[0].decode('ascii', 'replace')
