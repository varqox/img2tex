#include "commands.h"

#include <cstring>
#include <iostream>

using std::cerr;

int main2(int argc, char** argv) {
	if (argc < 2) {
		cerr << "Usage: " << argv[0] << " <command> [arguments...]\n"
		     <<
		   R"=(Available commands:\n"
  compare <png_file_1> <png_file_2>
                       Compares two png images as symbols
  gen                  Generates symbols database to file symbols.db.
  learn <symbol_file>  Reads symbol from symbol_file and saves it to the
                         symbols database as tex formula that is read from
                         input.
  tex <out_png_file>   Reads tex formula from input and writes PNG image
                         compiled from this formula to the out_png_file.
  untex <png_file> [--save-candidates]
                       Tries to convert png_file to the source tex formula and
                         print the result to the output, otherwise exits with
                         code 1.
)=";
		return 1;
	}

	const char* command = argv[1];
	if (strcmp(command, "compare") == 0)
		return compare_command(argc - 2, argv + 2);
	if (strcmp(command, "gen") == 0)
		return gen_command(argc - 2, argv + 2);
	if (strcmp(command, "learn") == 0)
		return learn_command(argc - 2, argv + 2);
	if (strcmp(command, "tex") == 0)
		return tex_command(argc - 2, argv + 2);
	if (strcmp(command, "untex") == 0)
		return untex_command(argc - 2, argv + 2);

	cerr << "Unknown command: " << command << '\n';
	return 1;
}

int main(int argc, char** argv) {
	try {
		return main2(argc, argv);
	} catch (const std::exception& e) {
		cerr << "Error: " << e.what() << '\n';
		return 1;
	}
}
