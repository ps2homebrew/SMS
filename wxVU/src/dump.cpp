#include <sys/types.h>
#include <sys/stat.h>
#ifdef	__WIN32__
#include <winsock2.h>
#endif
#include "linkproto_stub.h"
#include "vu.h"
#include "Log.h"

#ifndef __WIN32__
static int32 sock = -1;
#else
static SOCKET sock = -1;
#endif

static  int32 dumpWaitForFile(char *file, uint32 size);
static  int32 dumpOpen(void);
static  char cmd[1024];

static int32
dumpOpen(void) {
#ifdef __WIN32__
    WORD wVersionRequested;          /* socket dll version info */ 
    WSADATA wsaData;                 /* data for socket lib initialisation */
#endif

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PKO_SRV_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(&(addr.sin_zero), '\0', 8);
#ifdef __WIN32__
    wVersionRequested = MAKEWORD( 1, 1 );
    if ( WSAStartup( wVersionRequested, &wsaData ) != 0 ) {
        return E_SOCKET;
    }
#endif
    
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
        return E_SOCKET;
    }
    if((connect(sock, (struct sockaddr *)&addr,
                sizeof(struct sockaddr_in))) < 0) {
        return E_NO_LINK;
    }
    return 0;
}

int32
dumpIsOpen(void) {
    return sock;
}

int32
dumpClose(void) {
#ifndef __WIN32__
    return close(sock);
#else
	shutdown(sock, SD_SEND);
    WSACleanup();
    return closesocket(sock);
#endif
}

int32
dumpVU(char *cfile, char *dfile, uint32 vpu) {
    int count;
    FILE *fd;
    struct stat st;
    int file_size = 0;
    unsigned int dmem_offset = 0;
    unsigned int cmem_offset = 0;

    if (dfile == "" && cfile == "") {
        return E_FILE_OPEN;
    }
    if ( dumpOpen() < 0 ) {
        dumpClose();
        return sock;
    }
    if ((fd = fopen(cfile, "wb")) == NULL ) {
        dumpClose();
        return E_FILE_OPEN;
    }
    fclose(fd);
    if ((fd = fopen(dfile, "wb")) == NULL ) {
        dumpClose();
        return E_FILE_OPEN;
    }
    fclose(fd);
    if ( vpu == 0 ) {
        file_size = VU0_SIZE;
        dmem_offset = VU0_DATA_MEM;
        cmem_offset = VU0_CODE_MEM;
    } else {
        file_size = VU1_SIZE;
        dmem_offset = VU1_DATA_MEM;
        cmem_offset = VU1_CODE_MEM;
    }
    count = 0;
    pko_stop_vu(sock, vpu);
    pko_dumpmemory_req(sock, dfile, dmem_offset, file_size);
    while(true) {
        stat(dfile, &st);
        if (st.st_size == file_size) {
            break;
        }
#ifdef __WIN32__
        Sleep(1000);
#else
        usleep(1000);
#endif
        count++;
        if ( count == 50 ) {
            dumpClose();
            return E_TIMEOUT;
        }
    }
    count = 0;
    pko_dumpmemory_req(sock, cfile, cmem_offset, file_size);
    while(true) {
        stat(cfile, &st);
        if ( st.st_size == file_size) {
            break;
        }
#ifdef __WIN32__
        Sleep(1000);
#else
        usleep(1000);
#endif
        count++;
        if ( count == 50 ) {
            dumpClose();
            return E_TIMEOUT;
        }
    }
    pko_start_vu(sock, vpu);
    dumpClose();
    return 0;
}

int32
dumpVURegisters(char *rfile, uint32 vpu) {
    FILE *fd;
    struct stat st;
    int regs, count;
    int file_size;
    if ( rfile == "" ) {
        return E_FILE_OPEN;
    }
    if ( dumpOpen() < 0 ) {
        dumpClose();
        return sock;
    }
    if ( vpu == 0 ) {
        regs = 11;
        file_size = 896;
    } else {
        regs = 12;
        file_size = 896;
    }
    if((fd = fopen(rfile, "wb")) == NULL) {
        dumpClose();
        return E_FILE_OPEN;
    }
    fclose(fd);
    pko_dumpregs_req(sock, rfile, regs);
    count = 0;
    while(true) {
        stat(rfile, &st);
        if ( st.st_size == file_size) {
            break;
        }
#ifdef __WIN32__
        Sleep(1000);
#else
        usleep(1000);
#endif
        count++;
        if ( count == 50 ) {
            dumpClose();
            return E_TIMEOUT;
        }
    }
    dumpClose();
    return 0;
}

