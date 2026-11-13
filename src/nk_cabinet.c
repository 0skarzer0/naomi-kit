/**
 *    Name:        nk_cabinet.c
 *    Description: naomi-kit: the cabinet's controls (naomi_kit.h): wheel, pedals, shifter, START, coins
 *    Created:     13-11-2026
 *
 *    license:BSD-3-Clause
 *    From the controls of Daytona USA's native port (naomi_input.c, 0skarzer0/daytona-arcade-recomp, branch
 *    naomi-native) and of OutRunners' (ports/naomi/main.c, 0skarzer0/outrunners-port, branch naomi). The ranges
 *    are the raw JVS values measured on a Crazy Taxi upright (naomi-wheel-games' docs/hardware.md); they widen by
 *    themselves when a control goes further. The pedals keep 6 % at each end, so that rest reads 0 and a full press
 *    255. Coins: the coin meters' difference (the MIE driver's coin pulse arrives already cleared).
 */
#include <kos.h>
#include <dc/maple.h>
#include <dc/maple/mie.h>
#include <string.h>

#include "nk/naomi_kit.h"

extern volatile nk_live_head *nk_live_block(void);    /* (nk_boot.c) */

static struct
{
	int wheel_min, wheel_centre, wheel_max;
	int gas_min, gas_max;
	int brake_min, brake_max;
} calibration = { 8448, 33792, 60160, 11776, 47104, 14080, 47360 };

static bool coin_counts_known;
static uint16_t coin_counts[MIE_JVS_COIN_SLOTS];
static int start_coin_frames;    /* nk_cabinet_start_coin */
static bool start_held;          /* the cabinet's START, last read */
static int start_frames;         /* START reads as held for these many more reads */
static bool start_read;          /* START as read last time (for the edges) */

void nk_cabinet_start_coin(int frames)
{
	start_coin_frames = frames;
}

static int raw_analog(uint16_t raw)
{
	return (int)(raw & MIE_JVS_ANALOG_RAW_MASK);
}

static int8_t steer(int raw)
{
	int value;

	if (raw < calibration.wheel_min)
		calibration.wheel_min = raw;
	if (raw > calibration.wheel_max)
		calibration.wheel_max = raw;
	if (raw < calibration.wheel_centre)
		value = -((calibration.wheel_centre - raw) * 128) / (calibration.wheel_centre - calibration.wheel_min);
	else
		value = ((raw - calibration.wheel_centre) * 127) / (calibration.wheel_max - calibration.wheel_centre);
	return (int8_t)(value < -128 ? -128 : (value > 127 ? 127 : value));
}

static uint8_t pedal(int raw, int *low, int *high)
{
	int margin, from, to;

	if (raw < *low)
		*low = raw;
	if (raw > *high)
		*high = raw;
	margin = (*high - *low) * 6 / 100;
	from = *low + margin;
	to = *high - margin;
	if (raw <= from)
		return 0;
	if (raw >= to)
		return 255;
	return (uint8_t)((raw - from) * 255 / (to - from));
}

void nk_cabinet_read(nk_cabinet *cabinet)
{
	maple_device_t *device = maple_enum_type(0, MAPLE_FUNC_MIE);
	const mie_state_t *state = device && device->valid ? (const mie_state_t *)maple_dev_status(device) : NULL;
	volatile nk_live_head *live = nk_live_block();
	bool start = false;

	memset(cabinet, 0, sizeof(*cabinet));
	cabinet->wheel = 0x80;
	if (state)
	{
		const mie_jvs_inputs_t *jvs = &state->jvs;

		cabinet->present = true;
		cabinet->steer = steer(raw_analog(jvs->wheel));
		cabinet->wheel = (uint8_t)(cabinet->steer + 128);
		cabinet->gas = pedal(raw_analog(jvs->accel), &calibration.gas_min, &calibration.gas_max);
		cabinet->brake = pedal(raw_analog(jvs->brake), &calibration.brake_min, &calibration.brake_max);
		cabinet->forward = jvs->p1.up;
		cabinet->back = jvs->p1.down;
		start = jvs->p1.start;
		cabinet->service = jvs->p1.service || jvs->panel.psw.service;
		cabinet->test = jvs->system.test || jvs->panel.psw.test;
		for (int slot = 0; slot < MIE_JVS_COIN_SLOTS; slot++)
		{
			if (coin_counts_known && jvs->coin[slot].count != coin_counts[slot])
				cabinet->coins += (uint16_t)(jvs->coin[slot].count - coin_counts[slot]);
			coin_counts[slot] = jvs->coin[slot].count;
		}
		coin_counts_known = true;
	}
	if (live && (live->test_inputs & 0x80000000u))
	{
		/* a test with no one at the cabinet (tools/pi/drive.py): the poked controls instead */
		const uint32_t test = live->test_inputs;

		if (test & 1)
		{
			cabinet->coins++;
			live->test_inputs &= ~1u;
		}
		start = start || (test & 2);
		cabinet->forward = cabinet->forward || (test & 4);
		cabinet->back = cabinet->back || (test & 8);
		cabinet->service = cabinet->service || (test & 16);
		if (test >> 8 & 0xff)
			cabinet->gas = (uint8_t)(test >> 8);
		if (test >> 16 & 0xff)
		{
			cabinet->wheel = (uint8_t)(test >> 16);
			cabinet->steer = (int8_t)(cabinet->wheel - 128);
		}
		if (test >> 24)
			cabinet->brake = (uint8_t)(test >> 24);
	}
	if (start_coin_frames > 0)
	{
		/* START as the coin button: a press is a coin, a release START for a few reads */
		if (start && !start_held)
			cabinet->coins++;
		if (!start && start_held)
			start_frames = start_coin_frames;
		start_held = start;
		start = start_frames > 0;
		if (start_frames > 0)
			start_frames--;
	}
	cabinet->start = start;
	cabinet->start_pressed = start && !start_read;
	cabinet->start_released = !start && start_read;
	start_read = start;
	if (live)
		live->inputs = (uint32_t)cabinet->wheel | (uint32_t)cabinet->gas << 8 | (uint32_t)cabinet->brake << 16
			| (uint32_t)(cabinet->start | cabinet->forward << 1 | cabinet->back << 2 | (cabinet->coins > 0) << 3
			| cabinet->service << 4 | cabinet->test << 5) << 24;
}
