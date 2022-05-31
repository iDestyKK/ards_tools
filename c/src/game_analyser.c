/*
 * Game Analyser
 *
 * Description:
 *     Given a game's data and cheats, as dumped from an Action Replay DS
 *     cartridge, this can be used to display all cheats and information.
 *     Simply tries to make sure the cheats are in the correct format.
 *
 *     The data is supposed to be located at 0x00054000 and beyond. From 20
 *     bytes before the game's 4 digit identifier from the cartridge (e.g.
 *     the "ASME" from "NTR-ASME-USA" on a Super Mario 64 DS cartridge), all
 *     the way until the end of the strings for all of the codes in that game.
 *
 * Author:
 *     Clara Nguyen (@iDestyKK)
 */

// C includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// CNDS (Clara Nguyen's Data Structures)
#include "../lib/CN_Map/cn_cmp.h"
#include "../lib/CN_Map/cn_map.h"
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

		// Add to vector and get a pointer
		cn_vec_push_back(root, &tmp_data);
		header = cn_vec_at(root, cn_vec_size(root) - 1);

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
				for (k = 0; k < depth << 2; k++) printf(" ");
				printf("New Code:\n");

				header->data = cn_vec_init(ar_line_t);

				// Read each line (TODO: easy fread optimisation)
				for (j = 0; j < header->num_entries; j++) {
					file_read_type(fp, ar_line_t, tmp_line);
					cn_vec_push_back(header->data, &tmp_line);

					for (k = 0; k < depth << 2; k++) printf(" ");
					printf(
						"    %08X %08X\n",
						tmp_line.memory_location,
						tmp_line.value
					);
					fflush(stdout);
				}

				break;

			case AR_FLAG_FOLDER1:
			case AR_FLAG_FOLDER2:
				for (k = 0; k < depth << 2; k++) printf(" ");
				printf("New Folder:\n");

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

		if (it->data != NULL)
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

	cn_vec_free(root);
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

	// Begin reading the contents of the file
	fp = fopen(argv[1], "rb");

	// First 32 bytes are the header
	file_read_type(fp, ar_game_info_t, header);

	// Prepare data structure root and read all codes
	library = cn_vec_init(ar_data_t);
	file_read_cheats_and_folders(fp, library, 0, 0);

	// Close the file. We're done reading it
	fclose(fp);

	// Clean up all CNDS instances
	root_obliterate(library);

	//We're done here
	return 0;
}
