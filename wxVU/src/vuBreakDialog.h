// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
//
// A simple dialog for adding breakpoints
//

#include "datatypes.h"

class vuBreakDialog: public wxDialog {
public:
    vuBreakDialog(wxWindow *parent, uint32 row);
    ~vuBreakDialog();

    // event handlers
private:
    void OnCancel(wxCommandEvent &event);
    void OnTypeSelect(wxCommandEvent &event);
    void OnIndexEnter(wxCommandEvent &event);
    void OnOkay(wxCommandEvent &event);
    wxChoice    *m_type;
    wxTextCtrl  *m_index;
    wxTextCtrl  *m_value_x;
    wxTextCtrl  *m_value_y;
    wxTextCtrl  *m_value_z;
    wxTextCtrl  *m_value_w;
    wxStaticText *m_row;
    wxBoxSizer  *sizer;

    uint32      type;
    uint32      index;
    uint32      value;
    DECLARE_EVENT_TABLE()
};
