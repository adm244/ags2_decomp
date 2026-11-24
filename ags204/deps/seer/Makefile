# wrapper around original SeeR 0.95a makefile

PARAMS =

ifdef Debug
PARAMS += Debug=1
else
PARAMS += Optimize=1
endif

.PHONY: all static clean

all: static

static:
	@echo Building SeeR...
	@make -f Makefile.all $(PARAMS)
	@echo Done!

clean:
	-$(RM) obj/* lib/*
