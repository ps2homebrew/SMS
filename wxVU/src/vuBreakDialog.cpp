// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include <wx/wx.h>
#include "vuBreakDialog.h"
#include "breakpoint.h"
#include "vu.h"

enum {
    brkID_TYPE,
    brkID_INDEX
};

BEGIN_EVENT_TABLE(vuBreakDialog, wxDialog)
    EVT_BUTTON(wxID_CANCEL, vuBreakDialog::OnCancel)
    EVT_BUTTON(wxID_OK, vuBreakDialog::OnOkay)
    EVT_CHOICE(brkID_TYPE, vuBreakDialog::OnTypeSelect)
    EVT_TEXT_ENTER(brkID_INDEX, vuBreakDialog::OnIndexEnter)
END_EVENT_TABLE()

/////////////////////////////// PUBLIC ///////////////////////////////////////
vuBreakDialog::vuBreakDialog(wxWindow* parent, uint32 row)
    : wxDialog(parent, -1, _("Breakpoint settings"),
        wxDefaultPosition, wxDefaultSize) {

    wxString brkChoices[4] = {"None", "Integer Register", "Float Register",
        "Memory row"};

    // Display row number
    wxBoxSizer *disppane = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *text = new wxStaticText(this, -1, "Row number: ",
        wxDefaultPosition, wxDefaultSize);
    m_row = new wxStaticText(this, -1, wxString::Format("%d",
            row), wxDefaultPosition, wxDefaultSize);
    disppane->Add(text, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    disppane->Add(m_row, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);

    // Type
    wxBoxSizer *typepane = new wxBoxSizer(wxHORIZONTAL);
    text = new wxStaticText(this, -1, "Type: ", wxDefaultPosition, wxDefaultSize);
    m_type = new wxChoice(this, brkID_TYPE, wxDefaultPosition, wxDefaultSize,
        4, brkChoices);
    typepane->Add(text, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    typepane->Add(m_type, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);

    // Index
    wxBoxSizer *indexpane = new wxBoxSizer(wxHORIZONTAL);
    text = new wxStaticText(this, -1, "Index: ", wxDefaultPosition, wxDefaultSize);
    m_index = new wxTextCtrl(this, brkID_INDEX, wxEmptyString,
        wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB|wxTE_RIGHT);
    m_index->SetEditable(false);
    indexpane->Add(text, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
    indexpane->Add(m_index, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 5);

    // Value
    wxBoxSizer *valuepane = new wxBoxSizer(wxHORIZONTAL);
    text = new wxStaticText(this, -1, "Value: ", wxDefaultPosition, wxDefaultSize);
    m_value_x = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
        wxDefaultSize);
    m_value_y = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
        wxDefaultSize);
    m_value_z = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
        wxDefaultSize);
    m_value_w = new wxTextCtrl(this, -1, wxEmptyString, wxDefaultPosition,
        wxDefaultSize);
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
    sizer->Add(disppane, 0, wxALIGN_LEFT|wxALL, 10);
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

/////////////////////////////// PRIVATE ///////////////////////////////////
void
vuBreakDialog::OnTypeSelect(wxCommandEvent &WXUNUSED(event)) {
    type = m_type->GetSelection();
    switch (type) {
        case BRK_NONE:
            m_index->SetEditable(false);
            m_value_x->SetEditable(false);
            m_value_y->SetEditable(false);
            m_value_z->SetEditable(false);
            m_value_w->SetEditable(false);
            break;
        case BRK_INTREG:
            m_index->SetEditable(true);
            m_value_x->SetEditable(true);
            m_value_y->SetEditable(false);
            m_value_z->SetEditable(false);
            m_value_w->SetEditable(false);
            break;
        case BRK_FLOATREG:
        case BRK_MEMORY:
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
vuBreakDialog::OnCancel(wxCommandEvent &WXUNUSED(event)) {
    EndModal(wxID_CANCEL);
}

void
vuBreakDialog::OnOkay(wxCommandEvent &WXUNUSED(event)) {
    Breakpoint *bp = Breakpoint::Instance();
    uint32 row, index;
    int32 x, y, z, w;
    float fx, fy, fz, fw;
    char *end_ptr;
    row = (int)strtol(m_row->GetLabel().c_str(), (char **)NULL, 0);
    if ( m_type->GetSelection() > BRK_NONE ) {
        index = (int)strtol(m_index->GetValue().c_str(), (char **)NULL, 0);
    } else {
        index = 0;
    }
    x = (int32)strtol(m_value_x->GetValue().c_str(), &end_ptr, 0);
    if ( !((m_value_x->GetValue().c_str() != '\0') && (*end_ptr == '\0'))) {
        fx = (float)strtod(m_value_x->GetValue().c_str(), &end_ptr);
        x = *((int32 *)&fx);
    }
    y = (int32)strtol(m_value_y->GetValue().c_str(), &end_ptr, 0);
    if ( !((m_value_y->GetValue().c_str() != '\0') && (*end_ptr == '\0'))) {
        fy = (float)strtod(m_value_y->GetValue().c_str(), &end_ptr);
        y = *((int32 *)&fy);
    }
    z = (int32)strtol(m_value_z->GetValue().c_str(), &end_ptr, 0);
    if ( !((m_value_z->GetValue().c_str() != '\0') && (*end_ptr == '\0'))) {
        fz = (float)strtod(m_value_z->GetValue().c_str(), &end_ptr);
        z = *((int32 *)&fz);
    }
    w = (int32)strtol(m_value_w->GetValue().c_str(), &end_ptr, 0);
    if ( !((m_value_w->GetValue().c_str() != '\0') && (*end_ptr == '\0'))) {
        fw = (float)strtod(m_value_w->GetValue().c_str(), &end_ptr);
        w = *((int32 *)&fw);
    }

    bp->Add(row, m_type->GetSelection(), index, x, y, z, w);
    bp->List();
    EndModal(wxID_OK);
}

void
vuBreakDialog::OnIndexEnter(wxCommandEvent &WXUNUSED(event)) {
    int index;
    index = (int)strtol(m_index->GetValue().c_str(), (char **)NULL, 0);
    if ( index < 0 ) {
        m_index->SetValue("0");
    }
    if (m_type->GetSelection() == BRK_INTREG) {
        if ( index > 15 ) {
            m_index->SetValue("15");
        }
    } else if ( m_type->GetSelection() == BRK_FLOATREG) {
        if ( index > 31 ) {
            m_index->SetValue("31");
        }
    } else if ( m_type->GetSelection() == BRK_MEMORY) {
        if ( (uint32)index > MAX_VUDATA_SIZE ) {
            m_index->SetValue(wxString::Format("%d", MAX_VUDATA_SIZE));
        }
    }
}
