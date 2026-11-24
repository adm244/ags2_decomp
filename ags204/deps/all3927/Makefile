# wrapper around modified allegro makefile

PARAMS =

ifdef Debug
PARAMS += DEBUGMODE=1
endif

ifdef Static
PARAMS += STATICLINK=1
endif

ifdef Warnmode
PARAMS += WARNMODE=1
endif

.PHONY: all allegro clean

all: allegro

allegro:
	@echo Building Allegro...
	@make -f Makefile.all $(PARAMS)
	@echo Done!

clean:
	-$(RM) obj/* lib/*
