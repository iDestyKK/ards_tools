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

// CNDS (Clara Nguyen's Data Structures)
#include "../lib/CN_Vec/cn_vec.h"

// ----------------------------------------------------------------------------
// ARDS Data Structs                                                       {{{1
// ----------------------------------------------------------------------------

typedef struct AR_GAME_INFO_T {
	uint32_t idk1;            // 00 - 03
	uint32_t idk2;            // 04 - 07
	uint32_t code_bytes_size; // 08 - 11. Bytes between game_info_t to text - 1
	uint32_t idk3;            // 12 - 15
	uint32_t idk4;            // 16 - 19
	char     ID[4];           // 20 - 23
	uint32_t idk5;            // 24 - 27
	uint32_t idk6;            // 28 - 31 (END)
} ar_game_info_t;

typedef struct AR_LINE_T {
	uint32_t memory_location; // Left side of AR code
	uint32_t value;           // Right side of AR code
} ar_line_t;

typedef enum AR_FLAG_T {
	AR_FLAG_TERMINATE = 0,
	AR_FLAG_CODE      = 1,
	AR_FLAG_FOLDER1   = 2,
	AR_FLAG_FOLDER2   = 6
} ar_flag_t;

typedef struct AR_DATA_T {
	uint16_t   flag;            // Good luck getting that ENUM to work here
	uint16_t   num_entries;     // A line is 8 bytes, or "XXXXXXXX XXXXXXXX"
	char      *name;            // Name of cheat/folder/whatever
	char      *desc;            // Description
	CN_VEC     data;            // Abuse CN_Vec for type-agnosticism via flag
} ar_data_t;

// ----------------------------------------------------------------------------
// File Reading Helpers                                                    {{{1
// ----------------------------------------------------------------------------

// Read data as specific type
#define file_read_type(fp, type, var) \
	fread(&var, sizeof(type), 1, fp)

// Read C-Style String
char *file_read_string(FILE *fp) {
	CN_VEC buffer;
	char *str, tmp;

	buffer = cn_vec_init(char);

	while (1) {
		//Read and add character to vector until null-terminated
		file_read_type(fp, char, tmp);
		cn_vec_push_back(buffer, &tmp);

		if (tmp == 0)
			break;
	}

	//Convert CN_Vec to string
	str = (char *) calloc(cn_vec_size(buffer), sizeof(char));
	memcpy(str, cn_vec_data(buffer), cn_vec_size(buffer));

	//Clean up
	cn_vec_free(buffer);

	return str;
}

// ----------------------------------------------------------------------------
// Recursion Helpers                                                       {{{1
// ----------------------------------------------------------------------------

void file_read_cheats_and_folders(
	FILE    *fp,
	CN_VEC   root,
	uint16_t num,
	uint8_t  depth
) {
	ar_data_t  tmp_data;
	ar_data_t *header;
	ar_line_t  tmp_line;
	uint16_t i, j, k;

	// Just before something stupid happens...
	tmp_data.name = NULL;
	tmp_data.data = NULL;

	// Read in codes and folders
	for (i = 0; (depth == 0) || (i < num); i++) {
		// Initial information
		file_read_type(fp, uint16_t, tmp_data.flag       );
		file_read_type(fp, uint16_t, tmp_data.num_entries);


		if ((ar_flag_t) (tmp_data.flag & 0xFF) == AR_FLAG_TERMINATE) {
			//If we hit the end, fake the header and prevent insertion
			header = &tmp_data;
		}
		else {
			// Add to vector and get a pointer
			cn_vec_push_back(root, &tmp_data);
			header = cn_vec_at(root, cn_vec_size(root) - 1);
		}

		// Act based on it being either a folder or cheat code
		switch ((ar_flag_t) header->flag & 0xFF) {
			case AR_FLAG_TERMINATE:
				/*
				 * Go back 4 bytes and make previous call re-read so recursion
				 * can exit gracefully.
				 */

				fseek(fp, -4, SEEK_CUR);

				return;

			case AR_FLAG_CODE:
				// It's an AR code.
				// This means (8 * num_entries) bytes are read
				header->data = cn_vec_init(ar_line_t);

				// Read each line (TODO: easy fread optimisation)
				for (j = 0; j < header->num_entries; j++) {
					file_read_type(fp, ar_line_t, tmp_line);
					cn_vec_push_back(header->data, &tmp_line);
				}

				break;

			case AR_FLAG_FOLDER1:
			case AR_FLAG_FOLDER2:
				header->data = cn_vec_init(ar_data_t);
				file_read_cheats_and_folders(
					fp,
					header->data,
					header->num_entries,
					depth + 1
				);

				break;

			default:
				return;
		}
	}
}

