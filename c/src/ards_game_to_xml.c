/*
 * ARDS Game-to-XML "Decompiler"
 *
 * Description:
 *     Given a game's hex position in an Action Replay DS ROM dump, extract all
 *     codes and folders.
 *
 *     The data is supposed to be located at 0x00054000 and beyond. From 20
 *     bytes before the game's 4 digit identifier from the cartridge (e.g.
 *     the "ASME" from "NTR-ASME-USA" on a Super Mario 64 DS cartridge), all
 *     the way until the end of the strings for all of the codes in that game.
 *
 *     The address to input into this program is the location of the
 *     "01 00 1C 00", found 20 bytes before the cartridge ID, and 32 bytes
 *     before the binary section of the Action Replay codes.
 *
 * Author:
 *     Clara Nguyen (@iDestyKK)
 */

// C includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// ARDS Utils
#include "../lib/ards_util/io.h"

// CNDS (Clara Nguyen's Data Structures)
#include "../lib/CN_Vec/cn_vec.h"

// ----------------------------------------------------------------------------
// Main Function                                                           {{{1
// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
	// Argument check
	if (argc != 3) {
		fprintf(stderr, "usage: %s IN_ARDS.nds IN_POS_HEX\n", argv[0]);
		return 1;
	}

	FILE     *fp;       // File Pointer
	ARDS_GAME game;     // ARDS Game Object
	uint32_t  pos_hex;  // Position to jump to in ROM

	// Begin reading the contents of the file
	fp = fopen(argv[1], "rb");

	// Setup file and ARDS_GAME instance
	sscanf(argv[2], "%x", &pos_hex);
	game = ards_game_init();

	// Read game information at address "hex"
	ards_game_read(game, fp, pos_hex);

	// Close the file. We're done reading it
	fclose(fp);

	// Print everything out
	ards_game_export_as_xml(game, stdout);

	// Clean up all CNDS instances
	ards_game_free(game);

	//We're done here
	return 0;
}
