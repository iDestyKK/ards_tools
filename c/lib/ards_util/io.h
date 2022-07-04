/*
 * ARDS Utils - I/O
 *
 * Description:
 *     Provides I/O functionality. Reading bytes and writing. Also provides
 *     essential structs and data types for other include files to work.
 *
 * Author:
 *     Clara Nguyen (@iDestyKK)
 */

#ifndef __ARDS_UTILS_IO__
#define __ARDS_UTILS_IO__

// C Includes
#include <stdio.h>
#include <stdint.h>

// CNDS
#include "../CN_Vec/cn_vec.h"

// ----------------------------------------------------------------------------
// ARDS Data Structs                                                       {{{1
// ----------------------------------------------------------------------------

/*
 * Structs for helping read data from the ARDS ROM dump
 */

/*
 * AR_GAME_INFO_T
 *
 * The first 32 bytes of a game's code section.
 */

typedef struct AR_GAME_INFO_T {
	                          // Bytes    Description
	                          // ---      ---
	uint32_t magic;           // 00 - 03. Always "01 00 1C 00".
	uint16_t num_codes;       // 04 - 05. Number of codes present
	uint16_t nx20;            // 06 - 07. Always "20 00".
	uint32_t code_bytes_size; // 08 - 11. Bytes between game_info_t to text - 1
	uint32_t idk1;            // 12 - 15
	uint16_t wDosDate;        // 16 - 17. DOS date (?)
	uint16_t wDosTime;        // 18 - 19. DOS time (?)
	char     ID[4];           // 20 - 23. 4 characters that appear on cartridge
	uint32_t idk2;            // 24 - 27
	uint32_t N_CRC32;         // 28 - 31. ~CRC32(first 512 bytes of ROM)
} ar_game_info_t;

/*
 * AR_LINE_T
 *
 * A 64-bit type that stores a single line of an Action Replay code.
 */

typedef struct AR_LINE_T {
	uint32_t memory_location; // Left side of AR code
	uint32_t value;           // Right side of AR code
} ar_line_t;

/*
 * AR_FLAG_T
 *
 * Enum for determining what value a flag is set to. Not used in any structs.
 * Instead, used in implementation, as it makes code easier to read.
 */

typedef enum AR_FLAG_T {
	AR_FLAG_TERMINATE = 0,
	AR_FLAG_CODE      = 1,
	AR_FLAG_FOLDER1   = 2,
	AR_FLAG_FOLDER2   = 6
} ar_flag_t;

/*
 * AR_DATA_T
 *
 * A single Action Replay code. This can store a name, note (desc), and any
 * number of lines of an Action Replay code. Can also be used as a folder type,
 * as a CN_VEC allows for type specification upon initialisation.
 */

typedef struct AR_DATA_T {
	uint16_t   flag;            // Good luck getting that ENUM to work here
	uint16_t   num_entries;     // A line is 8 bytes, or "XXXXXXXX XXXXXXXX"
	char      *name;            // Name of cheat/folder/whatever
	char      *desc;            // Description
	CN_VEC     data;            // Abuse CN_Vec for type-agnosticism via flag
} ar_data_t;

// ----------------------------------------------------------------------------
// UTIL Data Structs                                                       {{{1
// ----------------------------------------------------------------------------

/*
 * Structs for the rest of the functions to work. Provides data types to allow
 * for a more object-oriented development.
 */

// ----------------------------------------------------------------------------
// File Reading Helpers                                                    {{{1
// ----------------------------------------------------------------------------

/*
 * Provides reading functionality for the rest of the functions here. These
 * condense repetitive code down into single functions for code readability.
 */

// Read data as specific type
#define file_read_type(fp, type, var) \
	fread(&var, sizeof(type), 1, fp)

char *file_read_string(FILE *);

// ----------------------------------------------------------------------------
// Internal Functions                                                      {{{1
// ----------------------------------------------------------------------------

/*
 * Functions for others to use, but not intended to be used by other developers
 * in other files.
 */

void __tabs(FILE *, size_t);

// ----------------------------------------------------------------------------
// ARDS Read Functions                                                     {{{1
// ----------------------------------------------------------------------------

void file_read_cheats_and_folders(FILE *, CN_VEC, uint16_t, uint8_t);
void file_read_names(FILE *, CN_VEC);

// ----------------------------------------------------------------------------
// ARDS Output Functions                                                   {{{1
// ----------------------------------------------------------------------------

/*
 * Provides export functionality to different formats. XML, JSON, basic output.
 * Can even be for writing a byte format compatible with ARDS in the future.
 */

// XML Export Functionality
void library_dump_as_xml    (FILE *, CN_VEC, const char *, ar_game_info_t);
void library_dump_as_xml_rec(FILE *, CN_VEC, size_t);

// ----------------------------------------------------------------------------
// Cleanup Functions                                                       {{{1
// ----------------------------------------------------------------------------

/*
 * Provides cleanup for other functions here.
 */

void root_obliterate(CN_VEC);

#endif
