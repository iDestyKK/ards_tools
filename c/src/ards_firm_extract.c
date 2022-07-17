/*
 * ARDS Firmware Extract
 *
 * Description:
 *     Given an Action Replay DS ROM dump, extract the firmware and generate a
 *     firmware file for it.
 *
 *     This assumes that the header will start with "FIRM". Perhaps in a
 *     revision, I will add in an optional argument that lets this be changed
 *     upon generation.
 *
 * Author:
 *     Clara Nguyen (@iDestyKK)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "../lib/ards_util/firmware.h"

#define FW_MAX   0x00040000
#define FW_START 0x00100000

int main(int argc, char **argv) {
	// Argument check
	if (argc != 2) {
		fprintf(stderr, "usage: %s ARDS_IN.nds > FIRMWARE_OUT.bin\n", argv[0]);
		return 1;
	}

	FILE    *fp;
	size_t   firmware_sz, ft;
	uint8_t *buffer;
	uint32_t checksum; // Trust me

	fp = fopen(argv[1], "rb");

	if (!fp) {
		fprintf(stderr, "Error: Failed to open file: %s\n", strerror(errno));
		return 1;
	}
	
	// Get file size and check
	fseek(fp, 0, SEEK_END);
	ft = ftell(fp);

	if (ft < 0x0013FFFF) {
		fprintf(
			stderr,
			"Error: File must be larger than 1310719 bytes (is %d)...\n",
			ft
		);

		return 2;
	}

	// Read in from 0x00100000 to 0x00140000.
	fseek(fp, FW_START, SEEK_SET);
	
	buffer = (uint8_t *) malloc(sizeof(uint8_t) * FW_MAX);
	fread(&buffer[0], sizeof(uint8_t), FW_MAX, fp);
	fclose(fp);

	// Skip to the end of the buffer and go backwards until not 0xFF
	// TODO: This will fail miserably if all bytes are 0xFF, lol...
	for (firmware_sz = FW_MAX; buffer[firmware_sz - 1] == 0xFF; firmware_sz--);

	// Get checksum result and write firmware out to stdout
	checksum = (uint32_t) crc16(0xFFFF, firmware_sz, buffer);

	printf("FIRM");
	fwrite(&checksum, sizeof(uint32_t), 1, stdout);
	fwrite(buffer, sizeof(uint8_t), firmware_sz, stdout);

	// Clean up. We're done here.
	free(buffer);
	return 0;
}
