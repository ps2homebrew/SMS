// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include "VuRegisterPanel.h"
#include "NumberFormat.h"

BEGIN_EVENT_TABLE(VuRegisterPanel, wxPanel)
    EVT_RADIOBOX(ID_INTREGRADIO,            VuRegisterPanel::OnIntRegRadio)
    EVT_RADIOBOX(ID_FLOATREGRADIO,          VuRegisterPanel::OnFloatRegRadio)
    EVT_RADIOBOX(ID_SPECREGRADIO,           VuRegisterPanel::OnSpecRegRadio)
    EVT_GRID_CELL_CHANGE(VuRegisterPanel::OnEdit)
END_EVENT_TABLE()

static const uint32 MAXFLOAT = 32;
static const uint32 MAXSPEC = 5;
static const uint32 MAXINT = 16;

/////////////////////////////// PUBLIC ///////////////////////////////////////
VuRegisterPanel::VuRegisterPanel(
    wxWindow*       parent,
    wxWindowID      id,
    const wxPoint&  pos,
    const wxSize&   size,
    long            style
    ) : wxPanel(parent, id, pos, size, style)
{
#ifdef WIN32
#undef GetCharWidth
#endif
    m_charWidth = wxWindow::GetCharWidth();
    SetParent(parent);
    BuildVuRegisterPanel();
    m_pIntRegArray = new int32[MAXINT];
    m_pFloatRegArray = new fvec[MAXFLOAT];
    m_pSpecRegArray = new fvec[MAXSPEC];
    m_pLog = Log::Instance();
}

void
VuRegisterPanel::WriteInt(const uint32 index, const int16 x) {
    if ( index >= MAXINT ) {
        m_pLog->Error("Integer register index too high\n");
        return;
    }
    switch(m_pIntRegRadio->GetSelection()) {
        case 0:
            m_pIntRegGrid->SetCellValue(0, index, wxString::Format("%d", x));
            break;
        case 1:
            m_pIntRegGrid->SetCellValue(0, index, wxString::Format("%.4f", 
                    (float)x/16));
            break;
        case 2:
            m_pIntRegGrid->SetCellValue(0, index, wxString::Format("%.12f",
                    (float)x/4096));
            break;
        case 3:
            m_pIntRegGrid->SetCellValue(0, index, wxString::Format("%.12f",
                    (float)x/32768));
            break;
        case 4:
            m_pIntRegGrid->SetCellValue(0, index, wxString::Format("%f",
                    *((float *)&x) ));
            break;
        case 5:
            m_pIntRegGrid->SetCellValue(0, index, wxString::Format("0x%x",
                    x));
            break;
    }
    m_pIntRegArray[index] = x;
}

void
VuRegisterPanel::WriteFloat(const uint32 index, const float x, const float y,
    const float z, const float w) {
    if ( index >= MAXFLOAT ) {
        m_pLog->Error("Float register index too high\n");
        return;
    }
    uint32 selection = m_pFloatRegRadio->GetSelection();
    m_pFloatRegGrid->SetCellValue(0, index, NumberFormat::ToString(x, selection));
    m_pFloatRegGrid->SetCellValue(1, index, NumberFormat::ToString(y, selection));
    m_pFloatRegGrid->SetCellValue(2, index, NumberFormat::ToString(z, selection));
    m_pFloatRegGrid->SetCellValue(3, index, NumberFormat::ToString(w, selection));
    m_pFloatRegArray[index].x = x;
    m_pFloatRegArray[index].y = y;
    m_pFloatRegArray[index].z = z;
    m_pFloatRegArray[index].w = w;
    return;
}

void
VuRegisterPanel::WriteAcc(const float x, const float y, const float z, const
    float w) {
    uint32 selection = m_pSpecRegRadio->GetSelection();
    m_pSpecRegGrid->SetCellValue(0, 0, NumberFormat::ToString(x, selection));
    m_pSpecRegGrid->SetCellValue(1, 0, NumberFormat::ToString(y, selection));
    m_pSpecRegGrid->SetCellValue(2, 0, NumberFormat::ToString(z, selection));
    m_pSpecRegGrid->SetCellValue(3, 0, NumberFormat::ToString(w, selection));
    m_pSpecRegArray[0].x = x;
    m_pSpecRegArray[0].y = y;
    m_pSpecRegArray[0].z = z;
    m_pSpecRegArray[0].w = w;
}

