# wrapper around modified JGMOD 0.99 makefile

PARAMS =

ifdef DEBUG
PRARMS += Debug=1
endif

ifdef CROSS
PARAMS += CROSS=1
endif

.PHONY: all static clean

all: static

static:
	@echo Building JGMOD...
	@make -f Makefile.all lib $(PARAMS)
	@echo Done!

clean:
	-$(RM) obj/* lib/*
