// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include "GifOutputPanel.h"
#include "vu.h"

BEGIN_EVENT_TABLE(GifOutputPanel, wxPanel)
END_EVENT_TABLE()

/////////////////////////////// PUBLIC ///////////////////////////////////////

GifOutputPanel::GifOutputPanel(
    wxWindow*           parent,
    wxWindowID          id,
    const wxPoint&      pos,
    const wxSize&       size,
    long                style
    ) : wxPanel(parent, id, pos, size, style)
{
    m_counter = 0;
#ifdef WIN32
#undef GetCharWidth
#endif
    m_charWidth = wxWindow::GetCharWidth();
    SetParent(parent);
    BuildGifOutputPanel();
}

void
GifOutputPanel::Write(const wxString& col1, const wxString& col2) {
    m_pGifGrid->SetCellValue(m_counter, 0, col1);
    m_pGifGrid->SetCellValue(m_counter, 1, col2);
    m_counter++;
    return;
}

void
GifOutputPanel::Clear(void) {
    m_counter = 0;
    return;
}

bool
GifOutputPanel::SetBackgroundColour(const wxColour& colour) {
    m_pGifGrid->SetCellBackgroundColour(m_counter, 0, colour);
    m_pGifGrid->SetCellBackgroundColour(m_counter, 1, colour);
    return true;
}

/////////////////////////////// PRIVATE    ///////////////////////////////////
void GifOutputPanel::BuildGifOutputPanel() {
    uint32 i;
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    m_pGifGrid = new wxGrid(this, ID_GRIDGIF, wxDefaultPosition);
    m_pGifGrid->CreateGrid(MAX_VUDATA_SIZE+16, 2);
    m_pGifGrid->SetColMinimalWidth(0, m_charWidth*8);
    m_pGifGrid->SetColMinimalWidth(1, m_charWidth*40);
    m_pGifGrid->SetColSize(0, m_charWidth*8);
    m_pGifGrid->SetColSize(1, m_charWidth*40);
    m_pGifGrid->EnableEditing(FALSE);
    m_pGifGrid->SetColLabelValue(0, "");
    m_pGifGrid->SetColLabelValue(1, "");
    m_pGifGrid->SetColLabelSize(int(GetCharHeight()*1.5));
    m_pGifGrid->SetRowLabelSize(m_charWidth*5);
    m_pGifGrid->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    for (i = 0; i < MAX_VUDATA_SIZE; i++) {
        m_pGifGrid->SetRowLabelValue(i, wxString::Format("%04d", i));
        m_pGifGrid->SetRowSize(i, int(GetCharHeight()*1.5));
        m_pGifGrid->SetCellAlignment(i, 0, wxALIGN_LEFT, wxALIGN_CENTRE);
        m_pGifGrid->SetCellAlignment(i, 1, wxALIGN_LEFT, wxALIGN_CENTRE);
    }
    m_pGifGrid->DisableDragGridSize();
    m_pGifGrid->SetGridLineColour(*wxBLACK);
    sizer->Add(m_pGifGrid, 1, wxEXPAND);
    this->SetSizer(sizer);
}