void
VuRegisterPanel::WriteQ(const float x, const float y, const float z, const
    float w) {
    uint32 selection = m_pSpecRegRadio->GetSelection();
    m_pSpecRegGrid->SetCellValue(0, 1, NumberFormat::ToString(x, selection));
    m_pSpecRegGrid->SetCellValue(1, 1, NumberFormat::ToString(y, selection));
    m_pSpecRegGrid->SetCellValue(2, 1, NumberFormat::ToString(z, selection));
    m_pSpecRegGrid->SetCellValue(3, 1, NumberFormat::ToString(w, selection));
    m_pSpecRegArray[1].x = x;
    m_pSpecRegArray[1].y = y;
    m_pSpecRegArray[1].z = z;
    m_pSpecRegArray[1].w = w;
}

void
VuRegisterPanel::WriteP(const float x, const float y, const float z, const
    float w) {
    uint32 selection = m_pSpecRegRadio->GetSelection();
    m_pSpecRegGrid->SetCellValue(0, 2, NumberFormat::ToString(x, selection));
    m_pSpecRegGrid->SetCellValue(1, 2, NumberFormat::ToString(y, selection));
    m_pSpecRegGrid->SetCellValue(2, 2, NumberFormat::ToString(z, selection));
    m_pSpecRegGrid->SetCellValue(3, 2, NumberFormat::ToString(w, selection));
    m_pSpecRegArray[2].x = x;
    m_pSpecRegArray[2].y = y;
    m_pSpecRegArray[2].z = z;
    m_pSpecRegArray[2].w = w;
}

void
VuRegisterPanel::WriteR(const float x, const float y, const float z, const float w) {
    uint32 selection = m_pSpecRegRadio->GetSelection();
    m_pSpecRegGrid->SetCellValue(0, 3, NumberFormat::ToString(x, selection));
    m_pSpecRegGrid->SetCellValue(1, 3, NumberFormat::ToString(y, selection));
    m_pSpecRegGrid->SetCellValue(2, 3, NumberFormat::ToString(z, selection));
    m_pSpecRegGrid->SetCellValue(3, 3, NumberFormat::ToString(w, selection));
    m_pSpecRegArray[3].x = x;
    m_pSpecRegArray[3].y = y;
    m_pSpecRegArray[3].z = z;
    m_pSpecRegArray[3].w = w;
}

void
VuRegisterPanel::WriteI(const float x, const float y, const float z, const float w) {
    uint32 selection = m_pSpecRegRadio->GetSelection();
    m_pSpecRegGrid->SetCellValue(0, 4, NumberFormat::ToString(x, selection));
    m_pSpecRegGrid->SetCellValue(1, 4, NumberFormat::ToString(y, selection));
    m_pSpecRegGrid->SetCellValue(2, 4, NumberFormat::ToString(z, selection));
    m_pSpecRegGrid->SetCellValue(3, 4, NumberFormat::ToString(w, selection));
    m_pSpecRegArray[4].x = x;
    m_pSpecRegArray[4].y = y;
    m_pSpecRegArray[4].z = z;
    m_pSpecRegArray[4].w = w;
}

