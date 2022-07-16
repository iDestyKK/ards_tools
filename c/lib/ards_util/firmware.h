/*
 * ARDS Utils - Firmware
 *
 * Description:
 *     Provides interface for dealing with firmwares for the Action Replay DS,
 *     I guess.
 *
 * Author:
 *     Clara Nguyen (@iDestyKK)
 */

#ifndef __ARDS_UTILS_FIRMWARE__
#define __ARDS_UTILS_FIRMWARE__

#include <stdlib.h>
#include <stdint.h>

uint16_t crc16(uint16_t, size_t, uint8_t *);

#endif
