CFLAGS=-Wall -lm -I/usr/local/include -lfftw3 -L/usr/local/lib -ggdb
EXE=fft_gen

all: $(EXE)

$(EXE):	fft_gen.c
	gcc -o $(EXE) fft_gen.c $(CFLAGS)
clean:	
	rm -f $(EXE)
