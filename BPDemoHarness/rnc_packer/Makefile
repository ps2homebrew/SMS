all: rnc dernc
clean: 
	rm -rf *.o rnc dernc
CFLAGS=-Wall

rnc: dernc.c rnc.c
	gcc $(CFLAGS) -DCOMPRESSOR -o rnc rnc.c dernc.c
dernc: dernc.c
	gcc $(CLFAGS) -DMAIN -o dernc dernc.c