/////////////////////////////// PRIVATE    ///////////////////////////////////
void
VuRegisterPanel::BuildVuRegisterPanel() {
	int i;
    m_pNoteBook = new wxNotebook(
        this, -1, wxDefaultPosition, wxDefaultSize
        );
    m_pIntRegPanel = new wxPanel(m_pNoteBook, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER);
    m_pFloatRegPanel = new wxPanel(m_pNoteBook, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER);
    m_pSpecRegPanel = new wxPanel(m_pNoteBook, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER);

    // Build the Integer register tab
	
    wxString choices[6] = {"Fixed 0", "Fixed 4", "Fixed 12", "Fixed 15",
		"Float", "Hex" };
    m_pIntRegRadio = new wxRadioBox(m_pIntRegPanel, ID_INTREGRADIO,
        "Number representation", wxDefaultPosition,
        wxDefaultSize, 6, choices, 6);
    m_pIntRegRadio->SetSelection(0);
    m_pIntRegGrid = new wxGrid(m_pIntRegPanel, ID_INTGRIDEDIT, wxDefaultPosition);
    m_pIntRegGrid->CreateGrid(1, 16);
    m_pIntRegGrid->SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    m_pIntRegGrid->EnableEditing(TRUE);
    m_pIntRegGrid->SetRowLabelValue(0, "X");
    m_pIntRegGrid->SetColLabelSize(int(wxWindow::GetCharHeight()*1.5));
    m_pIntRegGrid->SetRowLabelSize(m_charWidth*2);
    m_pIntRegGrid->SetRowSize(0, int(GetCharHeight()*1.5));
    m_pIntRegGrid->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    for (i = 0; i < 16; i++) {
        m_pIntRegGrid->SetColLabelValue(i, wxString::Format("vi%02d", i));
    }

    // Build the Float register tab
    m_pFloatRegRadio = new wxRadioBox(m_pFloatRegPanel, ID_FLOATREGRADIO,
        "Number representation", wxDefaultPosition,
        wxDefaultSize, 6, choices, 6);
    m_pFloatRegRadio->SetSelection(4);
    m_pFloatRegGrid = new wxGrid(m_pFloatRegPanel, ID_FLOATGRIDEDIT, wxDefaultPosition);
    m_pFloatRegGrid->CreateGrid(4, 32);
    m_pFloatRegGrid->SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    m_pFloatRegGrid->EnableEditing(TRUE);
    m_pFloatRegGrid->SetRowLabelValue(0, "X");
    m_pFloatRegGrid->SetRowLabelValue(1, "Y");
    m_pFloatRegGrid->SetRowLabelValue(2, "Z");
    m_pFloatRegGrid->SetRowLabelValue(3, "W");
    m_pFloatRegGrid->SetColLabelSize(int(GetCharHeight()*1.5));
    m_pFloatRegGrid->SetRowLabelSize(m_charWidth*3);
    m_pFloatRegGrid->SetRowSize(0, int(GetCharHeight()*1.5));
    m_pFloatRegGrid->SetRowSize(1, int(GetCharHeight()*1.5));
    m_pFloatRegGrid->SetRowSize(2, int(GetCharHeight()*1.5));
    m_pFloatRegGrid->SetRowSize(3, int(GetCharHeight()*1.5));
    m_pFloatRegGrid->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    for (i = 0; i < 32; i++) {
        m_pFloatRegGrid->SetColLabelValue(i, wxString::Format("vf%02d", i));
    }

    // Build the Special register tab
    m_pSpecRegRadio = new wxRadioBox(m_pSpecRegPanel, ID_SPECREGRADIO,
        "Number representation", wxDefaultPosition,
        wxDefaultSize, 6, choices, 6);
    m_pSpecRegRadio->SetSelection(4);
    m_pSpecRegGrid = new wxGrid(m_pSpecRegPanel, ID_SPECGRIDEDIT, wxDefaultPosition);
    m_pSpecRegGrid->CreateGrid(4, 5);
    m_pSpecRegGrid->SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    m_pSpecRegGrid->EnableEditing(TRUE);
    m_pSpecRegGrid->SetRowLabelValue(0, "X");
    m_pSpecRegGrid->SetRowLabelValue(1, "Y");
    m_pSpecRegGrid->SetRowLabelValue(2, "Z");
    m_pSpecRegGrid->SetRowLabelValue(3, "W");
    m_pSpecRegGrid->SetColLabelSize(int(GetCharHeight()*1.5));
    m_pSpecRegGrid->SetRowLabelSize(m_charWidth*3);
    m_pSpecRegGrid->SetRowSize(0, int(GetCharHeight()*1.5));
    m_pSpecRegGrid->SetRowSize(1, int(GetCharHeight()*1.5));
    m_pSpecRegGrid->SetRowSize(2, int(GetCharHeight()*1.5));
    m_pSpecRegGrid->SetRowSize(3, int(GetCharHeight()*1.5));
    m_pSpecRegGrid->SetColLabelValue(0, wxString("ACC"));
    m_pSpecRegGrid->SetColLabelValue(1, wxString("Q"));
    m_pSpecRegGrid->SetColLabelValue(2, wxString("P"));
    m_pSpecRegGrid->SetColLabelValue(3, wxString("R"));
    m_pSpecRegGrid->SetColLabelValue(4, wxString("I"));
    m_pSpecRegGrid->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

    wxBoxSizer *m_pIntRegSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *m_pFloatRegSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *m_pSpecRegSizer = new wxBoxSizer(wxVERTICAL);

	m_pIntRegSizer->Add(m_pIntRegRadio);
	m_pIntRegSizer->Add(m_pIntRegGrid, 1, wxEXPAND);
	m_pIntRegPanel->SetSizer(m_pIntRegSizer);

	m_pFloatRegSizer->Add(m_pFloatRegRadio);
	m_pFloatRegSizer->Add(m_pFloatRegGrid, 1, wxEXPAND);
	m_pFloatRegPanel->SetSizer(m_pFloatRegSizer);

	m_pSpecRegSizer->Add(m_pSpecRegRadio);
	m_pSpecRegSizer->Add(m_pSpecRegGrid, 1, wxEXPAND);
	m_pSpecRegPanel->SetSizer(m_pSpecRegSizer);

    m_pNoteBook->AddPage(m_pIntRegPanel, "Integer registers", TRUE, -1);
    m_pNoteBook->AddPage(m_pFloatRegPanel, "Float registers", FALSE, -1);
    m_pNoteBook->AddPage(m_pSpecRegPanel, "Special registers", FALSE, -1);
    wxBoxSizer *m_sizer = new wxBoxSizer(wxVERTICAL);
    m_sizer->Add(m_pNoteBook, 1, wxEXPAND);
    this->SetSizer(m_sizer);
}

