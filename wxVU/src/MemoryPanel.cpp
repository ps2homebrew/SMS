// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
#include <iostream>
#include "MemoryPanel.h"
#include "debug.h"

BEGIN_EVENT_TABLE(MemoryPanel, wxPanel)
    EVT_RADIOBOX(ID_MEMORYRADIO, MemoryPanel::OnFormatChange)
    EVT_GRID_CELL_CHANGE(MemoryPanel::OnMemoryEdit)
END_EVENT_TABLE()

/////////////////////////////// PUBLIC ///////////////////////////////////////
MemoryPanel::MemoryPanel(
    wxWindow*       parent,
    wxWindowID      id,
    const wxPoint&  pos,
    const wxSize&   size,
    long            style,
    const uint32    maxRows
    ) : wxPanel(parent, id, pos, size, style)
{
#ifdef WIN32
#undef GetCharWidth
#endif
    m_charWidth = wxWindow::GetCharWidth();
    SetParent(parent);
    BuildMemoryPanel(maxRows);
    m_pVuMemArray = new quadvector[maxRows];
    m_maxRows = maxRows-1;
    m_pLog = Log::Instance();
}

void
MemoryPanel::Write(const uint32 row, const wxString& x, const wxString& y,
    const wxString& z, const wxString& w)
{
    if ( row > m_maxRows  ) {
        m_pLog->Warning("Writing outside memory grid range\n");
        return;
    }
    m_pMemoryGrid->SetCellValue(row, 0, x);
    m_pMemoryGrid->SetCellValue(row, 1, y);
    m_pMemoryGrid->SetCellValue(row, 2, z);
    m_pMemoryGrid->SetCellValue(row, 3, w);
    m_pVuMemArray[row].x = x;
    m_pVuMemArray[row].y = y;
    m_pVuMemArray[row].z = z;
    m_pVuMemArray[row].w = w;
    return;
}

void
MemoryPanel::WriteX(const uint32 row, const int32 x) {
    if ( row > m_maxRows  ) {
        m_pLog->Warning("Writing outside memory grid range\n");
        return;
    }
    m_pVuMemArray[row].x = x;
    m_pMemoryGrid->SetCellValue(row, 0, NumberFormat::ToString(x,
            m_pRegRadioBox->GetSelection()));
    return;
}
void
MemoryPanel::WriteY(const uint32 row, const int32 y) {
    if ( row > m_maxRows  ) {
        m_pLog->Warning("Writing outside memory grid range\n");
        return;
    }
    m_pVuMemArray[row].y = y;
    m_pMemoryGrid->SetCellValue(row, 1, NumberFormat::ToString(y,
            m_pRegRadioBox->GetSelection()));
    return;
}
void
MemoryPanel::WriteZ(const uint32 row, const int32 z) {
    if ( row > m_maxRows  ) {
        m_pLog->Warning("Writing outside memory grid range\n");
        return;
    }
    m_pVuMemArray[row].z = z;
    m_pMemoryGrid->SetCellValue(row, 2, NumberFormat::ToString(z,
            m_pRegRadioBox->GetSelection()));
    return;
}
void
MemoryPanel::WriteW(const uint32 row, const int32 w) {
    if ( row > m_maxRows  ) {
        m_pLog->Warning("Writing outside memory grid range\n");
        return;
    }
    m_pVuMemArray[row].w = w;
    m_pMemoryGrid->SetCellValue(row, 3, NumberFormat::ToString(w,
            m_pRegRadioBox->GetSelection()));
    return;
}

void
MemoryPanel::Clear(void) {
    uint32 i;
    wxString zero = NumberFormat::ToString(0, m_pRegRadioBox->GetSelection());
    for(i = 0; i < m_maxRows; i++) {
        m_pMemoryGrid->SetCellValue(i, 0, zero);
        m_pMemoryGrid->SetCellValue(i, 1, zero);
        m_pMemoryGrid->SetCellValue(i, 2, zero);
        m_pMemoryGrid->SetCellValue(i, 3, zero);
    }
    return;
}

