BASE=..
include $(BASE)/Makefile.header

PROGRAMS = sh

sh_SOURCES = sh.c
sh_LDADD   = -llwk -lpthread

sh_SOURCES = sh.c

# Embed the smartmap_app ELF executable in the loader's ELF image
#sh_RAWDATA = sh
#CC_LDFLAGS += -Wl,--section-start -Wl,.rawdata=0x1000000

include $(BASE)/Makefile.footer
