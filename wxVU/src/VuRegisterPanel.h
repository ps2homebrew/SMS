// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#ifndef __VUREGISTERPANEL__
#define __VUREGISTERPANEL__
#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/notebook.h>

#include "datatypes.h"
#include "Log.h"
#include "NumberFormat.h"

class VuRegisterPanel : public wxPanel {
public:
    VuRegisterPanel(
        wxWindow*       parent,
        wxWindowID      id,
        const wxPoint&  pos = wxDefaultPosition,
        const wxSize&   size = wxDefaultSize,
        long            style = wxTAB_TRAVERSAL
        );
    void        WriteInt(const uint32 index, const int16 x);
    void        WriteFloat(const uint32 index, const float x, const float y,
        const float z, const float w);
    void        WriteAcc(const float x, const float y, const float z, const float w);
    void        WriteQ(const float x, const float y, const float z, const float w);
    void        WriteP(const float x, const float y, const float z, const float w);
    void        WriteR(const float x, const float y, const float z, const float w);
    void        WriteI(const float x, const float y, const float z, const float w);
private:
    DECLARE_EVENT_TABLE()

    void        BuildVuRegisterPanel(void);
    void        OnIntRegRadio(wxCommandEvent& event);
    void        OnFloatRegRadio(wxCommandEvent& event);
    void        OnSpecRegRadio(wxCommandEvent& event);
    void        OnEdit(wxGridEvent& event);
    wxString    FloatToString(float x, uint32 format);

    wxPanel*    m_pIntRegPanel;
    wxPanel*    m_pFloatRegPanel;
    wxPanel*    m_pSpecRegPanel;
    wxGrid*     m_pFloatRegGrid;
    wxGrid*     m_pIntRegGrid;
    wxGrid*     m_pSpecRegGrid;
    wxRadioBox* m_pIntRegRadio;
    wxRadioBox* m_pFloatRegRadio;
    wxRadioBox* m_pSpecRegRadio;
    wxRadioBox* m_pRegRadioBox;
    wxNotebook* m_pNoteBook;
    uint32      m_charWidth;
    fvec*       m_pFloatRegArray;
    fvec*       m_pSpecRegArray;
    int32*      m_pIntRegArray;
    Log*        m_pLog;
};

enum {
    ID_INTREGRADIO,
    ID_INTGRIDEDIT,
    ID_FLOATREGRADIO,
    ID_FLOATGRIDEDIT,
    ID_SPECREGRADIO,
    ID_SPECGRIDEDIT
};
#endif
