/**
 *    Name:        nk_sram.c
 *    Description: naomi-kit: a save in the board's battery-backed SRAM (naomi_kit.h)
 *    Created:     13-11-2026
 *
 *    license:BSD-3-Clause
 *    After naomi_save.c of Daytona USA's native port (0skarzer0/daytona-arcade-recomp, branch naomi-native). The
 *    SRAM is 32 KB at 0x00200000 (0xA0200000 uncached), byte-wide, shared by every homebrew on the board, each in its
 *    own region (README.md). A save: "MAGI" (written last), the size, a checksum, 4 bytes reserved, then the data.
 */
#include <stdint.h>
#include <string.h>

#include "nk/naomi_kit.h"

#define SRAM        ((volatile uint8_t *)0xA0200000u)
#define SRAM_BYTES  0x8000u
#define HEADER      16u

static uint32_t get32(uint32_t at)
{
	return (uint32_t)SRAM[at] | (uint32_t)SRAM[at + 1] << 8 | (uint32_t)SRAM[at + 2] << 16 | (uint32_t)SRAM[at + 3] << 24;
}

static void put32(uint32_t at, uint32_t value)
{
	for (unsigned index = 0; index < 4; index++)
		SRAM[at + index] = (uint8_t)(value >> (8 * index));
}

static uint32_t magic_word(const char *magic)
{
	return (uint32_t)(uint8_t)magic[0] | (uint32_t)(uint8_t)magic[1] << 8 | (uint32_t)(uint8_t)magic[2] << 16
		| (uint32_t)(uint8_t)magic[3] << 24;
}

static uint32_t checksum(uint32_t seed, const volatile uint8_t *data, uint32_t size)
{
	uint32_t sum = seed;

	for (uint32_t index = 0; index < size; index++)
		sum = (sum << 5) + sum + data[index];
	return sum;
}

static bool region_fits(uint32_t offset, uint32_t region)
{
	return region >= HEADER && offset < SRAM_BYTES && region <= SRAM_BYTES - offset;
}

int nk_sram_load(uint32_t offset, uint32_t region, const char *magic, void *data, uint32_t size)
{
	uint32_t stored;

	if (!region_fits(offset, region) || get32(offset) != magic_word(magic))
		return -1;
	stored = get32(offset + 4);
	if (stored > region - HEADER || stored > size
		|| checksum(magic_word(magic), SRAM + offset + HEADER, stored) != get32(offset + 8))
		return -1;
	for (uint32_t index = 0; index < stored; index++)
		((uint8_t *)data)[index] = SRAM[offset + HEADER + index];
	return (int)stored;
}

bool nk_sram_save(uint32_t offset, uint32_t region, const char *magic, const void *data, uint32_t size)
{
	if (!region_fits(offset, region) || size > region - HEADER)
		return false;
	put32(offset, 0);    /* (not whole while it is written) */
	for (uint32_t index = 0; index < size; index++)
		SRAM[offset + HEADER + index] = ((const uint8_t *)data)[index];
	put32(offset + 4, size);
	put32(offset + 8, checksum(magic_word(magic), SRAM + offset + HEADER, size));
	put32(offset + 12, 0);
	put32(offset, magic_word(magic));
	return true;
}
