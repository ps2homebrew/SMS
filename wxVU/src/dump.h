#include "datatypes.h"

int32 dumpIsOpen(void);
int32 dumpClose(void);
int32 dumpVU(char *cfile, char *dfile, uint32 vpu);
int32 dumpVURegisters(char *file, uint32 vpu);
int32 dumpRegisters(char *file);
int32 dumpDisplayList(char *file, uint32 *data, uint32 size);
