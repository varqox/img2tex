include makefile-utils/Makefile.config

define IMG2TEX_FLAGS =
INTERNAL_EXTRA_CXX_FLAGS = -isystem /usr/include/opencv4
INTERNAL_EXTRA_LD_FLAGS = -lopencv_core -lopencv_imgcodecs -pthread
endef

.PHONY: all
all: img2tex unique_symbol_img_files make_comparison_img
	@printf "\033[32mBuild finished\033[0m\n"

$(eval $(call add_executable, img2tex, $(IMG2TEX_FLAGS), \
	src/commands.cc \
	src/img2tex.cc \
	src/symbol_img_utils.cc \
	src/improve_tex.cc \
	src/untex_img.cc \
))

$(eval $(call add_executable, unique_symbol_img_files, $(IMG2TEX_FLAGS), \
	src/unique_symbol_img_files.cc \
))

$(eval $(call add_executable, make_comparison_img, $(IMG2TEX_FLAGS), \
	src/make_comparison_img.cc \
))

.PHONY: format
format: $(shell find src | grep -E '\.(cc?|h)$$' | sed 's/$$/-make-format/')

.PHONY: help
help:
	@echo "Nothing is here yet..."
