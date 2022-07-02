/*
 * Get Game ID
 *
 * Description:
 *     Gets the long Game ID (XXXX-XXXXXXXX) for an NDS game.
 *
 *     The first 4 chars are from bytes 0x0C through 0x0F of the ROM. The
 *     remaining characters are the bitwise NOT of a CRC32 of the first 512
 *     bytes of the ROM file.
 *
 *     Written modular so code can be used by other files if needed.
 *
 * Author:
 *     Clara Nguyen (@iDestyKK)
 */

// C Include
#include <stdio.h>

// Utility
#include "../lib/ards_util/gameid.h"

int main(int argc, char **argv) {
	// Argument check
	if (argc != 2) {
		fprintf(stderr, "usage: %s NDS_IN\n", argv[0]);
		return 1;
	}

	// Setup buffer. XXXX-XXXXXXXX + NULL
	char gameid[14];

	// Process
	get_gameid(argv[1], &gameid[0]);

	// Done
	printf("%s\n", gameid);
	return 0;
}
