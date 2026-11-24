# wrapper around original libamp 0.2 djgpp port makefile

PARAMS =

ifdef CROSS
PARAMS += CROSS=1
endif

ifdef Debug
PARAMS += DEBUG=1
endif

ifdef Profile
PARAMS += PROFILE=1
endif

.PHONY: all static clean

all: static

static:
	@echo Building LibAMP...
	@make -f Makefile.dj $(PARAMS)
	@echo Done!

clean:
	-$(RM) obj/* lib/*
