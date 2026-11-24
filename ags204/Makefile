# make DJGPP_DIR=<path_to_cross_compiler>
# make diff OBJECT=AC.O SYMBOL_INDEX=0 SYMBOL=update_stuff
# make diff TOOLCHAIN=msvc OBJECT=ac.obj SYMBOL=update_stuff
# make MSVC_DIR=<path_to_msvc_env>
# WINEPREFIX=<path_prefix> make MSVC_DIR=<path_to_msvc_env>

OBJECT ?= AC.O
SYMBOL ?= process_event
SYMBOL_INDEX ?= 0

BIN_DIR = bin
OBJ_DIR = obj
INC_DIR = inc
LIB_DIR = lib
SRC_DIR = src
DEP_DIR = deps

ifndef TOOLCHAIN
ifdef MSVC_DIR
TOOLCHAIN = msvc
else
TOOLCHAIN = djgpp
endif # MSVC_DIR
endif # TOOLCHAIN

MAKEFILE_INC = Makefile.dj
ifeq ($(strip $(TOOLCHAIN)),msvc)
MAKEFILE_INC = Makefile.vc
endif

.PHONY: default
default: all

# platform makefile defines: TARGET, BASE_BIN, BASE_SYMBOLS
include $(MAKEFILE_INC)

TEMP_DIRS = \
	$(BIN_DIR) $(OBJ_DIR) $(INC_DIR) $(LIB_DIR) \
	$(DEP_DIR)/all3927/lib $(DEP_DIR)/all3927/obj \
	$(DEP_DIR)/allegro/lib $(DEP_DIR)/allegro/obj \
	$(DEP_DIR)/libamp/lib $(DEP_DIR)/libamp/obj \
	$(DEP_DIR)/seer/lib $(DEP_DIR)/seer/obj \
	$(DEP_DIR)/jgmod/lib $(DEP_DIR)/jgmod/obj

.PHONY: all prepare ac diff clean distclean

all: ac

prepare:
	@mkdir -p $(TEMP_DIRS)

ac: $(TARGET)
	@echo OK!

diff: $(OBJ_DIR)/$(OBJECT)
	@./diff_symbol.sh $(BASE_BIN) $(BASE_SYMBOLS) $(OBJ_DIR)/$(OBJECT) $(SYMBOL) $(SYMBOL_INDEX)

clean:
	$(RM) $(BIN_DIR)/* $(OBJ_DIR)/* base.txt target.txt
	@$(MAKE) -C $(DEP_DIR)/all3927 clean
	@$(MAKE) -C $(DEP_DIR)/allegro clean
	@$(MAKE) -C $(DEP_DIR)/jgmod clean
	@$(MAKE) -C $(DEP_DIR)/libamp clean
	@$(MAKE) -C $(DEP_DIR)/seer clean

distclean: clean
	-$(RM) -r $(INC_DIR)/* $(LIB_DIR)/*
