#include "datatypes.h"

int32 dumpIsOpen(void);
int32 dumpClose(void);
int32 dumpVU(char *cfile, char *dfile, uint32 vpu);
int32 dumpVURegisters(char *file, uint32 vpu);
int32 dumpRegisters(char *file);
int32 dumpDisplayList(char *file, uint32 *data, uint32 size);

int32 dumpExecVU(char *arg, char *dfile, char *cfile, uint32 vpu);
int32 dumpExecVURegisters(char *arg, char *rfile, uint32 vpu);
int32 dumpExecRegisters(char *arg, char *rfile);
int32 dumpExecDisplayList(char *arg, char *file, uint32 *data, uint32 size);