/////////////////////////////// PRIVATE ///////////////////////////////////
void
MemoryPanel::BuildMemoryPanel(uint32 maxRows) {
    uint32 i;
    wxBoxSizer *m_sizer = new wxBoxSizer(wxVERTICAL);

    wxString choices[6] = {
        "Fixed 0", "Fixed 4", "Fixed 12",
        "Fixed 15", "Float", "Hex"
    };
    m_pRegRadioBox = new wxRadioBox(
        this, ID_MEMORYRADIO, "Number representation",
        wxDefaultPosition, wxDefaultSize, 6, choices, 6
        );
    m_pRegRadioBox->SetSelection(4);
    m_pMemoryGrid = new wxGrid(
        this, ID_GRIDMEMORY, wxDefaultPosition, wxDefaultSize
        );
    m_pMemoryGrid->SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    m_pMemoryGrid->CreateGrid(maxRows, 4);
    m_pMemoryGrid->EnableEditing(TRUE);
    m_pMemoryGrid->SetColLabelValue(0, "X");
    m_pMemoryGrid->SetColLabelValue(1, "Y");
    m_pMemoryGrid->SetColLabelValue(2, "Z");
    m_pMemoryGrid->SetColLabelValue(3, "W");
    m_pMemoryGrid->SetColLabelSize(int(GetCharHeight()*1.5));
    m_pMemoryGrid->SetRowLabelSize(m_charWidth*5);
    m_pMemoryGrid->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    for (i = 0; i < maxRows; i++) {
        m_pMemoryGrid->SetRowLabelValue(i, wxString::Format("%04d", i));
        m_pMemoryGrid->SetRowSize(i, int(GetCharHeight()*1.5));
    }
    m_pMemoryGrid->DisableDragGridSize();
    m_pMemoryGrid->SetGridLineColour(*wxBLACK);
    m_sizer->Add(m_pRegRadioBox);
    m_sizer->Add(m_pMemoryGrid, 1, wxEXPAND);
    this->SetSizer(m_sizer);
    return;
}

void
MemoryPanel::OnFormatChange(wxCommandEvent &event) {
    Redraw();
}

void
MemoryPanel::OnMemoryEdit(wxGridEvent &event) {
    char *end_ptr;
    int num;
    float fnum;
    wxString cell = m_pMemoryGrid->GetCellValue(event.GetRow(),
        event.GetCol());
    num = (int)strtol(cell.c_str(), &end_ptr, 0);
    if ( !((cell.c_str() != '\0') && (*end_ptr == '\0'))) {
        fnum = (float)strtod(cell.c_str(), &end_ptr);
        num = *((int *)&fnum);
    }

    // TODO
    // write back to VuMem
    switch(event.GetCol()) { 
        case 0:
            m_pVuMemArray[event.GetRow()].x = num;
            break;
        case 1:
            m_pVuMemArray[event.GetRow()].y = num;
            break;
        case 2:
            m_pVuMemArray[event.GetRow()].z = num;
            break;
        case 3:
            m_pVuMemArray[event.GetRow()].w = num;
            break;
    }
    m_pMemoryGrid->SetCellValue(event.GetRow(), event.GetCol(),
        NumberFormat::ToString(num, m_pRegRadioBox->GetSelection())
        );
    return;
}

void
MemoryPanel::Redraw() {
    uint32 i;
    for(i = 0; i < m_maxRows; i++) {
        m_pMemoryGrid->SetCellValue(i, 0,
            NumberFormat::ToString(m_pVuMemArray[i].x,
                m_pRegRadioBox->GetSelection()));
        m_pMemoryGrid->SetCellValue(i, 1,
            NumberFormat::ToString(m_pVuMemArray[i].y,
                m_pRegRadioBox->GetSelection()));
        m_pMemoryGrid->SetCellValue(i, 2,
            NumberFormat::ToString(m_pVuMemArray[i].z,
                m_pRegRadioBox->GetSelection()));
        m_pMemoryGrid->SetCellValue(i, 3,
            NumberFormat::ToString(m_pVuMemArray[i].w,
                m_pRegRadioBox->GetSelection()));
    }
    return;
}
