#ifndef __REMOTE__
#define __REMOTE__
#include "datatypes.h"

class Remote {
public:
    Remote() {};
    ~Remote() {};
    static const int32  GetVu(const uint32 vpu);
    static const int32  GetVuRegisters(const uint32 vpu);

    static const int32  GetMiscRegisters(void);

    static const int32  GsExec(const unsigned char* buffer, const uint32 size);
    static const int32  GsInit(void);
    static const int32  GsSetColor(void);

    static void         SetTmpFiles(const char* dfile, const char* cfile,
                                    const char* rfile, const char* mfile,
                                    const char* gfile);
    static void         SetGsInit(const uint32 xOffset, const uint32 yOffset, 
                                    const uint32 xScissor, const uint32 yScissor,
                                    const uint32 rgba
                                    );
private:
    static const int32     Open(void);
    static const int32     Close(void);
    static const int32     WaitForFile(const char* filename, const uint32 size);

    static char     m_vuCodeFilename[256];
    static char     m_vuDataFilename[256];
    static char     m_vuRegFilename[256];
    static char     m_gsFilename[256];
    static char     m_miscFilename[256];
    static uint32   m_tagGsInit[24];
    static uint32   m_tagGsClear[24];
    static const char   cmd[1024];
};
#endif
