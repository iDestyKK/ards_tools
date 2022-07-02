/*
 * ARDS Utils - Game ID
 *
 * Description:
 *     Provides interface for obtaining the Game ID from an NDS game.
 *
 *     The first 4 chars are from bytes 0x0C through 0x0F of the ROM. The
 *     remaining characters are the bitwise NOT of a CRC32 of the first 512
 *     bytes of the ROM file.
 *
 * Author:
 *     Clara Nguyen (@iDestyKK)
 */

#ifndef __ARDS_UTILS_GAME_ID__
#define __ARDS_UTILS_GAME_ID__

// C Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Utility
uint32_t reverse(uint32_t);
uint32_t crc32  (const char *, size_t);

// Convenience
void get_gameid(const char *, char *);

#endif