void
VuRegisterPanel::OnIntRegRadio(wxCommandEvent &WXUNUSED(event)) {
    uint32 i;
    for(i = 0; i < MAXINT; i++) {
        m_pIntRegGrid->SetCellValue(0, i,
            NumberFormat::ToString(m_pIntRegArray[i],
                m_pIntRegRadio->GetSelection()));
    }
}
void
VuRegisterPanel::OnFloatRegRadio(wxCommandEvent &WXUNUSED(event)) {
    uint32 i;
    for(i = 0; i < MAXFLOAT; i++) {
        m_pFloatRegGrid->SetCellValue(0, i,
            NumberFormat::ToString(m_pFloatRegArray[i].x,
                m_pFloatRegRadio->GetSelection()));
        m_pFloatRegGrid->SetCellValue(1, i, 
            NumberFormat::ToString(m_pFloatRegArray[i].y,
                m_pFloatRegRadio->GetSelection()));
        m_pFloatRegGrid->SetCellValue(2, i,
            NumberFormat::ToString(m_pFloatRegArray[i].z,
                m_pFloatRegRadio->GetSelection()));
        m_pFloatRegGrid->SetCellValue(3, i,
            NumberFormat::ToString(m_pFloatRegArray[i].w,
                m_pFloatRegRadio->GetSelection()));
    }
}
void
VuRegisterPanel::OnSpecRegRadio(wxCommandEvent &WXUNUSED(event)) {
    uint32 i;
    for(i = 0; i < MAXSPEC; i++) {
        m_pSpecRegGrid->SetCellValue(0, i,
            NumberFormat::ToString(m_pSpecRegArray[i].x,
                m_pSpecRegRadio->GetSelection()));
        m_pSpecRegGrid->SetCellValue(1, i,
            NumberFormat::ToString(m_pSpecRegArray[i].y,
                m_pSpecRegRadio->GetSelection()));
        m_pSpecRegGrid->SetCellValue(2, i,
            NumberFormat::ToString(m_pSpecRegArray[i].z,
                m_pSpecRegRadio->GetSelection()));
        m_pSpecRegGrid->SetCellValue(3, i,
            NumberFormat::ToString(m_pSpecRegArray[i].w,
                m_pSpecRegRadio->GetSelection()));
    }
}

void
VuRegisterPanel::OnEdit(wxGridEvent& event) {
    char *end_ptr;
    int num;
    float fnum;
    wxString cell;
    if ( event.GetId() == ID_INTGRIDEDIT ) {
        cell = m_pIntRegGrid->GetCellValue(event.GetRow(), event.GetCol());
    } else if ( event.GetId() == ID_FLOATGRIDEDIT ) {
        cell = m_pFloatRegGrid->GetCellValue(event.GetRow(), event.GetCol());
    } else if ( event.GetId() == ID_SPECGRIDEDIT ) {
        cell = m_pSpecRegGrid->GetCellValue(event.GetRow(), event.GetCol());
    }
    num = (int)strtol(cell.c_str(), &end_ptr, 0);
    if ( !((cell.c_str() != '\0') && (*end_ptr == '\0'))) {
        fnum = (float)strtod(cell.c_str(), &end_ptr);
        num = *((int *)&fnum);
    }
    if ( event.GetId() == ID_INTGRIDEDIT ) {
        // cdbg << "int num = " << num << endl;
    } else if ( event.GetId() == ID_FLOATGRIDEDIT ) {
        // cdbg << "float num = " << num << endl;
    } else if ( event.GetId() == ID_SPECGRIDEDIT ) {
        // cdbg << "spec num = " << num << endl;
    }
}
