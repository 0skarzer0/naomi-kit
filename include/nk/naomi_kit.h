/**
 *    Name:        naomi_kit.h
 *    Description: naomi-kit: what a native game on a SEGA NAOMI / NAOMI 2 netbooted from a NetDIMM needs besides
 *                 KallistiOS (KOS_SUBARCH=naomi): its files in the NetDIMM, a console it can be read from, the
 *                 cabinet's controls, live figures a Raspberry can peek at, and the board's SRAM
 *    Created:     13-11-2026
 *
 *    license:BSD-3-Clause
 *    Gathered from two ports that grew the same pieces: Daytona USA's native port (0skarzer0/daytona-arcade-recomp,
 *    branch naomi-native, platform/dreamcast-native/naomi) and OutRunners' (0skarzer0/outrunners-port, branch
 *    naomi, ports/naomi). README.md says what each piece is and how a game uses it.
 *
 *      nk_boot          main()'s first call: the console into a ring of lines, /dimm mounted, the NetDIMM's peeks
 *                       answered, a watchdog, a sampling profiler, exceptions caught (misaligned accesses done a
 *                       byte at a time, as the V60 and the 68000's code expects)
 *      nk_live_head     the live block's head: naomi-pi-tools' pi/live.py reads any game's figures by its field names
 *      nk_cabinet_read  the JVS controls as a wheel, two pedals, a two-position shifter, START, coins
 *      nk_dimmfs_*      read-only files appended to the netboot image (tools/nk_pack.py), mounted at /dimm
 *      nk_sram_*        a save in the board's 32 KB of battery-backed SRAM, in a region of the game's own
 */
#ifndef NAOMI_KIT_H
#define NAOMI_KIT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NK_VERSION 1

/* ---- the live block ----

   A game keeps its live figures in a struct of 32-bit words that starts with an nk_live_head; the head says where
   the names of the game's own words are, so that naomi-pi-tools' pi/live.py shows any game's figures. Its address: the
   console's line "naomi-kit: live figures at 0x8c......", or `sh-elf-nm game.elf | grep <its name>`. Words, little
   endian; the Raspberry reads them with DragonMinded's netdimm.py (NetDimm(ip).peek). */

#define NK_LIVE_MAGIC 0x564c4b4eu    /* "NKLV" */

typedef struct
{
	uint32_t magic;            /* +0x00 NK_LIVE_MAGIC */
	uint32_t version;          /* +0x04 NK_VERSION; 0xdeadXXXX after an exception nothing handles (XXXX its code) */
	uint32_t game;             /* +0x08 the game's four letters, little endian ("ORUN") */
	uint32_t names;            /* +0x0c address of the game's words' names: one string, the names apart by spaces */
	uint32_t count;            /* +0x10 the game's words after the head */
	uint32_t seconds;          /* +0x14 seconds counted (nk_config.every_second runs once each) */
	uint32_t dimm_requests;    /* +0x18 peeks and pokes answered */
	uint32_t log_lines;        /* +0x1c address of the console ring: log_count lines of log_width bytes */
	uint32_t log_count;        /* +0x20 */
	uint32_t log_width;        /* +0x24 */
	uint32_t log_next;         /* +0x28 address of the number of lines written so far (index % log_count) */
	uint32_t heap_left;        /* +0x2c bytes above the heap's top, once a second */
	uint32_t misaligned;       /* +0x30 misaligned loads and stores done a byte at a time */
	uint32_t profile_pcs;      /* +0x34 address of profile_size words: the watched thread's pc, sampled every ms while
	                              it runs or is ready (not while it waits: those are counted in profile_waiting) */
	uint32_t profile_count;    /* +0x38 samples taken (the ring's next index is this modulo profile_size) */
	uint32_t profile_size;     /* +0x3c */
	uint32_t test_inputs;      /* +0x40 pokeable, to play with no one at the cabinet (nk_cabinet_read): bit 31 on;
	                              bit 0 a coin (cleared once taken), 1 START, 2 shifter forward, 3 back, 4 SERVICE,
	                              8-15 gas, 16-23 the wheel (0x80 the middle), 24-31 brake (0 = the cabinet's own) */
	uint32_t inputs;           /* +0x44 what nk_cabinet_read read last: wheel | gas << 8 | brake << 16 | buttons << 24
	                              (bit 24 START, 25 forward, 26 back, 27 coin, 28 SERVICE, 29 TEST) */
	uint32_t frames_alive;     /* +0x48 the game counts its frames here: the watchdog's sign of life */
	uint32_t profile_prs;      /* +0x4c address of profile_size words: its pr at the same samples (the caller) */
	uint32_t profile_waiting;  /* +0x50 samples skipped because it was waiting (blocked: idle time) */
	uint32_t reserved[3];      /* +0x54 (the head is 0x60 bytes) */
} nk_live_head;

