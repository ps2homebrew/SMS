// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include <wx/wx.h>
#include "vuBreakDialog.h"

enum {
    brkID_TYPE
};

BEGIN_EVENT_TABLE(vuBreakDialog, wxDialog)
    EVT_BUTTON(wxID_CANCEL, vuBreakDialog::OnCancel)
    EVT_BUTTON(wxID_OK, vuBreakDialog::OnOkay)
    EVT_CHOICE(brkID_TYPE, vuBreakDialog::OnTypeSelect)
END_EVENT_TABLE()

vuBreakDialog::vuBreakDialog(wxWindow *parent)
    : wxDialog(parent, -1, _("Breakpoint settings"),
        wxDefaultPosition, wxDefaultSize) {

    wxString brkChoices[4] = {"None", "Float Register", "Integer Register",
        "Memory row"};
    m_type = new wxChoice(this, brkID_TYPE, wxDefaultPosition, wxDefaultSize,
        4, brkChoices);
    // m_index = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
    //     wxDefaultSize);
    // m_value = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
    //     wxDefaultSize);

    // buttons
    wxBoxSizer *buttonpane = new wxBoxSizer (wxHORIZONTAL);
    wxButton *okButton = new wxButton (this, wxID_OK, _("&Ok"));
    okButton->SetDefault();
    buttonpane->Add (okButton, 0, wxALIGN_CENTER);
    buttonpane->Add (6, 0);
    wxButton *cancelButton = new wxButton (this, wxID_CANCEL, _("Cancel"));
    buttonpane->Add (cancelButton, 0, wxALIGN_CENTER);

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_type, 0, wxALIGN_CENTER|wxALL, 10);
    sizer->Add(buttonpane, 0, wxALIGN_CENTER|wxALL, 10);
    SetSizerAndFit(sizer);
    ShowModal();
}

vuBreakDialog::~vuBreakDialog() {
    delete m_type;
}

void
vuBreakDialog::OnTypeSelect(wxCommandEvent &event) {
    type = m_type->GetSelection();
}

void
vuBreakDialog::OnCancel(wxCommandEvent &event) {
    EndModal(wxID_CANCEL);
}

void
vuBreakDialog::OnOkay(wxCommandEvent &event) {
    EndModal(wxID_OK);
}

// private 
void
vuBreakDialog::DrawFloatCtrl(void) {
}

void
vuBreakDialog::DrawIntCtrl(void) {
}

void
vuBreakDialog::DrawMemCtrl(void) {
}
