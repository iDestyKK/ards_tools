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

	FILE           *fp;       // File Pointer
	ar_game_info_t  header;   // Header
	CN_VEC          library;  // Codes and Folders
	char           *name;     // Game Name
	char           *shit;     // lol
	uint32_t        pos_hex;  // Position to jump to in ROM

	// Begin reading the contents of the file
	fp = fopen(argv[1], "rb");

	// Skip to specified section
	sscanf(argv[2], "%x", &pos_hex);
	fseek(fp, pos_hex, SEEK_SET);

	// First 32 bytes are the header
	file_read_type(fp, ar_game_info_t, header);

	// Prepare data structure root and read all codes
	library = cn_vec_init(ar_data_t);
	file_read_cheats_and_folders(fp, library, 0, 0);

	// Jump to the end of the code bytes segment and start reading text
	fseek(fp, pos_hex + header.code_bytes_size + 1, SEEK_SET);

	// Game name is first
	name = file_read_string(fp);
	shit = file_read_string(fp);

	// Now unleash recursion and get everything else
	file_read_names(fp, library);

	// Close the file. We're done reading it
	fclose(fp);

	// Print everything out
	library_dump_as_xml(stdout, library, name, header);

	// Clean up all CNDS instances
	root_obliterate(library);
	free(name);
	free(shit);

	//We're done here
	return 0;
}
