#ifndef _AIO_FIO_H
#define _AIO_FIO_H

//
// The following functions are just a quick hack on top of the AIO
// code to help getting zlib working (zlib expects standard fio functions -
// not AIO).
//

#ifdef __cplusplus
extern "C" {

void aioSetCurrent(abstractIO *currentAIO);
abstractIO *aioGetCurrent();

#endif

int aioOpen(const char *name, int flags);
int aioClose(int fd);
int aioRead(int fd, unsigned char *buffer, int size);
int aioWrite(int fd, const unsigned char *buffer, int size);
int aioLseek(int fd, int offset, int whence);
int aioRemove(const char *name);
int aioRename(const char *old, const char *newname);
int aioMkdir(const char *name);
int aioRmdir(const char *name);
//int aioGetdir(const char *name, const char *extensions, t_aioDent dentBuf[], int maxEnt);
//int aioGetstat(const char *name, t_aioDent *dent);

#ifdef __cplusplus
}
#endif

#endif /* _AIO_FIO_H */
