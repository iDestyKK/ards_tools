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

// CNDS (Clara Nguyen's Data Structures)
#include "../lib/CN_Vec/cn_vec.h"
#include "../lib/CN_Map/cn_cmp.h"
#include "../lib/CN_Map/cn_map.h"

// ----------------------------------------------------------------------------
// CNDS Configuration Functions                                            {{{1
// ----------------------------------------------------------------------------

/*
 * Map destructor for freeing a C-String key in a CN_Map.
 */

void destruct_key(CNM_NODE *node) {
	char *v = *(char **) node->key;

	if (v != NULL)
		free(v);
}

// ----------------------------------------------------------------------------
// Basic Argument Parser                                                   {{{1
// ----------------------------------------------------------------------------

typedef struct ARGS_T {
	uint8_t flag_allow_dup,
	        flag_error,
	        flag_skip_name,
			flag_rescue,
	        flag_warning;
} args_t;

void print_help(int argc, char **argv) {
	printf("usage: %s [-dehnrw] IN_ARDS.nds\n", argv[0]);
	printf("Listing utility for game addresses in an Action Replay DS ROM "
		"dump.\n\n");

	printf("Optional arguments are:\n\n");

	printf("\t-d\tAllow duplicates. Won't skip reading the same game even if "
		"it's present\n\t\tin multiple locations in the same ROM. By "
		"default, duplicates are\n\t\tskipped, and it's determined by the "
		"Game ID (XXXX-YYYYYYYY).\n\n");

	printf("\t-e\tPrints errors. By default, this will only print out games "
		"where a code\n\t\tsection check looks correct. With this flag, it"
		" will additionally print\n\t\terrors to stderr.\n\n");

	printf("\t-h\tPrints this help prompt in the terminal and then "
		"terminates.\n\n");

	printf("\t-n\tSkips name reading. By default, this will read a game "
		"header, validate\n\t\tthe code section, and then run 2n C-Style "
		"string reads. This argument\n\t\ttells it to skip the final step "
		"and try to read the next game through\n\t\tthose string "
		"sections.\n\n");

	printf("\t-r\tRescue mode. Skips the game list and tries to search for "
		"games via a\n\t\tdeep search. Brute force. Searches all bytes after "
		"0x00054000. Will be\n\t\tmuch slower.\n\n");

	printf("\t-w\tShows warnings while reading. Prints to stderr.\n");

	exit(0);
}

void parse_flags(int argc, char **argv, args_t *obj) {
	int i, j, len;

	// Defaults
	obj->flag_allow_dup = 0;
	obj->flag_error     = 0;
	obj->flag_skip_name = 0;
	obj->flag_rescue    = 0;
	obj->flag_warning   = 0;

	// Go through every argument and read characters
	for (i = 1; i < argc; i++) {
		// Skip if not a valid flag header
		if (argv[i][0] != '-')
			continue;

		len = strlen(argv[i]);
		for (j = 1; j < len; j++) {
			switch (argv[i][j]) {
				case 'd':
					// Allow duplicate games
					obj->flag_allow_dup = 1;
					break;

				case 'e':
					// Show errors
					obj->flag_error = 1;
					break;

				case 'h':
					// Shows help prompt and terminate
					print_help(argc, argv);
					break;

				case 'n':
					// Skips name reading
					obj->flag_skip_name = 1;
					break;

				case 'r':
					// Enter Rescue Mode
					obj->flag_rescue = 1;
					break;

				case 'w':
					// Show warnings
					obj->flag_warning = 1;
					break;

				default:
					// Invalid Flag
					fprintf(
						stderr,
						"WARN: Invalid flag \"%c\" was given. Ignoring...\n",
						argv[i][j]
					);

					break;
			}
		}
	}
}

// ----------------------------------------------------------------------------
// Verification Functions                                                  {{{1
// ----------------------------------------------------------------------------

