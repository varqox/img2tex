#include "symbol_img_utils.h"

using std::cerr;
using std::endl;
using std::max;

void make_comparison_img(const char* top_img_path, const char* bottom_img_path,
                         const char* out_img_path) {
	auto top_img = teximg_to_matrix(top_img_path);
	auto bottom_img = teximg_to_matrix(bottom_img_path);

	constexpr int SPACING = 3;

	int rows = top_img.rows() + SPACING + bottom_img.rows();
	int cols = max(top_img.cols(), bottom_img.cols());

	Matrix<int> out_img(rows, cols);
	static_assert(SPACING > 0);
	for (int i = 0; i < cols; ++i)
		out_img[top_img.rows()][i] = 1;

	auto copy_img = [&](const Matrix<int>& img, int dr, int dc) {
		for (int r = 0; r < img.rows(); ++r)
			for (int c = 0; c < img.cols(); ++c)
				out_img[r + dr][c + dc] = img[r][c];
	};

	copy_img(top_img, 0, (cols - top_img.cols()) / 2);
	copy_img(bottom_img, top_img.rows() + SPACING,
	         (cols - bottom_img.cols()) / 2);

	save_binary_image_to(out_img, out_img_path);
}

int main2(int argc, char** argv) {
	if (argc != 4) {
		cerr << "Usage: " << argv[0]
		     << " <top_image> <bottom_image> <out_image>\n"
		     << "Draws top_image and bottom_image separated by some blank "
		        "space onto one image out_image";
		return 1;
	}

	make_comparison_img(argv[1], argv[2], argv[3]);
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
