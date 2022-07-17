/*
 * ARDS Firmware Checksum
 *
 * Description:
 *     Given an Action Replay DS firmware file, compute the checksum that will
 *     appear in the 5th and 6th byte of the file. This will skip the first 8
 *     bytes of the file.
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

int main(int argc, char **argv) {
	// Argument check
	if (argc != 2) {
		fprintf(stderr, "usage: %s ARDS_FIRMWARE.bin\n", argv[0]);
		return 1;
	}

	FILE    *fp;
	size_t   buffer_sz, ft;
	uint8_t *buffer;

	fp = fopen(argv[1], "rb");

	if (!fp) {
		fprintf(stderr, "Error: Failed to open file: %s\n", strerror(errno));
		return 1;
	}
	
	// Get file size and check
	fseek(fp, 0, SEEK_END);
	ft = ftell(fp);

	if (ft <= 8) {
		fprintf(
			stderr, "Error: File must be larger than 8 bytes (is %d)...\n", ft
		);

		return 2;
	}

	// Read in all but first 8 bytes
	buffer_sz = ft - 8;
	fseek(fp, 8, SEEK_SET);
	
	buffer = (uint8_t *) malloc(sizeof(uint8_t) * buffer_sz);
	fread(&buffer[0], sizeof(uint8_t), buffer_sz, fp);
	fclose(fp);

	// Get checksum result
	printf(
		"%04X\n", crc16(0xFFFF, buffer_sz, buffer)
	);

	// Clean up. We're done here.
	free(buffer);
	return 0;
}