void root_obliterate(CN_VEC root) {
	ar_data_t *it;

	cn_vec_traverse(root, it) {
		if (it->name != NULL)
			free(it->name);

		if (it->desc != NULL)
			free(it->desc);

		if (it->data != NULL) {
			switch ((ar_flag_t) it->flag & 0xFF) {
				case AR_FLAG_CODE:
					// AR Code lists are just a list of ar_line_t
					cn_vec_free(it->data);
					break;

				case AR_FLAG_FOLDER1:
				case AR_FLAG_FOLDER2:
					// AR Folders are recursive
					root_obliterate(it->data);
					break;

				case AR_FLAG_TERMINATE:
				default:
					// These should never happen...
					break;
			}
		}
	}

	cn_vec_free(root);
}

void file_read_names(FILE *fp, CN_VEC root) {
	ar_data_t *it;

	cn_vec_traverse(root, it) {
		it->name = file_read_string(fp);
		it->desc = file_read_string(fp);

		if (it->data != NULL) {
			switch ((ar_flag_t) it->flag & 0xFF) {
				case AR_FLAG_FOLDER1:
				case AR_FLAG_FOLDER2:
					// AR Folders are recursive
					file_read_names(fp, it->data);
					break;

				case AR_FLAG_TERMINATE:
				case AR_FLAG_CODE:
				default:
					break;
			}
		}
	}
}

void library_dump(FILE *out, CN_VEC root, size_t depth) {
	ar_data_t *it;
	ar_line_t *lt;

	cn_vec_traverse(root, it) {
		fprintf(
			out,
			(strlen(it->desc) == 0) ?
				"%*s%s\n" :
				"%*s%s (%s)\n",
			depth << 2,
			(depth == 0) ? "" : " ",
			it->name,
			it->desc
		);

		if (it->data != NULL) {
			switch ((ar_flag_t) it->flag & 0xFF) {
				case AR_FLAG_CODE:
					// Print out all lines of the AR code
					cn_vec_traverse(it->data, lt) {
						fprintf(
							out,
							"%*s%08X %08X\n",
							(depth + 1) << 2,
							" ",
							lt->memory_location,
							lt->value
						);
					}
					break;

				case AR_FLAG_FOLDER1:
				case AR_FLAG_FOLDER2:
					// AR Folders are recursive
					library_dump(out, it->data, depth + 1);
					break;

				case AR_FLAG_TERMINATE:
				default:
					break;
			}
		}
	}
}

// ----------------------------------------------------------------------------
// Main Function                                                           {{{1
// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
	// Argument check
	if (argc != 2) {
		fprintf(stderr, "usage: %s in.bin\n");
		return 1;
	}

	FILE           *fp;       // File Pointer
	ar_game_info_t  header;   // Header
	CN_VEC          library;  // Codes and Folders
	char           *name;     // Game Name
	char           *shit;     // lol

	// Begin reading the contents of the file
	fp = fopen(argv[1], "rb");

	// First 32 bytes are the header
	file_read_type(fp, ar_game_info_t, header);

	// Prepare data structure root and read all codes
	library = cn_vec_init(ar_data_t);
	file_read_cheats_and_folders(fp, library, 0, 0);

	// Jump to the end of the code bytes segment and start reading text
	fseek(fp, header.code_bytes_size + 1, SEEK_SET);

	// Game name is first
	name = file_read_string(fp);
	shit = file_read_string(fp);

	// Now unleash recursion and get everything else
	file_read_names(fp, library);

	// Close the file. We're done reading it
	fclose(fp);

	// Print everything out
	library_dump(stdout, library, 0);

	// Clean up all CNDS instances
	root_obliterate(library);
	free(name);
	free(shit);

	//We're done here
	return 0;
}
