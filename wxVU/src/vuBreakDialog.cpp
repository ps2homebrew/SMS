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

    // Type
    wxBoxSizer *typepane = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *text = new wxStaticText(this, -1, "Type", wxDefaultPosition, wxDefaultSize);
    m_type = new wxChoice(this, brkID_TYPE, wxDefaultPosition, wxDefaultSize,
        4, brkChoices);
    typepane->Add(text, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    typepane->Add(m_type, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);

    // Index
    wxBoxSizer *indexpane = new wxBoxSizer(wxHORIZONTAL);
    text = new wxStaticText(this, -1, "Index", wxDefaultPosition, wxDefaultSize);
    m_index = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize);
    m_index->SetEditable(false);
    indexpane->Add(text, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    indexpane->Add(m_index, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);

    // Value
    wxBoxSizer *valuepane = new wxBoxSizer(wxHORIZONTAL);
    text = new wxStaticText(this, -1, "Value", wxDefaultPosition, wxDefaultSize);
    m_value_x = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxTE_RICH);
    m_value_y = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxTE_RICH);
    m_value_z = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxTE_RICH);
    m_value_w = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxTE_RICH);
    m_value_x->SetEditable(false);
    m_value_y->SetEditable(false);
    m_value_z->SetEditable(false);
    m_value_w->SetEditable(false);
    valuepane->Add(text, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    valuepane->Add(m_value_x, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    valuepane->Add(m_value_y, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    valuepane->Add(m_value_z, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    valuepane->Add(m_value_w, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);

    // buttons
    wxBoxSizer *buttonpane = new wxBoxSizer (wxHORIZONTAL);
    wxButton *okButton = new wxButton (this, wxID_OK, _("&Ok"));
    okButton->SetDefault();
    buttonpane->Add (okButton, 0, wxALIGN_CENTER);
    buttonpane->Add (6, 0);
    wxButton *cancelButton = new wxButton (this, wxID_CANCEL, _("Cancel"));
    buttonpane->Add (cancelButton, 0, wxALIGN_CENTER);

    sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(typepane, 0, wxALIGN_LEFT|wxALL, 10);
    sizer->Add(indexpane, 0, wxALIGN_LEFT|wxALL, 10);
    sizer->Add(valuepane, 0, wxALIGN_LEFT|wxALL, 10);
    sizer->Add(buttonpane, 0, wxALIGN_CENTER|wxALL, 10);
    SetSizerAndFit(sizer);
    SetAutoLayout(true);
    ShowModal();
}

vuBreakDialog::~vuBreakDialog() {
    delete m_type;
}

void
vuBreakDialog::OnTypeSelect(wxCommandEvent &event) {
    type = m_type->GetSelection();
    switch (type) {
        case 0:
            m_index->SetEditable(false);
            m_value_x->SetEditable(false);
            m_value_y->SetEditable(false);
            m_value_z->SetEditable(false);
            m_value_w->SetEditable(false);
            break;
        case 1:
            m_index->SetEditable(true);
            m_value_x->SetEditable(true);
            m_value_y->SetEditable(true);
            m_value_z->SetEditable(true);
            m_value_w->SetEditable(true);
            break;
        case 2:
            m_index->SetEditable(true);
            m_value_x->SetEditable(true);
            m_value_y->SetEditable(false);
            m_value_z->SetEditable(false);
            m_value_w->SetEditable(false);
            break;
        case 3:
            m_index->SetEditable(true);
            m_value_x->SetEditable(true);
            m_value_y->SetEditable(true);
            m_value_z->SetEditable(true);
            m_value_w->SetEditable(true);
            break;
        default:
            break;
    }
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
