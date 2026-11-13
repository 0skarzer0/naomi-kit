# naomi-kit

What a native game on a **SEGA NAOMI / NAOMI 2 netbooted from a NetDIMM** needs besides
[KallistiOS](https://github.com/KallistiOS/KallistiOS) (built with `KOS_SUBARCH=naomi`): its files in the NetDIMM,
a console it can be read from on a cabinet with no serial cable, the cabinet's controls, live figures a Raspberry
can peek at while it runs, and the board's SRAM. Made for the ports of
[naomi-wheel-games](https://github.com/0skarzer0/naomi-wheel-games) (a Crazy Taxi upright: wheel, two pedals, a
two-position shifter), from the pieces Daytona USA's and OutRunners' NAOMI ports had each grown.

*En español:* librería común de los ports nativos para NAOMI del proyecto naomi-wheel-games (lectura de ficheros de
la NetDIMM, consola y pantalla de fallo, controles del mueble, datos en vivo para la Raspberry, SRAM). Estado y cómo
seguir: [HANDOFF.md](HANDOFF.md).

| Piece | What it does |
|---|---|
| `nk_boot` (`src/nk_boot.c`) | `main()`'s first call. The console into a ring of lines (shown on the screen on a failure, readable live from the Raspberry), `/dimm` mounted, the NetDIMM's peeks answered, a watchdog (no frame for a while: the console on the screen with the stuck thread's pc and pr), a sampling profiler, exceptions caught: misaligned loads and stores done a byte at a time (code translated from the V60, 68000 or i960 does them; the SH-4 faults), anything else written to the console while the peeks keep answering |
| Live block (`nk_live_head`) | The game's live figures start with a head that names them: `tools/pi/live.py` shows any game's words by name; `test_inputs` lets a script play with no one at the cabinet |
| `nk_cabinet_read` (`src/nk_cabinet.c`) | The JVS controls through the MIE as a wheel (-128..127 and 0..255), gas and brake (0..255 with dead ends), the shifter, START, SERVICE, TEST and coins (by the coin meters' difference). The ranges start at the values measured on a Crazy Taxi upright and widen by themselves. `nk_cabinet_start_coin(n)`: START as the coin button too (a press inserts a coin, the release is START) |
| `nk_dimmfs_*` (`src/nk_dimmfs.c`) | Read-only files appended to the netboot image by `tools/nk_pack.py`, read through the cartridge port and mounted at `/dimm` (`fopen("/dimm/…")`); `nk_dimmfs_cache` copies one into main RAM |
| `nk_sram_*` (`src/nk_sram.c`) | A save in the 32 KB of battery-backed SRAM, in the game's own region, with a magic and a checksum |
| `tools/nk_pack.py` | The netboot image: naomibintool's `.bin` with the `/dimm` archive after it |
| `tools/pi/` | On the netbooter Raspberry (WiPi): `live.py ADDR fps|log|words`, `watch.py` (the console as it comes, a poke first), `profile.py` (the profiler's pcs for `sh-elf-addr2line`), `drive.py` (a race with no one at the cabinet), `info.py`; from a computer on its LAN: `install.sh`, `send.sh`, `dimm_wait.sh` (`pi_env.sh`: the password is never stored) |

## Using it

```make
NAOMI_KIT := path/to/naomi-kit          # (a git submodule, say)
include $(NAOMI_KIT)/kit.mk
OBJS += $(NAOMI_KIT_OBJS)
KOS_CFLAGS += $(NAOMI_KIT_CFLAGS)
```

```c
#include "nk/naomi_kit.h"

static struct { nk_live_head head; uint32_t fps, frame_us; } live;    /* the game's words after the head */

int main(void)
{
    static const nk_config config = {
        .title = "MY GAME (NAOMI)", .live = &live.head, .game = "MYGM", .names = "fps frame_us", .count = 2,
        .need_dimm = true, .hang_first_s = 90, .hang_s = 10, .misaligned = true, .every_second = NULL,
    };
    nk_boot(&config);
    for (;;) {
        nk_cabinet cabinet;
        nk_cabinet_read(&cabinet);
        /* ... the game's frame ... */
        live.head.frames_alive++;
    }
}
```

`examples/minimal/` is a whole one (the controls on the screen). Build: `source /opt/toolchains/dc/kos-naomi/environ.sh;
make -C examples/minimal`, then `naomibintool build example.bin -t NAOMIKIT -d HOMEBREW -e example.elf` and
`tools/nk_pack.py` (or send the `.bin` as it is: with no archive, `need_dimm = false`).

On the Raspberry, after `tools/pi/install.sh`:

```sh
python3 /sbin/piforce/naomi-kit/live.py 0x8c...... fps 60    # the game's words and the kit's, once a second
python3 /sbin/piforce/naomi-kit/live.py 0x8c...... log       # the console
python3 /sbin/piforce/naomi-kit/drive.py 0x8c...... 100      # a race with no one at the cabinet
```

## The SRAM's regions

32 KB at 0x00200000, shared by every homebrew on the board. Taken today:

| Region | Whose |
|---|---|
| 0x0000-0x0FFF | Cannonball (OutRun) |
| 0x1000-0x5FFF | Daytona USA, the recompilation (branch `naomi`, parked) |
| 0x6000-0x7FFF | Daytona USA, the native port |

The SRAM is all taken: a new game that saves needs a region given back (the parked recompilation's is the candidate)
or a smaller one agreed here.

## Users

- [OutRunners](https://github.com/0skarzer0/outrunners-port/tree/naomi) (`ports/naomi/`, as a submodule).
- [Daytona USA native](https://github.com/0skarzer0/daytona-arcade-recomp/tree/naomi-native) has its own copies of the
  same pieces (`platform/dreamcast-native/naomi/`); moving it onto the kit is pending (HANDOFF.md).

## Hardware traps it already knows

- KallistiOS stops a thread whose stack pointer is below its stack's base: a game's own stacks (tasks, coroutines)
  must be inside the stack of the thread that runs them.
- The VRAM takes no byte writes.
- An unanswered NetDIMM peek keeps the NetDIMM busy for up to a minute (sends then fail): `tools/pi/dimm_wait.sh`.
- While the program reads megabytes through the cartridge port the peeks wait: the loop takes the lock back first.
- newlib's `memcmp` goes a byte at a time on the SH-4; libgcc's 64-bit division is slow (no instruction for it).

## Acknowledgements

- **DragonMinded**: [libnaomi](https://github.com/DragonMinded/libnaomi), whose cartridge-port read sequence and
  NetDIMM peek / poke protocol `nk_dimmfs.c` and `nk_dimmcomms.c` reimplement, and the netboot tools (`netdimm.py`)
  the Raspberry tools import.
- **The KallistiOS team and contributors**, with its NAOMI subarchitecture and `naomibintool`.
- **Daytona USA's native port** ([alphanu1/daytona-arcade-recomp](https://github.com/alphanu1/daytona-arcade-recomp),
  dfchil's native port, its NAOMI build in [0skarzer0/daytona-arcade-recomp](https://github.com/0skarzer0/daytona-arcade-recomp/tree/naomi-native)):
  `nk_dimmfs.c`, `nk_dimmcomms.c`, `nk_boot.c`'s console and watchdog, `nk_cabinet.c`'s ranges, `nk_sram.c` and
  `nk_pack.py` come from its `platform/dreamcast-native/naomi/`.
- **cheviman** (Sebastian Toyos), whose [outrunners-port](https://github.com/cheviman/outrunners-port) the
  OutRunners NAOMI build is on: from that build come the misaligned accesses, the profiler, the test inputs, the
  coins by the meters and START as the coin button.
- **[dc2naomi](https://github.com/0skarzer0/dc2naomi)**, where the NetDIMM's quirks were found (its resident's
  `dimm_service`).
- **The WiPi Netbooter / Piforce** image of the Raspberry, and **Flycast** for testing before the board.
- **Claude** (Anthropic), with which the code was written and debugged in sessions with the cabinet's owner, who tests
  every image on the board.

License: BSD-3-Clause ([LICENSE](LICENSE)). Not affiliated with or endorsed by SEGA.
