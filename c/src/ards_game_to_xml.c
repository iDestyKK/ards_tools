/*
 * ARDS Game-to-XML "Decompiler"
 *
 * Description:
 *     Given a game's hex position in an Action Replay DS ROM dump, extract all
 *     codes and folders.
 *
 *     The address to input into this program is the location of the
 *     "01 00 1C 00", found 20 bytes before the cartridge ID, and 32 bytes
 *     before the binary section of the Action Replay codes.
 *
 *     Multiple addresses can be supplied to make a single XML file of multiple
 *     games.
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
	if (argc < 3) {
		fprintf(
			stderr,
			"usage: %s IN_ARDS.nds IN_POS_HEX1 [IN_POS_HEX2 [...]]\n",
			argv[0]
		);

		return 1;
	}

	FILE      *fp;       // File Pointer
	CN_VEC     games;    // ARDS Game Object Vector
	size_t     game_num, // Game counter
	           i;        // Loop counter
	ARDS_GAME *game_arr; // Game Array
	uint32_t   pos_hex;  // Position to jump to in ROM

	// Setup variables and data structures
	game_num = argc - 2;
	games = cn_vec_init(ARDS_GAME);
	cn_vec_resize(games, game_num);
	game_arr = cn_vec_array(games, ARDS_GAME);

	// Begin reading the contents of the file
	fp = fopen(argv[1], "rb");

	// Read all games from the addresses in the arguments
	for (i = 0; i < game_num; i++) {
		// Setup files and ARDS_GAME instances
		sscanf(argv[i + 2], "%x", &pos_hex);
		game_arr[i] = ards_game_init();

		// Read game information at address "hex"
		ards_game_read(game_arr[i], fp, pos_hex);
	}

	// Close the file. We're done reading it
	fclose(fp);

	// Print everything out
	ards_game_export_as_xml(games, stdout);

	// Clean up all CNDS instances
	for (i = 0; i < game_num; i++)
		ards_game_free(game_arr[i]);

	cn_vec_free(games);

	//We're done here
	return 0;
}
