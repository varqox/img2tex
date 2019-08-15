include makefile-utils/Makefile.config

IMG2TEX_CXX_FLAGS := -isystem /usr/include/opencv4
IMG2TEX_LD_FLAGS := -lopencv_core -lopencv_imgcodecs -pthread

.PHONY: all
all: img2tex unique_symbol_img_files
	@printf "\033[32mBuild finished\033[0m\n"

IMG2TEX_SRCS := \
	src/commands.cc \
	src/img2tex.cc \
	src/symbol_img_utils.cc \
	src/symbols_to_tex.cc \

$(eval $(call load_dependencies, $(IMG2TEX_SRCS)))
IMG2TEX_OBJS := $(call SRCS_TO_OBJS, $(IMG2TEX_SRCS))

img2tex: $(IMG2TEX_OBJS)
	$(LINK)

UNIQ_SYMBOL_IMG_FILES_SRCS := \
	src/unique_symbol_img_files.cc \

$(eval $(call load_dependencies, $(UNIQ_SYMBOL_IMG_FILES_SRCS)))
UNIQ_SYMBOL_IMG_FILES_OBJS := $(call SRCS_TO_OBJS, $(UNIQ_SYMBOL_IMG_FILES_SRCS))

unique_symbol_img_files: $(UNIQ_SYMBOL_IMG_FILES_OBJS)
	$(LINK)

IMG2TEX_ALL_OBJS := $(IMG2TEX_OBJS) $(UNIQ_SYMBOL_IMG_FILES_OBJS)
IMG2TEX_EXECS := img2tex unique_symbol_img_files

$(IMG2TEX_ALL_OBJS): override EXTRA_CXX_FLAGS += $(IMG2TEX_CXX_FLAGS)
$(IMG2TEX_EXECS): private override EXTRA_LD_FLAGS += $(IMG2TEX_LD_FLAGS)

.PHONY: format
format: $(shell find src | grep -E '\.(cc?|h)$$' | sed 's/$$/-make-format/')

.PHONY: clean
clean: OBJS := $(IMG2TEX_ALL_OBJS)
clean:
	$(Q)$(RM) $(IMG2TEX_EXECS) $(OBJS) $(OBJS:o=dwo)
	$(Q)find src -type f -name '*.deps' | xargs rm -f

.PHONY: help
help:
	@echo "Nothing is here yet..."
