// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include "VuMem.h"

/////////////////////////////// PUBLIC ///////////////////////////////////////
VuMem::VuMem(uint32 maxRows, MemoryPanel* memPanel) {
    m_pVuMemArray = new quadvector[maxRows];
    m_maxRows = maxRows-1;
    m_pMemoryPanel = memPanel;
    m_pLog = Log::Instance();
}

VuMem::~VuMem() {
    delete [] m_pVuMemArray;
}

void
VuMem::WriteVector(uint32 row, float x, float y, float z, float w) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }

    m_pVuMemArray[row].x = *((int32 *)&x);
    m_pVuMemArray[row].y = *((int32 *)&y);
    m_pVuMemArray[row].z = *((int32 *)&z);
    m_pVuMemArray[row].w = *((int32 *)&w);
    return;
}

void
VuMem::WriteVector(uint32 row, int32 x, int32 y, int32 z, int32 w) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }

    m_pVuMemArray[row].x = x;
    m_pVuMemArray[row].y = y;
    m_pVuMemArray[row].z = z;
    m_pVuMemArray[row].w = w;
    return;
}

void
VuMem::WriteX(uint32 row, float x) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }
    m_pVuMemArray[row].x = *((int32 *)&x);
    return;
}

void
VuMem::WriteX(uint32 row, int32 x) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }
    m_pVuMemArray[row].x = x;
    m_pMemoryPanel->WriteX(row, x);
    return;
}

void
VuMem::WriteX(uint32 row, uint32 x) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }
    m_pVuMemArray[row].x = x;
    m_pMemoryPanel->WriteX(row, x);
    return;
}

void
VuMem::WriteY(uint32 row, float y) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }

    m_pVuMemArray[row].y = *((int32 *)&y);
    return;
}

void
VuMem::WriteY(uint32 row, int32 y) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }
    m_pVuMemArray[row].y = y;
    m_pMemoryPanel->WriteY(row, y);
    return;
}

void
VuMem::WriteY(uint32 row, uint32 y) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }
    m_pVuMemArray[row].y = y;
    m_pMemoryPanel->WriteY(row, y);
    return;
}

void
VuMem::WriteZ(uint32 row, float z) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }
    m_pVuMemArray[row].z = *((int32 *)&z);
    return;
}

void
VuMem::WriteZ(uint32 row, int32 z) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }
    m_pVuMemArray[row].z = z;
    m_pMemoryPanel->WriteZ(row, z);
    return;
}

void
VuMem::WriteZ(uint32 row, uint32 z) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }

    m_pVuMemArray[row].z = z;
    m_pMemoryPanel->WriteZ(row, z);
    return;
}

void
VuMem::WriteW(uint32 row, float w) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }

    m_pVuMemArray[row].w = *((int32 *)&w);
    return;
}

void
VuMem::WriteW(uint32 row, int32 w) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }
    m_pVuMemArray[row].w = w;
    m_pMemoryPanel->WriteW(row, w);
    return;
}

void
VuMem::WriteW(uint32 row, uint32 w) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Writing outside VuDataMem range\n");
        return;
    }
    m_pVuMemArray[row].w = w;
    m_pMemoryPanel->WriteW(row, w);
    return;
}

int32
VuMem::ReadX(uint32 row) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Reading outside VuDataMem range\n");
        return 0;
    }
    return m_pVuMemArray[row].x;
}

int32
VuMem::ReadY(uint32 row) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Reading outside VuDataMem range\n");
        return 0;
    }
    return m_pVuMemArray[row].y;
}

int32
VuMem::ReadZ(uint32 row) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Reading outside VuDataMem range\n");
        return 0;
    }
    return m_pVuMemArray[row].z;
}

int32
VuMem::ReadW(uint32 row) {
    if ( row > m_maxRows ) {
        m_pLog->Warning("Reading outside VuDataMem range\n");
        return 0;
    }
    return m_pVuMemArray[row].w;
}

// void
// VuMem::WriteMemFromFile(char *filename) {
//     FILE *fd;
//     struct stat sb;
//     uint32 i;

//     if ((stat(file.GetFullPath().c_str(), &sb)) != 0 ) {
//         return E_FILE_OPEN;
//     }
//     // KLUDGE
//     if ((fd = fopen(file.GetFullPath().c_str(), "rb")) != NULL) {
//         for(i = 0; i < MAX_VUDATA_SIZE && i < sb.st_size/16; i++) {
//             uint32 x, y, z, w;
//             fread(&x, sizeof(int32), 1, fd);
//             fread(&y, sizeof(int32), 1, fd);
//             fread(&z, sizeof(int32), 1, fd);
//             fread(&w, sizeof(int32), 1, fd);
//             m_pVu1->WriteMemX(i, x);
//             m_pVu1->WriteMemY(i, y);
//             m_pVu1->WriteMemZ(i, z);
//             m_pVu1->WriteMemW(i, w);
//             if ( tagShow == 0 ) {
//                 if (
//                     (validateRegsField(((uint64)z<<32)+w) == getNregs(((uint64)x<<32)+y))
//                     && ((getFLGField(((uint64)x<<32)+y) == 0))
//                    ) {
//                     // m_pVu1->dataMem[i].gif = true;
//                 } else {
//                     // m_pVu1->dataMem[i].gif = false;
//                 }
//             } else {
//                 // m_pVu1->dataMem[i].gif = false;
//             }
//         }
//         fclose(fd);
//         DrawMemory();
//         m_pTextDebug->AppendText("VU Data loaded from file: " +
//             file.GetFullPath() + "\n");
//     } else {
//         TextDebugFailed(wxString("Failed to load VU Data from file: " +
//                 m_dataFile.GetFullPath() + "\n"));
//         return E_FILE_OPEN;
//     }
//     return 0;
// }

/////////////////////////////// PRIVATE    ///////////////////////////////////
