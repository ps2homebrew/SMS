#include <sys/types.h>
#include <sys/stat.h>
#ifdef	__WIN32__
#include <winsock2.h>
#endif
#include "linkproto_stub.h"
#include "Remote.h"
#include "Log.h"
#include "vu.h"

#ifndef __WIN32__
static int32 sock = -1;
#else
static SOCKET sock = -1;
#endif

char    Remote::m_vuCodeFilename[256];
char    Remote::m_vuDataFilename[256];
char    Remote::m_vuRegFilename[256];
char    Remote::m_gsFilename[256];
char    Remote::m_miscFilename[256];
uint32  Remote::m_tagGsInit[24];
uint32  Remote::m_tagGsClear[24];

/////////////////////////////// PUBLIC ///////////////////////////////////////
const int32
Remote::GetMiscRegisters(void) {
    int count;
    FILE *fd;
    struct stat st;
    int regs = 10;
    int file_size = 508;
    if ( m_vuRegFilename == "" ) {
        return E_FILE_OPEN;
    }
    if ( (fd = fopen(m_miscFilename, "wb")) == NULL ) {
        fclose(fd);
        return E_FILE_OPEN;
    }
    if ( Open() < 0 ) {
        fclose(fd);
        Close();
        return sock;
    }
    count = 0;
    pko_dumpregs_req(sock, m_miscFilename, regs);
    while(true) {
        stat(m_miscFilename, &st);
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
            Close();
            return E_TIMEOUT;
        }
    }
    Close();
    return E_OK;
}

const int32
Remote::GsExec(const unsigned char* buffer, const uint32 size) {
    FILE *fd;
	int ret;
    if ( m_gsFilename == "" ) {
        return E_FILE_OPEN;
    }
    if ( (fd = fopen(m_gsFilename, "wb")) == NULL ) {
        fclose(fd);
        return E_FILE_OPEN;
    }
    fwrite(buffer, size, 1, fd);
    fclose(fd);
    if ( Open() < 0 ) {
		Close();
        return sock;
    }
    ret = pko_gsexec_req(sock, m_gsFilename, size);
    Close();
    return E_OK;
}

const int32
Remote::GsInit(void) {
    return GsExec((const unsigned char*)m_tagGsInit, 24);
}

const int32
Remote::GsSetColor(void) {
    return GsExec((const unsigned char*)m_tagGsClear, 24);
}

const int32
Remote::GetVu(const uint32 vpu) {
    int count;
    FILE *fd;
    struct stat st;
    int file_size = 0;
    unsigned int dmem_offset = 0;
    unsigned int cmem_offset = 0;

    if (m_vuDataFilename == "" && m_vuCodeFilename == "") {
        return E_FILE_OPEN;
    }
    if ( Open() < 0 ) {
        Close();
        return sock;
    }
    if ((fd = fopen(m_vuCodeFilename, "wb")) == NULL ) {
        Close();
        return E_FILE_OPEN;
    }
    fclose(fd);
    if ((fd = fopen(m_vuDataFilename, "wb")) == NULL ) {
        Close();
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
    pko_dumpmemory_req(sock, m_vuDataFilename, dmem_offset, file_size);
    while(true) {
        stat(m_vuDataFilename, &st);
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
            Close();
            return E_TIMEOUT;
        }
    }
    count = 0;
    pko_dumpmemory_req(sock, m_vuCodeFilename, cmem_offset, file_size);
    while(true) {
        stat(m_vuCodeFilename, &st);
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
            Close();
            return E_TIMEOUT;
        }
    }
    pko_start_vu(sock, vpu);
    Close();
    return E_OK;
}

const int32
Remote::GetVuRegisters(const uint32 vpu) {
    FILE *fd;
    struct stat st;
    int regs, count;
    int file_size;
    if ( m_vuRegFilename == "" ) {
        return E_FILE_OPEN;
    }
    if ( Open() < 0 ) {
        Close();
        return sock;
    }
    if ( vpu == 0 ) {
        regs = 11;
        file_size = 896;
    } else {
        regs = 12;
        file_size = 896;
    }
    if((fd = fopen(m_vuRegFilename, "wb")) == NULL) {
        Close();
        return E_FILE_OPEN;
    }
    fclose(fd);
    pko_dumpregs_req(sock, m_vuRegFilename, regs);
    count = 0;
    while(true) {
        stat(m_vuRegFilename, &st);
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
            Close();
            return E_TIMEOUT;
        }
    }
    Close();
    return 0;
}

void
Remote::SetTmpFiles(const char* dfile, const char *cfile, const char* rfile,
    const char* mfile, const char* gfile) {
    strncpy(m_vuDataFilename, dfile, 256);
    strncpy(m_vuCodeFilename, cfile, 256);
    strncpy(m_vuRegFilename, rfile, 256);

    strncpy(m_miscFilename, mfile, 256);

    strncpy(m_gsFilename, gfile, 256);

}

