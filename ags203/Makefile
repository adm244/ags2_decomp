# make DJGPP_DIR=<path_to_cross_compiler>
# make diff OBJECT=AC.O SYMBOL_INDEX=0 SYMBOL=update_stuff

ifdef DJGPP_DIR
CC = $(DJGPP_DIR)/bin/i586-pc-msdosdjgpp-gcc
CXX = $(DJGPP_DIR)/bin/i586-pc-msdosdjgpp-g++
CROSS = 1
endif

MAKE = make --no-print-directory \
	CC=$(CC) CXX=$(CXX) \
	CROSS=$(CROSS)

OBJECT ?= AC.O
SYMBOL ?= process_event
SYMBOL_INDEX ?= 0

DEPEND_DIR = deps

BIN_DIR = bin
OBJ_DIR = obj
INC_DIR = inc
LIB_DIR = lib
SRC_DIR = src

SRCS = $(filter-out compress.cpp,$(notdir $(wildcard $(SRC_DIR)/*.cpp)))
OBJS = $(SRCS:.cpp=.o)

SRCS_DIR = $(addprefix $(SRC_DIR)/,$(SRCS))
OBJS_DIR = $(addprefix $(OBJ_DIR)/,$(OBJS))

# order matters
LIBS = -lamp -lalleg -ljgmod -lseer

DEPS = \
	$(LIB_DIR)/liballeg.a \
	$(LIB_DIR)/libamp.a \
	$(LIB_DIR)/libjgmod.a \
	$(LIB_DIR)/libseer.a

DEFINES = 
WFLAGS = -Wno-unused -Wno-write-strings -Wno-sign-compare -Wno-char-subscripts

CFLAGS = -W -Wall $(WFLAGS) -I$(INC_DIR) -I$(SRC_DIR) $(DEFINES)
CPPFLAGS = 
LDFLAGS = -L$(LIB_DIR)

ifdef CROSS
WFLAGS += -Wno-unused-parameter -Wno-attributes -Wno-delete-non-virtual-dtor -Wno-empty-body
CFLAGS += -fpermissive -fmax-errors=5
endif

TEMP_DIRS = \
	$(BIN_DIR) $(OBJ_DIR) $(INC_DIR) $(LIB_DIR) \
	$(DEPEND_DIR)/allegro/lib $(DEPEND_DIR)/allegro/obj \
	$(DEPEND_DIR)/libamp/lib $(DEPEND_DIR)/libamp/obj \
	$(DEPEND_DIR)/seer/lib $(DEPEND_DIR)/seer/obj \
	$(DEPEND_DIR)/jgmod/lib $(DEPEND_DIR)/jgmod/obj

.PHONY: all prepare ac allegro jgmod libamp seer diff clean distclean

all: $(BIN_DIR)/ac.exe

prepare:
	@mkdir -p $(TEMP_DIRS)

ac: $(BIN_DIR)/ac.exe
allegro: $(LIB_DIR)/liballeg.a
jgmod: $(LIB_DIR)/libjgmod.a
libamp: $(LIB_DIR)/libamp.a
seer: $(LIB_DIR)/libseer.a

$(BIN_DIR)/ac.exe: $(DEPS) $(OBJS_DIR)
	$(CXX) -o $(BIN_DIR)/ac.exe $(OBJS_DIR) $(LDFLAGS) $(LIBS)
	@echo OK!

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

$(LIB_DIR)/liballeg.a:
	@$(MAKE) -C $(DEPEND_DIR)/allegro
	@cp -f $(DEPEND_DIR)/allegro/lib/liballeg.a $(LIB_DIR)/
	@cp -f $(DEPEND_DIR)/allegro/allegro.h $(INC_DIR)/

$(LIB_DIR)/libjgmod.a:
	@$(MAKE) -C $(DEPEND_DIR)/jgmod
	@cp -f $(DEPEND_DIR)/jgmod/lib/libjgmod.a $(LIB_DIR)/
	@cp -f $(DEPEND_DIR)/jgmod/src/jgmod.h $(INC_DIR)/

$(LIB_DIR)/libamp.a:
	@$(MAKE) -C $(DEPEND_DIR)/libamp
	@cp -f $(DEPEND_DIR)/libamp/lib/libamp.a $(LIB_DIR)/
	@cp -f $(DEPEND_DIR)/libamp/src/libamp.h $(INC_DIR)/

$(LIB_DIR)/libseer.a:
	@$(MAKE) -C $(DEPEND_DIR)/seer
	@cp -f $(DEPEND_DIR)/seer/lib/libseer.a $(LIB_DIR)/
	@cp -f $(DEPEND_DIR)/seer/seer.h $(INC_DIR)/

diff: $(OBJ_DIR)/$(OBJECT)
	@./diff_symbol.sh orig/ac.coff orig/ac_symbols.txt $(OBJ_DIR)/$(OBJECT) $(SYMBOL) $(SYMBOL_INDEX)

clean:
	$(RM) $(BIN_DIR)/* $(OBJ_DIR)/* base.txt target.txt
	@$(MAKE) -C $(DEPEND_DIR)/allegro clean
	@$(MAKE) -C $(DEPEND_DIR)/jgmod clean
	@$(MAKE) -C $(DEPEND_DIR)/libamp clean
	@$(MAKE) -C $(DEPEND_DIR)/seer clean

distclean: clean
	-$(RM) $(INC_DIR)/* $(LIB_DIR)/*
