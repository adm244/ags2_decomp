# wrapper around modified allegro makefile for djgpp

PARAMS = CC=$(CC) CXX=$(CXX)

ifdef Debug
PARAMS += DEBUGMODE=1
endif

.PHONY: all mmxtest static clean

all: static

mmxtest:
	@make -f Makefile.dj mmxtest

static:
	@echo Building Allegro...
	@make -f Makefile.dj $(PARAMS)
	@echo Done!

clean:
	-$(RM) obj/* lib/*
