// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
// 
// Singleton
//
#ifndef __LOG__
#define __LOG__

#include "datatypes.h"

class wxFrame;
class wxString;
class wxTextCtrl;

const int32     E_OK            =   0;
const int32     E_TIMEOUT       =   -10;
// VU Errors

// PS2Link protocol errorrs 
const int32     E_SOCKET        =   -20;
const int32     E_NO_LINK       =   -21;
const int32     E_LINK_TIMEOUT  =   -22;
const int32     E_SOCK_CLOSE    =   -23;

// File operations errors
const int32     E_FILE_OPEN     =   -30;
const int32     E_FILE_READ     =   -31;
const int32     E_FILE_WRITE    =   -32;
const int32     E_FILE_EOF      =   -33;

//
const int32     E_SYSTEM_CMD    =   -40;

// VIF Errors
const int32     E_VIF_DECODE    =   -50;

class Log {
public:
    static Log*     Instance();
    void			SetFrame(wxFrame* frame);
    void			SetTextCtrl(wxTextCtrl* out);
    void            Error(const wxString& message);
    void            Error(const int32 errno);
    void            Error(const int32 errno, const wxString& message);
    void            Warning(const wxString& message);
    void            Warning(const int32 errno);
    void            Trace(const wxString& message);
    void            Trace(int32 level, const wxString& message);
protected:
    Log();
    ~Log();
private:
    static Log*     _instance;
    wxString*       m_pLastMessage;
    wxTextCtrl*     m_pOut;
    wxFrame*        m_pFrame;
    uint32          m_numLastMessage;
};

#endif
