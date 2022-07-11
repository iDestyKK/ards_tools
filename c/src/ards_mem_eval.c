/*
 * ARDS Memory Evaluator
 *
 * Description:
 *     Given an Action Replay DS ROM dump of exactly 16,777,216 bytes (16 MiB),
 *     evaluate all sections to make sure they are all the same, byte-by-byte.
 *
 * Author:
 *     Clara Nguyen (@iDestyKK)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define CHUNKS   16
#define CHUNK_SZ 0x00100000
#define FILE_SZ  (CHUNK_SZ * CHUNKS)

// ----------------------------------------------------------------------------
// ARDS Checking Functions                                                 {{{1
// ----------------------------------------------------------------------------

/*
 * ards_linear_check
 *
 * Fast and easy. Go through each 1 MiB segment and check if it's the same as
 * the previous one.
 */

int ards_linear_check(uint8_t *segment[CHUNKS]) {
	size_t i;
	int    status;

	for (i = 1; i < CHUNKS; i++) {
		status = memcmp(segment[i - 1], segment[i], CHUNK_SZ);

		if (status != 0)
			return status;
	}

	return status;
}

/*
 * ards_square_check
 *
 * More detailed check. Checks 0-1, 0-2, ... 0-15, 1-2, 1-3, ... 1-15, etc.
 * Prints out a chart of differences.
 */

int ards_square_check(uint8_t *segment[CHUNKS]) {
	size_t i, j;
	int table[CHUNKS][CHUNKS];

	// Top bar
	printf("    | ");
	for (i = 0; i < CHUNKS; i++) printf("  %2d%c", i, (i == 15) ? '\n' : ' ');

	printf("----+-");
	for (i = 0; i < CHUNKS; i++) printf("----%c", (i == 15) ? '\n' : '-');

	// Data
	for (i = 0; i < CHUNKS; i++) {
		// Left column
		printf(" %2d | ", i);

		// Compare chunk i to every possible one (j)...
		for (j = 0; j < CHUNKS; j++) {
			// Only memcmp if we have to
			if (i == j) {
				// Assume they are the same
				table[i][j] = 0;
			}
			else if (i > j) {
				// Skip computed parts (i, j) = -(j, i).
				table[i][j] = -table[j][i];
			}
			else {
				// Compute
				table[i][j] = memcmp(segment[i], segment[j], CHUNK_SZ);
			}

			// Output to table
			printf((j != 15) ? "%4d " : "%4d \n", table[i][j]);
		}
	}
}

// ----------------------------------------------------------------------------
// Basic Argument Parser                                                   {{{1
// ----------------------------------------------------------------------------

typedef struct ARGS_T {
	uint8_t flag_quick_quiet;
} args_t;

void print_help(int argc, char **argv) {
	printf("usage: %s [-hq] IN_ARDS.nds\n", argv[0]);
	printf("Memory evaluation utility for an Action Replay DS ROM "
		"dump.\n\n");

	printf("Optional arguments are:\n\n");

	printf("\t-h\tPrints this help prompt in the terminal and then "
		"terminates.\n\n");

	printf("\t-q\tQuick (and quiet). Instead of O(n^2) memory comparisons, "
		"perform O(n)\n\t\tcomparisons and quit the instant a check fails. "
		"Exit status will be\n\t\tpersistent with the non-quick method.\n\n");

	exit(0);
}

void parse_flags(int argc, char **argv, args_t *obj) {
	int i, j, len;

	// Defaults
	obj->flag_quick_quiet = 0;

	// Go through every argument and read characters
	for (i = 1; i < argc; i++) {
		// Skip if not a valid flag header
		if (argv[i][0] != '-')
			continue;

		len = strlen(argv[i]);
		for (j = 1; j < len; j++) {
			switch (argv[i][j]) {
				case 'q':
					// Allow duplicate games
					obj->flag_quick_quiet = 1;
					break;

				case 'h':
					// Shows help prompt and terminate
					print_help(argc, argv);
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
// Main Function                                                           {{{1
// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
	// Argument check
	if (argc < 2) {
		fprintf(stderr, "usage: %s [-hq] IN_ARDS.nds\n", argv[0]);

		return 1;
	}

	// Parse arguments
	args_t args;

	parse_flags(argc, argv, &args);

	size_t   i;
	size_t   fsz;
	int      status;
	uint8_t *buffer;
	uint8_t *segment[CHUNKS];
	FILE    *fp;

	// Setup file for traversal
	// The first argument without a "-" is the filename.
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;
	}

	fp = fopen(argv[i], "rb");

	if (!fp) {
		fprintf(stderr, "Error: Failed to open file: %s\n", strerror(errno));
		return 3;
	}

	// Check if file is exactly 16 MiB
	fseek(fp, 0, SEEK_END);
	fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (fsz != FILE_SZ) {
		fprintf(
			stderr,
			"Error: File size is not %d bytes (got %d bytes instead)\n",
			FILE_SZ,
			fsz
		);

		fclose(fp);
		return 2;
	}

	// Memory hack
	buffer = (uint8_t *) malloc(FILE_SZ);

	for (i = 0; i < CHUNKS; i++)
		segment[i] = buffer + (CHUNK_SZ * i);

	// Read in all bytes
	fread(buffer, FILE_SZ, 1, fp);

	fclose(fp);

	// Evaluate based on method
	if (args.flag_quick_quiet)
		status = ards_linear_check(segment);
	else
		status = ards_square_check(segment);

	// Clean up
	free(buffer);

	// 0 = All same. 1 = Differences exist.
	return !!status;
}
