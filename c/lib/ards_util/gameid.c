/*
 * gameid.c
 */

#include "gameid.h"

/*
 * reverse
 *
 * Flips the bits of a 32-bit integer
 * From: http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel
 */

uint32_t reverse(uint32_t v) {
	// swap odd and even bits
	v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
	// swap consecutive pairs
	v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
	// swap nibbles ... 
	v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
	// swap bytes
	v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
	// swap 2-byte long pairs
	v = ( v >> 16             ) | ( v               << 16);

	return v;
}

/*
 * crc32
 *
 * Given a buffer "buffer" of size "len", compute the CRC-32 checksum. Returns
 * a guaranteed 32-bit unsigned integer.
 */

uint32_t crc32(const char *buffer, size_t len) {
	size_t                  i, ib;
	const register uint32_t poly = 0x04C11DB7;
	uint32_t                ret;
	uint32_t                byte;

	ret = 0xFFFFFFFF;

	// For every byte...
	for (i = 0; i < len; i++) {
		byte = reverse(buffer[i]);

		// For every bit...
		for (ib = 0; ib < 8; ib++, byte <<= 1) {
			ret = ((ret ^ byte) >= 0x80000000)
				? (ret << 1) ^ poly
				: (ret << 1);
		}
	}

	return reverse(~ret);
}

/*
 * get_gameid
 *
 * All-in-one function. Given a NDS rom at path "fpath", generate the
 * "XXXX-XXXXXXXX" Game ID string. This will be stored in "out_buffer". This
 * does not call malloc. Define it yourself. Make sure it is at least 14 bytes
 * in length.
 */

void get_gameid(const char *fpath, char *out_buffer) {
	FILE    *fp;
	char    *buffer;
	char     ID_A[5];
	uint32_t ID_B;

	// Null terminator and defaults
	ID_A[4] = 0;
	ID_B    = 0;

	// Read the first 512 bytes of the file
	buffer = (char *) malloc(sizeof(char) * 0x200);

	fp = fopen(fpath, "rb");
	fread(buffer, sizeof(char), 0x200, fp);
	fclose(fp);

	// Get the ID from bytes 0x0C - 0x0F via reinterpret_cast
	*(uint32_t *) &ID_A[0] = *(uint32_t *) &buffer[0x0C];

	// Run a crc32 on the first 512 bytes of the file
	ID_B = crc32(buffer, 0x200);

	// Generate the string
	sprintf(out_buffer, "%s-%08X", ID_A, ~ID_B);
}
