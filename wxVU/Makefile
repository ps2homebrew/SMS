SUFFIXES : .cpp .cc .asm
VPATH = src:utils:obj

CFLAGS = -g `wx-config --cflags`
CFLAGS += -W -Wall -Wpointer-arith -Wcast-align
CFLAGS += -Wsign-compare
CFLAGS += -Wmissing-noreturn
CFLAGS += -Wchar-subscripts -Wformat-security

LIBS = `wx-config --libs` -lc_r
CPP = c++
RM = rm -f
OBJECTS = main.o lower.o parser.o upper.o util.o vu.o \
		gif.o prefdlg.o prefs.o dump.o linkproto_stub.o \
		vif.o timer.o sif.o dma.o

FILES = Makefile Makefile.WIN32 TODO src/datatypes.h src/defsext.h src/dma.cpp src/dma.h \
	src/dump.cpp src/dump.h src/errors.h src/gif.cpp src/gif.h src/instructions.txt src/linkproto_stub.cpp \
	src/linkproto_stub.h src/lower.cpp src/main.cpp src/main.h src/opcodes.h src/parser.cpp src/parser.h \
	src/prefdef.h src/prefdlg.cpp src/prefdlg.h src/prefs.cpp src/prefs.h src/sif.cpp src/sif.h src/timer.cpp \
	src/timer.h src/upper.cpp src/util.cpp src/util.h src/vif.cpp src/vif.h src/vu.cpp src/vu.h

ZIP = zip -9

all: vuemu run

wxVU: $(OBJECTS)
	@cd obj; \
	$(CPP) $(CFLAGS) -o ../bin/vuemu $(OBJECTS) $(LIBS)

run:
	./bin/vuemu

memcmp: memcmp.cpp
	$(CPP) -o bin/memcmp utils/memcmp.cpp -lc
regcmp: regcmp.cpp
	$(CPP) -o bin/regcmp utils/regcmp.cpp -lc

.cpp.o :
	$(CPP) $(CFLAGS) -c $< -o obj/$@

clean:
	$(RM) obj/*.o
	$(RM) bin/vuemu
	$(RM) bin/regcmp
	$(RM) bin/memcmp

dist:
	$(ZIP) wxVU.zip $(FILES)
