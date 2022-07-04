/*
 * io.c
 */

#include "io.h"

// ----------------------------------------------------------------------------
// File Reading Helpers                                                    {{{1
// ----------------------------------------------------------------------------

/*
 * Provides reading functionality for the rest of the functions here. These
 * condense repetitive code down into single functions for code readability.
 */

/*
 * file_read_string                                                        {{{2
 *
 * Reads a string from "fp" until a null-terminator (0x00) is hit. Returns the
 * string to you. This calls "calloc", so you are responsible for freeing the
 * memory afterwards.
 */

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
// Internal Functions                                                      {{{1
// ----------------------------------------------------------------------------

/*
 * Functions for others to use, but not intended to be used by other developers
 * in other files.
 */

/*
 * __tabs                                                                  {{{2
 *
 * Helper function to put in "num" number of tabs in "out". This can be a file
 * or stdout.
 */

void __tabs(FILE *out, size_t num) {
	size_t i;

	for (i = 0; i < num; i++)
		fprintf(out, "\t");
}

// ----------------------------------------------------------------------------
// ARDS Read Functions                                                     {{{1
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

// ----------------------------------------------------------------------------
// ARDS Output Functions                                                   {{{1
// ----------------------------------------------------------------------------

/*
 * Provides export functionality to different formats. XML, JSON, basic output.
 * Can even be for writing a byte format compatible with ARDS in the future.
 */

/*
 * library_dump_as_xml                                                     {{{2
 *
 * Exports the game's Action Replay code list as an XML file.
 */

void library_dump_as_xml(
	FILE          *out,
	CN_VEC         root,
	const char    *name,
	ar_game_info_t header
) {
	// XML Begin
	fprintf(out, "<?xml version = \"1.0\" encoding = \"UTF-8\"?>\n");
	fprintf(out, "<codelist>\n");
	fprintf(
		out,
		"\t<name>%s</name>\n",
		"Extracted via CN_ARDS - ards_game_to_xml"
	);

	// Game Header
	fprintf(out, "\t<game>\n");
	fprintf(out, "\t\t<name>%s</name>\n", name);
	fprintf(
		out,
		"\t\t<gameid>%.4s %08X</gameid>\n",
		header.ID,
		header.N_CRC32
	);

	// Date, if possible
	if (header.wDosDate != 0 && header.wDosTime != 0) {
		fprintf(
			out,
			"\t\t<date>%04d/%02d/%02d %02d:%02d</date>\n",
			(header.wDosDate >>  9) + 1980,
			(header.wDosDate >>  5) & 0xF,
			(header.wDosDate      ) & 0x1F,
			(header.wDosTime >> 11),
			(header.wDosTime >>  5) & 0x3F
		);
	}

	// Recursion
	library_dump_as_xml_rec(out, root, 0);

	// Closing
	fprintf(out, "\t</game>\n");
	fprintf(out, "</codelist>\n");
}

/*
 * library_dump_as_xml_rec                                                 {{{2
 *
 * Recursive helper to "library_dump_as_xml". Goes through the code/folder tree
 * and exports everything to XML.
 */

void library_dump_as_xml_rec(FILE *out, CN_VEC root, size_t depth) {
	ar_data_t *it;
	ar_line_t *lt;
	size_t i;

	cn_vec_traverse(root, it) {
		if (it->data != NULL) {
			switch ((ar_flag_t) it->flag & 0xFF) {
				case AR_FLAG_CODE:
					__tabs(out, depth + 2);
					fprintf(out, "<cheat>\n");

					__tabs(out, depth + 3);
					fprintf(out, "<name>%s</name>\n", it->name);

					// If there is a note, add that too
					if (strlen(it->desc) > 0) {
						__tabs(out, depth + 3);
						fprintf(out, "<note>%s</note>\n", it->desc);
					}

					// Print out all lines of the AR code
					__tabs(out, depth + 3);
					fprintf(out, "<codes>");

					i = 0;
					cn_vec_traverse(it->data, lt) {
						fprintf(
							out,
							"%08X %08X%s",
							lt->memory_location,
							lt->value,
							(i + 1 == cn_vec_size(it->data)) ? "" : " "
						);

						i++;
					}

					fprintf(out, "</codes>\n");

					__tabs(out, depth + 2);
					fprintf(out, "</cheat>\n");
					break;

				case AR_FLAG_FOLDER1:
				case AR_FLAG_FOLDER2:
					__tabs(out, depth + 2);
					fprintf(out, "<folder>\n");

					__tabs(out, depth + 3);
					fprintf(out, "<name>%s</name>\n", it->name);

					// If there is a note, add that too
					if (strlen(it->desc) > 0) {
						__tabs(out, depth + 3);
						fprintf(out, "<note>%s</note>\n", it->desc);
					}

					// AR Folders are recursive
					library_dump_as_xml_rec(out, it->data, depth + 1);

					__tabs(out, depth + 2);
					fprintf(out, "</folder>\n");
					break;

				case AR_FLAG_TERMINATE:
				default:
					break;
			}
		}
	}
}

// ----------------------------------------------------------------------------
// Cleanup Functions                                                       {{{1
// ----------------------------------------------------------------------------

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
