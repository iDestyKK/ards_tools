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
// Verification Functions                                                  {{{1
// ----------------------------------------------------------------------------

int verify_code_segment(unsigned char *buf, size_t len, uint16_t num_codes) {
	size_t    pos, i, j, c_found;
	ar_flag_t flag, in_flag;
	uint16_t  num , in_num ;

	// For every code...
	for (i = pos = c_found = 0; i < num_codes; i++) {
		if (pos > len)
			// Exceeded buffer size
			return 3;

		// Code/Folder header via reinterpret casting
		flag = (ar_flag_t) (*(uint8_t  *) &buf[pos    ]);
		num  =              *(uint16_t *) &buf[pos + 2] ;

		pos += 4;

		// Act based on flag
		switch (flag) {
			case AR_FLAG_CODE:
				// All codes are 8 bytes. So n * 8.
				pos += 8 * num;
				c_found++;

				break;

			case AR_FLAG_FOLDER1:
			case AR_FLAG_FOLDER2:
				// ARDS doesn't allow nested folders. Cheat and avoid recursion
				for (j = 0; j < num; j++) {
					// They better be codes or else...
					in_flag = (ar_flag_t) (*(uint8_t  *) &buf[pos    ]);
					in_num  =              *(uint16_t *) &buf[pos + 2] ;

					if (in_flag != AR_FLAG_CODE)
						// Flag inside folder was not a code
						return 2;

					pos += 4 + (8 * in_num);
					c_found++;

					if (c_found == num_codes && j < num - 1)
						// More codes than mentioned in header
						return 4;
				}

				break;

			case AR_FLAG_TERMINATE:
				if (c_found == num_codes)
					return 0;
				else
					// More codes than mentioned in header
					return 4;
				break;

			default:
				// Invalid flag was found
				return 1;
		}
	}

	// Nothing is wrong
	return 0;
}

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
	int            status;

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

		// Read bytes segment and check if it's correct
		buf = (unsigned char *)
			malloc(sizeof(unsigned char) * header.code_bytes_size - 32);

		fread(&buf[0], sizeof(unsigned char), header.code_bytes_size - 32, fp);

		status = verify_code_segment(
			buf,
			header.code_bytes_size - 32,
			header.num_codes
		);

		free(buf);

		if (status != 0) {
			fseek(fp, pos + 1, SEEK_SET);
			continue;
		}

		// Read name, if possible
		fseek(fp, 1, SEEK_CUR);

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
