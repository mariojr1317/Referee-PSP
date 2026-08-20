TARGET = ArbitroPSP
OBJS = main.o audio.o graphics.o

BUILD_PRX = 1
PSP_FW_VERSION = 500

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = -lpspgu -lpspgum -lpng -lz -lpspaudio -lpspmp3 -lpsputility -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Simulador de Arbitro PSP

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
