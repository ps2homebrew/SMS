// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#ifndef __MEMORYPANEL__
#define __MEMORYPANEL__

#include <wx/wx.h>
#include <wx/grid.h>

#include "datatypes.h"
#include "NumberFormat.h"
#include "Log.h"

class MemoryPanel : public wxPanel {
public:
    MemoryPanel(
        wxWindow*       parent,
        wxWindowID      id,
        const wxPoint&  pos = wxDefaultPosition,
        const wxSize&   size = wxDefaultSize,
        long            style = wxTAB_TRAVERSAL,
        const uint32    maxRows = 0
        );
    void Write(
        const uint32    row,
        const wxString& x, 
        const wxString& y,
        const wxString& z,
        const wxString& w
        );
    void WriteX(
        const uint32    row,
        const int32     x
        );
    void WriteY(
        const uint32    row,
        const int32     y
        );
    void WriteZ(
        const uint32    row,
        const int32     z
        );
    void WriteW(
        const uint32    row,
        const int32     w
        );
    void Clear(void);
private:
    DECLARE_EVENT_TABLE()

    void        Redraw(void);
    void        BuildMemoryPanel(uint32 maxRows);
    void        OnFormatChange(wxCommandEvent& event);
    void        OnMemoryEdit(wxGridEvent& event);

    uint32      m_maxRows;
    uint32      m_charWidth;
    wxGrid*     m_pMemoryGrid;
    wxRadioBox* m_pRegRadioBox;
    quadvector* m_pVuMemArray;
    Log*        m_pLog;
};

enum {
    ID_GRIDMEMORY,
    ID_MEMORYRADIO
};

#endif
