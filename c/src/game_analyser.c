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

// ----------------------------------------------------------------------------
// Main Function                                                           {{{1
// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
	// Argument check
	if (argc != 2) {
		fprintf(stderr, "usage: %s in.bin\n");
		return 1;
	}

	//We're done here
	return 0;
}
