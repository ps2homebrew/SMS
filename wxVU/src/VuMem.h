// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#ifndef __VUMEM__
#define __VUMEM__

#include "datatypes.h"
#include "MemoryPanel.h"
#include "Log.h"

class VuMem {
public:
    VuMem(uint32 maxRows, MemoryPanel* memPanel);
    ~VuMem();
    void    WriteVector(uint32 row, float x, float y, float z, float w);
    void    WriteVector(uint32 row, int32 x, int32 y, int32 z, int32 w);
    void    WriteVector(uint32 row, int16 x, int16 y, int16 z, int16 w);
    void    WriteX(uint32 row, float x);
    void    WriteX(uint32 row, uint32 x);
    void    WriteX(uint32 row, int32 x);
    void    WriteX(uint32 row, int16 x);
    void    WriteY(uint32 row, float x);
    void    WriteY(uint32 row, uint32 x);
    void    WriteY(uint32 row, int32 x);
    void    WriteY(uint32 row, int16 x);
    void    WriteZ(uint32 row, float x);
    void    WriteZ(uint32 row, uint32 x);
    void    WriteZ(uint32 row, int32 x);
    void    WriteZ(uint32 row, int16 x);
    void    WriteW(uint32 row, float x);
    void    WriteW(uint32 row, uint32 x);
    void    WriteW(uint32 row, int32 x);
    void    WriteW(uint32 row, int16 x);

    int32   ReadVector(uint32 row);
    int32   ReadMem(uint32* data, uint32 offset, uint32 size);
    int32   ReadX(uint32 row);
    int32   ReadY(uint32 row);
    int32   ReadZ(uint32 row);
    int32   ReadW(uint32 row);
    float   ReadFloatX(uint32 row);
    float   ReadFloatY(uint32 row);
    float   ReadFloatZ(uint32 row);
    float   ReadFloatW(uint32 row);
private:
    Log*            m_pLog;
    quadvector*     m_pVuMemArray;
    MemoryPanel*    m_pMemoryPanel;
    uint32          m_maxRows;
};
#endif
