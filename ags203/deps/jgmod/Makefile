# wrapper around modified JGMOD 0.99 DJGPP makefile

PARAMS =

ifdef CROSS
PARAMS += CROSS=1
endif

.PHONY: all static clean

all: static

static:
	@echo Building JGMOD...
	@make -f Makefile.dj lib $(PARAMS)
	@echo Done!

clean:
	-$(RM) obj/* lib/*