int verify_code_segment(
	unsigned char *buf,
	size_t         len,
	uint16_t       num_codes,
	args_t        *args,
	size_t        *err_pos,
	uint8_t       *err_val
) {
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
		switch (flag & 0x03) {
			case AR_FLAG_CODE:
				// All codes are 8 bytes. So n * 8.
				pos += 8 * num;
				c_found++;

				break;

			case AR_FLAG_FOLDER:
				// ARDS doesn't allow nested folders. Cheat and avoid recursion
				for (j = 0; j < num; j++) {
					if (pos > len)
						// Exceeded buffer size
						return 3;

					// They better be codes or else...
					in_flag = (ar_flag_t) (*(uint8_t  *) &buf[pos    ]);
					in_num  =              *(uint16_t *) &buf[pos + 2] ;

					if ((in_flag & 0x03) != AR_FLAG_CODE) {
						// Flag inside folder was not a code
						*err_pos = pos;
						return 2;
					}

					pos += 4 + (8 * in_num);
					c_found++;

					if (c_found == num_codes && j < num - 1) {
						// More codes than mentioned in header
						*err_pos = pos;
						return 4;
					}
				}

				break;

			case AR_FLAG_TERMINATE:
				if (c_found == num_codes)
					// Looks good to me
					return 0;
				else {
					// More codes than mentioned in header
					*err_pos = pos;
					return 4;
				}
				break;

			default:
				// Invalid flag was found
				*err_pos = pos - 4;
				*err_val = flag;
				return 1;
		}
	}

	// Nothing is wrong
	return 0;
}

// ----------------------------------------------------------------------------
// Regular Mode                                                            {{{1
// ----------------------------------------------------------------------------

/*
 * Use game list at 0x00044000 to get location of each game. Then jump to each
 * spot to get game information.
 */

int data_iterate(int argc, char **argv, args_t *args) {
	FILE *fp;
	ar_game_list_node header_list, *it;
	ar_game_info_t    header_game;
	CN_VEC game_list;  // vector<ar_game_list_node>

	char *title;
	size_t i;

	title = NULL;

	// Prepare CN_Vec for insertion of "ar_game_list_node"s
	game_list = cn_vec_init(ar_game_list_node);

	// Setup file for traversal
	// The first argument without a "-" is the filename.
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;
	}
	fp = fopen(argv[i], "rb");

	// The code list is at 0x00044000
	fseek(fp, 0x44000, SEEK_SET);

	// Read in "ar_game_list_node"s until "FF FF FF FF"
	while (1) {
		// Read 32 bytes
		fread(&header_list, sizeof(struct AR_GAME_LIST_NODE), 1, fp);

		// Check header
		if (header_list.magic == 0xFFFFFFFFU) {
			// FF FF FF FF = End of list
			break;
		}
		else
		if (header_list.magic != 0x00000000U) {
			// Anything other than "00 00 00 00" = not a game
			continue;
		}

		// Guaranteed to be "00 00 00 00" at this point. Insert into CN_VEC
		cn_vec_push_back(game_list, &header_list);
	}

	// Now go through each game and print out information
	cn_vec_traverse(game_list, it) {
		// Jump to spot in memory
		fseek(fp, 0x40000 + (it->location << 8), SEEK_SET);

		// Read in the game header (32 bytes)
		fread(&header_game, sizeof(struct AR_GAME_INFO_T), 1, fp);

		// Jump to the title and read the name of the game
		fseek(fp, header_game.offset_text - 32 + 1, SEEK_CUR);
		title = file_read_string(fp);

		// Print info
		printf(
			"0x%08x - %s - %s\n",
			0x40000 + (it->location << 8),
			it->ID.raw,
			title
		);

		// Clean up title, since we are done with it
		free(title);
	}

	// Clean up
	fclose(fp);
	cn_vec_free(game_list);
}

// ----------------------------------------------------------------------------
// Rescue Mode                                                             {{{1
// ----------------------------------------------------------------------------

/*
 * Search bytes from 0x00054000 onwards in each chunk for information on codes.
 * This bypasses the code list at 0x00044000 entirely and tries to make sense
 * out of bytes and magic numbers it finds.
 */