/* ---- the boot ---- */

typedef struct
{
	const char *title;               /* the failure screen's ("OUTRUNNERS (NAOMI)") */
	volatile nk_live_head *live;     /* the game's live block (its head; magic, game, names and count are set here) */
	const char *game;                /* four letters */
	const char *names;               /* the names of the words after the head, apart by spaces */
	uint32_t count;                  /* how many */
	bool need_dimm;                  /* the screen says so and stops when the NetDIMM holds no archive */
	unsigned hang_first_s;           /* the watchdog: seconds with no frame before the first one (loading) ... */
	unsigned hang_s;                 /* ... and after it; 0 = no watchdog */
	bool misaligned;                 /* do misaligned loads and stores a byte at a time instead of stopping */
	void (*every_second)(void);      /* once a second, on the peeks' thread: the game's figures of the last second */
} nk_config;

/* main()'s first call. Returns the number of files in /dimm (-1 none) */
int nk_boot(const nk_config *config);

/* The thread the watchdog names and the profiler samples (by default the one that called nk_boot) */
void nk_watch_thread(void *kthread);

/* The console's last lines on the screen, for good */
void nk_show_log(const char *why) __attribute__((noreturn));

/* Bytes above the heap's top (what malloc can still take from the system) */
uint32_t nk_heap_left(void);

/* After an exception, with interrupts off: answer the NetDIMM's peeks for good (the console can still be read) */
void nk_dimmcomms_forever(void) __attribute__((noreturn));

/* ---- the cabinet ----

   The JVS I/O board through the MIE (KallistiOS's maple driver), read with the raw analog values: KallistiOS's own
   mapping wants a calibration run and reads a pedal at rest as pressed. The ranges start at the values measured on a
   Crazy Taxi upright and widen by themselves when a control goes further. Coins: the coin meters' difference (the
   driver's pulse arrives already cleared). */

typedef struct
{
	bool present;          /* an I/O board answered */
	int8_t steer;          /* the wheel, -128 full left .. 127 full right */
	uint8_t wheel;         /* the same, 0 .. 255 (0x80 the middle) */
	uint8_t gas, brake;    /* 0 at rest .. 255 pressed down */
	bool forward, back;    /* the shifter (with its spring: held while pushed) */
	bool start;            /* START held (see nk_cabinet_start_coin) */
	bool service, test;    /* the buttons and the panel's switches */
	int coins;             /* coins inserted since the last read */
	bool start_pressed, start_released;    /* START's edges since the last read */
} nk_cabinet;

/* Read the controls (once a frame); test_inputs of the live block, when on, stand in for the cabinet's */
void nk_cabinet_read(nk_cabinet *cabinet);

/* START as the coin button too (a cabinet without one at hand): pressing START inserts a coin, and START reads as
   held only for 'frames' reads after it is let go. 0 = off (the default) */
void nk_cabinet_start_coin(int frames);

/* ---- /dimm: files in the NetDIMM ----

   The BIOS copies only the program into main RAM; tools/nk_pack.py appends an archive after it (at 64 KB), whose
   files are read on demand through the cartridge port, mounted at /dimm (fopen("/dimm/name")). nk_boot mounts it. */

int nk_dimmfs_init(void);                    /* the number of files, -1 no archive */
int nk_dimmfs_mounted(void);
int nk_dimmfs_cache(const char *name);       /* a file into main RAM (its reads become a copy): its size, or -1 */
int nk_dimmfs_size(const char *name);        /* -1 none */
void nk_dimmfs_lock(void);                   /* the cartridge port's lock (the peeks use it too) */
void nk_dimmfs_unlock(void);

/* ---- the SRAM ----

   32 KB, battery-backed, at 0x00200000, byte-wide, shared by every homebrew on the board: each keeps to its own
   region (README.md has the table: Cannonball 0x0000-0x0FFF, Daytona USA recomp 0x1000-0x5FFF, Daytona USA native
   0x6000-0x7FFF). A save here: 16 bytes of header (magic, size, checksum) and the data, the magic written last. */

/* The save in the region at 'offset' of 'region' bytes, with the four-letter 'magic', into 'data' (at most 'size'
   bytes): its size, or -1 when there is none whole */
int nk_sram_load(uint32_t offset, uint32_t region, const char *magic, void *data, uint32_t size);

/* Write it: false when it does not fit */
bool nk_sram_save(uint32_t offset, uint32_t region, const char *magic, const void *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
