# wrapper around original SeeR 0.95a makefile

PARAMS = OBJ_PATH=obj
EXTRA_PARAMS = DEST_OS=DOS DEST_CPU=INTEL DEST_CC=GCC DEST_DIR=linux

ifdef Debug
PARAMS += Debug=1
else
PARAMS += Optimize=1
endif

.PHONY: all static clean

all: static

static:
	@echo Building SeeR...
	@make -f Makefile.all $(PARAMS) $(EXTRA_PARAMS)
	@echo Done!

clean:
	-$(RM) obj/* lib/*
