/*
 * ARDS Game Listing utility
 *
 * Description:
 *     Given an Action Replay DS ROM dump, list all addresses of possible games
 *     to dump. These addresses can be fed into "ards_game_to_xml" to export an
 *     XML codelist.
 *
 *     This program will skip all "game lists" and look directly in memory for
 *     magic numbers in hopes it finding a game.
 *
 * Author:
 *     Clara Nguyen (@iDestyKK)
 */

// C Includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// ARDS Utils
#include "../lib/ards_util/io.h"

// ----------------------------------------------------------------------------
// Main Function                                                           {{{1
// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
	// Argument check
	if (argc != 2) {
		fprintf(stderr, "usage: %s IN_ARDS.nds\n", argv[0]);

		return 1;
	}

	FILE          *fp;
	ar_game_info_t header;
	size_t         pos, i, fsize;
	char          *name;
	char          *shit;
	unsigned char *buf;

	// Defaults
	name = NULL;
	shit = NULL;

	// Setup file for traversal
	fp = fopen(argv[1], "rb");

	// Get size of file
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);

	// First game should be at 0x00054000
	fseek(fp, 0x54000, SEEK_SET);

	// For every "game"...
	while (1) {
		// Fix address. At 0x00XYYYYY, the "YYYYY" should be 0x54000 or above.
		pos = ftell(fp);

		if (pos & 0x000FFFFF < 0x54000) {
			pos = (pos & 0xFFF00000) + 0x54000;
			fseek(fp, pos, SEEK_SET);
		}

		// If we are past the end of the file, kill it
		if (ftell(fp) >= fsize)
			break;

		// Read header
		file_read_type(fp, ar_game_info_t, header);

		// Check Magic Number
		if (header.magic != 0x001C0001 || header.nx20 != 0x0020) {
			fseek(fp, pos + 1, SEEK_SET);
			continue;
		}

		// Read name, if possible
		fseek(fp, header.code_bytes_size - 31, SEEK_CUR);

		if (name != NULL) free(name); name = file_read_string(fp);
		if (shit != NULL) free(shit); shit = file_read_string(fp);

		// Print
		printf("0x%08x - %s\n", pos, name);

		// Skip all codes afterwards
		for (i = 0; i < header.num_codes; i++) {
			if (shit != NULL) free(shit); shit = file_read_string(fp);
			if (shit != NULL) free(shit); shit = file_read_string(fp);
		}
	}

	// We're done here. Clean up
	fclose(fp);

	if (name != NULL) free(name);
	if (shit != NULL) free(shit);

	// Have a nice day
	return 0;
}
