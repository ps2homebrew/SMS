// -*- C++ -*-
// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/datatypes.h"

int
main(int argc, char *argv[]) {
    FILE *fd1, *fd2;
    struct stat st1, st2;
    int size1, size2;
    int dword1, dword2;
    fvec vec1, vec2;
    int i;

    if ( argc < 3 ) {
        printf("<program> <file1> <file2>\n");
        return -1;
    }

    if ( (fd1 = fopen(argv[1], "rb")) == NULL ) {
        printf("Unable to open file: %s\n", argv[1]);
        return -1;
    }
    if ( (fd2 = fopen(argv[2], "rb")) == NULL ) {
        printf("Unable to open file: %s\n", argv[2]);
        return -1;
    }

    stat(argv[1], &st1);
    stat(argv[2], &st2);

    size1 = st1.st_size;
    size2 = st2.st_size;

    if ( size1 != size2 ) {
        printf("filesize mismatch\n");
    }

    // check float registers
    for(i = 0; i < size1; i++) {
        fread(&vec1, sizeof(vec1), 1, fd1);
        fread(&vec2, sizeof(vec2), 1, fd2);
        if ( (vec1.x != vec2.x) ||
            (vec1.y != vec2.y) ||
            (vec1.z != vec2.z) ||
            (vec1.w != vec2.w)) {
            printf("memory mismatch at 0x%x\n", i);
        }
    }
    return 0;
}
