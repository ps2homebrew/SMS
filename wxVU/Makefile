.SUFFIXES:.cpp .cc .asm .vsm
VPATH = src:utils:obj

CFLAGS = -g `wx-config --cflags`
CFLAGS += -W -Wall -Wpointer-arith -Wcast-align
CFLAGS += -Wsign-compare
CFLAGS += -Wmissing-noreturn
CFLAGS += -Wchar-subscripts -Wformat-security

LIBS = `wx-config --libs` -lc_r
CPP = g++
RM = rm -f
DVP = dvp-elf-as
OBJCOPY = ee-objcopy
OBJECTS = main.o lower.o parser.o upper.o util.o vu.o \
		gif.o prefdlg.o prefs.o dump.o linkproto_stub.o \
		vif.o timer.o sif.o dma.o vuBreakDialog.o breakpoint.o debug.o

FILES = Makefile Makefile.WIN32 TODO src/datatypes.h src/defsext.h src/dma.cpp src/dma.h \
	src/dump.cpp src/dump.h src/errors.h src/gif.cpp src/gif.h src/instructions.txt src/linkproto_stub.cpp \
	src/linkproto_stub.h src/lower.cpp src/main.cpp src/main.h src/opcodes.h src/parser.cpp src/parser.h \
	src/prefdef.h src/prefdlg.cpp src/prefdlg.h src/prefs.cpp src/prefs.h src/sif.cpp src/sif.h src/timer.cpp \
	src/timer.h src/upper.cpp src/util.cpp src/util.h src/vif.cpp src/vif.h \
	src/vu.cpp src/vu.h src/vuBreakDialog.cpp src/vuBreakDialog.h \
	src/breakpoint.cpp src/breakpoint.h

ZIP = zip -9

all: wxVU run

wxVU: $(OBJECTS)
	@cd obj; \
	$(CPP) $(CFLAGS) -o ../bin/wxVU $(OBJECTS) $(LIBS)

run:
	./bin/wxVU

memcmp: memcmp.cpp
	$(CPP) -o bin/memcmp utils/memcmp.cpp -lc
regcmp: regcmp.cpp
	$(CPP) -o bin/regcmp utils/regcmp.cpp -lc

.cpp.o :
	$(CPP) $(CFLAGS) -c $< -o obj/$@

.vsm.o :
	$(DVP) $< -o $@
	$(OBJCOPY) -O binary $@ $*.bin

clean:
	$(RM) obj/*.o
	$(RM) bin/wxVU
	$(RM) bin/regcmp
	$(RM) bin/memcmp

dist:
	$(ZIP) wxVU.zip $(FILES)

test:
	g++ -g `wx-config --cflags` -c src/breakpoint.cpp -o obj/breakpoint.o
	g++ -g `wx-config --cflags` -c src/vuBreakDialog.cpp -o obj/vuBreakDialog.o
	g++ -g `wx-config --cflags` -o bin/testWidget src/testWidget.cpp obj/vuBreakDialog.o obj/breakpoint.o `wx-config --libs` -lc_r
	./bin/testWidget

testvif: vif.o testvif.o
	g++ -g -o bin/testvif obj/vif.o obj/testvif.o

testdma: dma.o testdma.o
	g++ -g -o bin/testdma obj/dma.o obj/testdma.o
