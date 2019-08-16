#include "symbol_database.h"
#include <dirent.h>
#include <filesystem>

using std::cerr;
using std::endl;
using std::map;
using std::pair;
using std::string;
using std::to_string;

template <class Func, class ErrFunc>
void for_each_dir_component(DIR* dir, Func&& func, ErrFunc&& readdir_failed) {
	dirent* file;
	for (;;) {
		errno = 0;
		file = readdir(dir);
		if (file == nullptr) {
			if (errno == 0)
				return; // No more entries

			readdir_failed();
			return;
		}

		if (strcmp(file->d_name, ".") and strcmp(file->d_name, ".."))
			func(file);
	}
}

void unique_files_from_full_main_to_main(const string& src_dir,
                                         string dest_dir) {
	if (not dest_dir.empty() and dest_dir.back() != '/')
		dest_dir += '/';

	auto cmp = [](const Matrix<int>& a, const Matrix<int>& b) {
		return pair(a.rows() * a.cols(),
		            SymbolDatabase::symbol_to_text_img(a)) <
		       pair(b.rows() * b.cols(), SymbolDatabase::symbol_to_text_img(b));
	};

	map<Matrix<int>, string, decltype(cmp)> images(std::move(cmp));
	for_each_dir_component(
	   opendir(src_dir.c_str()),
	   [&](dirent* file) {
		   string path = dest_dir + file->d_name;

		   auto img = teximg_to_matrix(path.data());
		   auto& mapped = images[img];
		   if (mapped.empty())
			   mapped = path;
	   },
	   [] { throw std::runtime_error("readdir() failed"); });

	cerr << images.size() << endl;

	namespace fs = std::filesystem;
	fs::remove_all("main/");
	(void)fs::create_directory("main");
	int i = 0;
	for (auto const& [symbol, path_str] : images) {
		fs::path path = path_str;
		if (not fs::copy_file(path, fs::path("main/") += to_string(++i) +=
		                            path.extension()))
			throw std::runtime_error("file copy failed");
	}
}

int main2(int argc, char** argv) {
	if (argc != 3) {
		cerr << "Usage: " << argv[0] << " <source_dir> <dest_dir>\n"
		     << "Copies files from source_dir and saves them in dest_dir (in "
		        "order of size) after making unique by treating them as symbol "
		        "images.";
		return 1;
	}

	unique_files_from_full_main_to_main(argv[1], argv[2]);
	return 0;
}

int main(int argc, char** argv) {
	try {
		return main2(argc, argv);
	} catch (const std::exception& e) {
		cerr << "Error: " << e.what() << '\n';
		return 1;
	}
}
