#!/bin/sh
for FILE in "IDCT" "CSC1" "CSC2h" "CSC2v" "CSC4"; do

	rm $FILE.o.xz $FILE.dmp
	xz --lzma2=nice=32 --check=crc32 --keep $FILE.o
	bin2c $FILE.o.xz $FILE.dmp s_$FILE

done