int32
dumpRegisters(char *rfile) {
    int count;
    FILE *fd;
    struct stat st;
    int regs = 10;
    int file_size = 508;
    if ( rfile == "" ) {
        return E_FILE_OPEN;
    }
    if ( dumpOpen() < 0 ) {
        dumpClose();
        return sock;
    }
    if ( (fd = fopen(rfile, "wb")) == NULL ) {
        dumpClose();
        return E_FILE_OPEN;
    }
    fclose(fd);
    count = 0;
    pko_dumpregs_req(sock, rfile, regs);
    while(true) {
        stat(rfile, &st);
        if ( st.st_size == file_size) {
            break;
        }
#ifdef __WIN32__
        Sleep(1000);
#else
        usleep(1000);
#endif
        count++;
        if ( count == 50 ) {
            dumpClose();
            return E_TIMEOUT;
        }
    }
    dumpClose();
    return 0;
}

int32
dumpDisplayList(char *file, uint32 *data, uint32 size) {
    FILE *fd;
	int ret;
    if ( file == "" ) {
        return E_FILE_OPEN;
    }

    if ( (fd = fopen(file, "wb")) == NULL ) {
        dumpClose();
        return E_FILE_OPEN;
    }
    fwrite(data, size, 1, fd);
    fclose(fd);
    if ( dumpOpen() < 0 ) {
		dumpClose();
        return sock;
    }
    ret = pko_gsexec_req(sock, file, size);
    dumpClose();
	return 0;
}

// batch tool versions of the dump commands
int32
dumpExecVU(char *arg, char *dfile, char *cfile, uint32 vpu) {
    FILE *fd;
    int file_size = 0;
    unsigned int dmem_offset = 0;
    unsigned int cmem_offset = 0;

    if (dfile == "" && cfile == "") {
        return E_FILE_OPEN;
    }
    if ((fd = fopen(cfile, "wb")) == NULL ) {
        dumpClose();
        return E_FILE_OPEN;
    }
    fclose(fd);
    if ((fd = fopen(dfile, "wb")) == NULL ) {
        dumpClose();
        return E_FILE_OPEN;
    }
    fclose(fd);
    if ( vpu == 0 ) {
        file_size = VU0_SIZE;
        dmem_offset = VU0_DATA_MEM;
        cmem_offset = VU0_CODE_MEM;
    } else {
        file_size = VU1_SIZE;
        dmem_offset = VU1_DATA_MEM;
        cmem_offset = VU1_CODE_MEM;
    }
    sprintf(cmd, arg, dfile, dmem_offset, file_size);
    system(cmd);
    dumpWaitForFile(dfile, file_size);

    sprintf(cmd, cmd, cfile, dmem_offset, file_size);
    system(cmd);
    dumpWaitForFile(cfile, file_size);
    return E_OK;
}

int32
dumpExecVURegisters(char *arg, char *rfile, uint32 vpu) {
    FILE *fd;
    int regs;
    int file_size;
    if ( rfile == "" ) {
        return E_FILE_OPEN;
    }
    if ( dumpOpen() < 0 ) {
        dumpClose();
        return sock;
    }
    if ( vpu == 0 ) {
        regs = 11;
        file_size = 896;
    } else {
        regs = 12;
        file_size = 896;
    }
    if((fd = fopen(rfile, "wb")) == NULL) {
        return E_FILE_OPEN;
    }
    fclose(fd);
    sprintf(cmd, arg, rfile, file_size);
    system(cmd);
    dumpWaitForFile(rfile, file_size);
    return E_OK;
}

int32
dumpExecRegisters(char *arg, char *rfile) {
    FILE *fd;
    int regs = 10;
    int file_size = 508;
    if ( rfile == "" ) {
        return E_FILE_OPEN;
    }
    if ( (fd = fopen(rfile, "wb")) == NULL ) {
        return E_FILE_OPEN;
    }
    fclose(fd);
    sprintf(cmd, arg, rfile, file_size, regs);
    system(cmd);
    dumpWaitForFile(rfile, file_size);
    return E_OK;
}

int32
dumpExecDisplayList(char *arg, char *file, uint32 *data, uint32 size) {
    FILE *fd;
    if ( file == "" ) {
        return E_FILE_OPEN;
    }
    if ( (fd = fopen(file, "wb")) == NULL ) {
        return E_FILE_OPEN;
    }
    fwrite(data, size, 1, fd);
    fclose(fd);
    sprintf(cmd, arg, file, size);
    if ( system(cmd) != 0 ) {
        return E_SYSTEM_CMD;
    }
	return E_OK;
}

static int32
dumpWaitForFile(char *file, uint32 size) {
    struct stat st;
    int count = 0;
    while(true) {
        stat(file, &st);
        if ( st.st_size == size) {
            break;
        }
#ifdef __WIN32__
        Sleep(1000);
#else
        usleep(1000);
#endif
        count++;
        if ( count == 50 ) {
            return E_TIMEOUT;
        }
    }
    return E_OK;
}