int data_rescue(int argc, char **argv, args_t *args) {
	FILE          *fp;
	ar_game_info_t header;
	size_t         pos, i, fsize, err_at;
	uint8_t        err_val;
	char          *name;
	char          *shit;
	unsigned char *buf;
	int            status, printable;

	CN_MAP         game_ids;
	CNM_ITERATOR   game_id_it;
	char          *game_id_key, *key_tmp, dummy;

	// Defaults
	name        = NULL;
	shit        = NULL;
	game_id_key = (char *) calloc(14, sizeof(char));
	key_tmp     = NULL;

	// Setup CNDS for keeping track of Game IDs
	game_ids = cn_map_init(char *, char, cn_cmp_cstr);
	cn_map_set_func_destructor(game_ids, destruct_key);

	// Setup file for traversal
	// The first argument without a "-" is the filename.
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;
	}
	fp = fopen(argv[i], "rb");

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
			pos = (pos & 0xFFF00000) | 0x54000;
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

		// Check if duplicate, and only if the flag "-d" isn't specified
		if (!args->flag_allow_dup) {
			// Read in the Game ID (XXXX-YYYYYYYY)
			sprintf(game_id_key, "%.4s-%08X", header.ID, header.N_CRC32);

			// Search for duplicates
			cn_map_find(game_ids, &game_id_it, &game_id_key);

			if (cn_map_at_end(game_ids, &game_id_it)) {
				// No duplicates found. Insert into the CN_Map
				key_tmp = strdup(game_id_key);
				cn_map_insert(game_ids, &key_tmp, &dummy);

				printable = 1;
			}
			else {
				// Duplicate was found. Still process, but don't print.
				printable = 0;

				if (args->flag_warning) {
					fprintf(
						stderr,
						"Warning 0x%08x: Duplicate Game ID \"%.4s-%08X\"\n",
						pos,
						header.ID,
						header.N_CRC32
					);
				}
			}
		}

		// Read bytes segment and check if it's correct
		buf = (unsigned char *)
			malloc(sizeof(unsigned char) * header.offset_strlen - 32);

		fread(&buf[0], sizeof(unsigned char), header.offset_strlen - 32, fp);

		status = verify_code_segment(
			buf,
			header.offset_strlen - 32,
			header.num_codes,
			args,
			&err_at,
			&err_val
		);

		free(buf);

		if (status != 0) {
			fseek(fp, pos + 1, SEEK_SET);
			if (args->flag_error == 1) {
				switch (status) {
					case 1:
						fprintf(
							stderr,
							"Error 0x%08x + 0x%08x: %s (%d)\n",
							pos,
							err_at + 32,
							"Invalid flag was found",
							err_val
						);
						break;

					case 2:
						fprintf(
							stderr,
							"Error 0x%08x + 0x%08x: %s\n",
							pos,
							err_at + 32,
							"Flag inside folder was not a code"
						);
						break;

					case 3:
						fprintf(
							stderr,
							"Error 0x%08x + 0x%08x: %s\n",
							pos,
							err_at + 32,
							"Exceeded buffer size"
						);
						break;

					case 4:
						fprintf(
							stderr,
							"Error 0x%08x + 0x%08x: %s\n",
							pos,
							err_at + 32,
							"More codes than mentioned in header"
						);
						break;

					default:
						fprintf(
							stderr,
							"Error 0x%08x + 0x%08x: %s\n",
							pos,
							err_at + 32,
							"Undocumented error"
						);
						break;
				}
			}
			continue;
		}

		// Read name, if possible
		fseek(fp, pos + header.offset_text + 1, SEEK_SET);

		if (name != NULL) free(name); name = file_read_string(fp);
		if (shit != NULL) free(shit); shit = file_read_string(fp);

		// Print
		if (printable)
			printf("0x%08x - %s - %s\n", pos, game_id_key, name);

		// Skip all codes afterwards
		if (args->flag_skip_name != 1) {
			for (i = 0; i < header.num_codes; i++) {
				if (shit != NULL) free(shit); shit = file_read_string(fp);
				if (shit != NULL) free(shit); shit = file_read_string(fp);
			}
		}
	}

	// We're done here. Clean up
	fclose(fp);

	if (name        != NULL) free(name       );
	if (shit        != NULL) free(shit       );
	if (game_id_key != NULL) free(game_id_key);

	cn_map_free(game_ids);

	return 0;
}

// ----------------------------------------------------------------------------
// Main Function                                                           {{{1
// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
	// Argument check
	if (argc < 2) {
		fprintf(stderr, "usage: %s [-dehnrw] IN_ARDS.nds\n", argv[0]);

		return 1;
	}

	// Parse arguments
	args_t args;

	parse_flags(argc, argv, &args);

	if (args.flag_rescue)
		return data_rescue(argc, argv, &args);
	else
		return data_iterate(argc, argv, &args);

	// Have a nice day, except this shouldn't ever happen
	return 0;
}
