/**
 *    Name:        main.c
 *    Description: naomi-kit's smallest game: the controls and the seconds on the screen, live figures for the
 *                 Raspberry (naomi-pi-tools' pi/live.py), the console on the screen when TEST is held
 *    Created:     13-11-2026
 *
 *    license:BSD-3-Clause
 */
#include <kos.h>
#include <dc/video.h>
#include <stdio.h>

#include "nk/naomi_kit.h"

/* The live block: the head, then the game's own words, named for live.py */
static struct
{
	nk_live_head head;
	uint32_t frames;
	uint32_t coins;
} live;

static uint32_t frames_this_second;

static void every_second(void)
{
	live.frames = frames_this_second;
	frames_this_second = 0;
}

int main(void)
{
	static const nk_config config = {
		.title = "NAOMI-KIT EXAMPLE", .live = &live.head, .game = "NKEX", .names = "frames coins", .count = 2,
		.need_dimm = false, .hang_first_s = 30, .hang_s = 5, .misaligned = true, .every_second = every_second,
	};
	nk_cabinet cabinet;

	nk_boot(&config);
	nk_cabinet_start_coin(6);
	vid_set_mode(DM_640x480, PM_RGB565);
	for (int test_held = 0;; frames_this_second++, live.head.frames_alive++)
	{
		vid_waitvbl();
		nk_cabinet_read(&cabinet);
		live.coins += (uint32_t)cabinet.coins;
		test_held = cabinet.test ? test_held + 1 : 0;
		if (test_held > 120)
			nk_show_log("TEST HELD");
		vid_clear(0, 0, 32);
		printf("\rwheel %4d gas %3u brake %3u %s%s%s coins %lu   ", cabinet.steer, cabinet.gas, cabinet.brake,
			cabinet.forward ? "HIGH " : "", cabinet.back ? "LOW " : "", cabinet.start ? "START" : "",
			(unsigned long)live.coins);
	}
	return 0;
}
