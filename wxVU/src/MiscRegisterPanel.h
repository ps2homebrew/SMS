// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#ifndef __MISCREGISTERPANEL__
#define __MISCREGISTERPANEL__

#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/treectrl.h>

#include "datatypes.h"

class MiscRegisterPanel : public wxPanel {
public:
    MiscRegisterPanel();
    MiscRegisterPanel(
        wxWindow*           parent,
        wxWindowID          id,
        const wxPoint&      pos = wxDefaultPosition,
        const wxSize&       size = wxDefaultSize,
        long                style = wxTAB_TRAVERSAL
        );

private:
    DECLARE_EVENT_TABLE()

    //!Function to build the register table
    void            BuildMiscRegistersTable(void);
    //!Function to update the info box for selected register
    //@param 
    //@param
    void            UpdateInfoBox(int a, int b);
    //!Signal handler for the register tree
    //@param event
    void            OnMiscRegSelect(wxTreeEvent &event);

    wxTreeCtrl*     m_pRegisterTree;
    wxBoxSizer*     m_pRegisterSizer;
    wxGrid*         m_pRegisterInfo;
};

enum {
    ID_REGINFOBOX,
    ID_REGTREE
};

#endif
