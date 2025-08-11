CONTIKI_PROJECT = aodv-main
PROJECT_SOURCEFILES += aodv.c data.c
all: $(CONTIKI_PROJECT)

CONTIKI = ../../..
CONTIKI_WITH_RIME = 1

include $(CONTIKI)/Makefile.include