/////////////////////////////// PRIVATE ///////////////////////////////////////
const int32
Remote::Open(void) {
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

const int32
Remote::Close(void) {
#ifndef __WIN32__
    return close(sock);
#else
	shutdown(sock, SD_SEND);
    WSACleanup();
    return closesocket(sock);
#endif
}

void
Remote::SetGsInit(const uint32 xOffset, const uint32 yOffset, const uint32
    xScissor, const uint32 yScissor, const uint32 rgba) {

    m_tagGsInit[12] = yOffset<<4;
    m_tagGsInit[13] = xOffset<<4;
    m_tagGsInit[16] = xScissor<<16;
    m_tagGsInit[17] = yScissor<<16;
    m_tagGsClear[15] = rgba;
    return;
}

// // batch tool versions of the dump commands
// int32
// dumpExecVU(char *arg, char *m_vuDataFilename, char *m_vuCodeFilename, uint32 vpu) {
//     FILE *fd;
//     int file_size = 0;
//     unsigned int dmem_offset = 0;
//     unsigned int cmem_offset = 0;

//     if (m_vuDataFilename == "" && m_vuCodeFilename == "") {
//         return E_FILE_OPEN;
//     }
//     if ((fd = fopen(m_vuCodeFilename, "wb")) == NULL ) {
//         Close();
//         return E_FILE_OPEN;
//     }
//     fclose(fd);
//     if ((fd = fopen(m_vuDataFilename, "wb")) == NULL ) {
//         Close();
//         return E_FILE_OPEN;
//     }
//     fclose(fd);
//     if ( vpu == 0 ) {
//         file_size = VU0_SIZE;
//         dmem_offset = VU0_DATA_MEM;
//         cmem_offset = VU0_CODE_MEM;
//     } else {
//         file_size = VU1_SIZE;
//         dmem_offset = VU1_DATA_MEM;
//         cmem_offset = VU1_CODE_MEM;
//     }
//     sprintf(cmd, arg, m_vuDataFilename, dmem_offset, file_size);
//     system(cmd);
//     dumpWaitForFile(m_vuDataFilename, file_size);

//     sprintf(cmd, cmd, m_vuCodeFilename, dmem_offset, file_size);
//     system(cmd);
//     dumpWaitForFile(m_vuCodeFilename, file_size);
//     return E_OK;
// }

// int32
// dumpExecVURegisters(char *arg, char *m_vuRegFilename, uint32 vpu) {
//     FILE *fd;
//     int regs;
//     int file_size;
//     if ( m_vuRegFilename == "" ) {
//         return E_FILE_OPEN;
//     }
//     if ( Open() < 0 ) {
//         Close();
//         return sock;
//     }
//     if ( vpu == 0 ) {
//         regs = 11;
//         file_size = 896;
//     } else {
//         regs = 12;
//         file_size = 896;
//     }
//     if((fd = fopen(m_vuRegFilename, "wb")) == NULL) {
//         return E_FILE_OPEN;
//     }
//     fclose(fd);
//     sprintf(cmd, arg, m_vuRegFilename, file_size);
//     system(cmd);
//     dumpWaitForFile(m_vuRegFilename, file_size);
//     return E_OK;
// }

// int32
// dumpExecRegisters(char *arg, char *m_vuRegFilename) {
//     FILE *fd;
//     int regs = 10;
//     int file_size = 508;
//     if ( m_vuRegFilename == "" ) {
//         return E_FILE_OPEN;
//     }
//     if ( (fd = fopen(m_vuRegFilename, "wb")) == NULL ) {
//         return E_FILE_OPEN;
//     }
//     fclose(fd);
//     sprintf(cmd, arg, m_vuRegFilename, file_size, regs);
//     system(cmd);
//     dumpWaitForFile(m_vuRegFilename, file_size);
//     return E_OK;
// }

// int32
// dumpExecDisplayList(char *arg, char *file, uint32 *data, uint32 size) {
//     FILE *fd;
//     if ( file == "" ) {
//         return E_FILE_OPEN;
//     }
//     if ( (fd = fopen(file, "wb")) == NULL ) {
//         return E_FILE_OPEN;
//     }
//     fwrite(data, size, 1, fd);
//     fclose(fd);
//     sprintf(cmd, arg, file, size);
//     if ( system(cmd) != 0 ) {
//         return E_SYSTEM_CMD;
//     }
//     return E_OK;
// }

const int32
Remote::WaitForFile(const char* file, uint32 size) {
    struct stat st;
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
        int count = 0;
        count++;
        if ( count == 50 ) {
            return E_TIMEOUT;
        }
    }
    return E_OK;
}
