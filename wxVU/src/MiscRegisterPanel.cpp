// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include "MiscRegisterPanel.h"

#include "dma.h"
#include "intc.h"
#include "timer.h"
#include "gs.h"
#include "sif.h"
#include "fifo.h"
#include "gif.h"
#include "vif0.h"
#include "vif1.h"

BEGIN_EVENT_TABLE(MiscRegisterPanel, wxPanel)
    EVT_TREE_ITEM_ACTIVATED(ID_REGTREE, MiscRegisterPanel::OnMiscRegSelect)
END_EVENT_TABLE()

/////////////////////////////// PUBLIC ///////////////////////////////////////
MiscRegisterPanel::MiscRegisterPanel() {};
MiscRegisterPanel::MiscRegisterPanel(
    wxWindow*           parent,
    wxWindowID          id,
    const wxPoint&      pos = wxDefaultPosition,
    const wxSize&       size = wxDefaultSize,
    long                style = wxTAB_TRAVERSAL
    ) : wxPanel(parent, id, pos, size, style)
{
    SetParent(parent);
    BuildMiscRegistersTable();
};

/////////////////////////////// PRIVATE    ///////////////////////////////////

// builds the register tree list
void
MiscRegisterPanel::BuildMiscRegistersTable(void) {
    uint32 i;
    m_pRegisterSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pRegisterInfo = new wxGrid(this, -1, wxDefaultPosition,
        wxDefaultSize);
    m_pRegisterInfo->CreateGrid(25, 1);
    m_pRegisterInfo->SetRowLabelValue(0, "");
    for (i = 0; i < 25; i++) {
        m_pRegisterInfo->SetRowLabelValue(i, "");
        m_pRegisterInfo->SetCellValue(i, 0, "");
    }

    m_pRegisterTree = new wxTreeCtrl(this, ID_REGTREE, wxDefaultPosition,
        wxDefaultSize,
        wxSP_3D|wxTR_NO_LINES|wxTR_FULL_ROW_HIGHLIGHT|wxTR_ROW_LINES);
    wxTreeItemId root;
    wxTreeItemId id;

    root = m_pRegisterTree->AddRoot("Registers");
    // id = m_pRegisterTree->AppendItem(root, "DMA");
    // for( i = 0; i < localDma->NumRegisters(); i++ ) {
    //     m_pRegisterTree->AppendItem(id, tDMA_REGISTERS[i]);
    // }
    id = m_pRegisterTree->AppendItem(root, "Intc");
	for( i = 0; i < 2; i++ ) {
	    m_pRegisterTree->AppendItem(id, tINTC_REGISTERS[i]);
	}
    id = m_pRegisterTree->AppendItem(root, "Timer");
    for( i = 0; i < 14; i++ ) {
        m_pRegisterTree->AppendItem(id, tTIMER_REGISTERS[i]);
    }
    id = m_pRegisterTree->AppendItem(root, "GS");
	for( i = 0; i < 2; i++ ) {
	    m_pRegisterTree->AppendItem(id, tGS_REGISTERS[i]);
	}
    id = m_pRegisterTree->AppendItem(root, "SIF");
    for( i = 0; i < 1; i++ ) {
        m_pRegisterTree->AppendItem(id, tSIF_REGISTERS[i]);
    }
    id = m_pRegisterTree->AppendItem(root, "FIFO");
	for( i = 0; i < 2; i++ ) {
	    m_pRegisterTree->AppendItem(id, tFIFO_REGISTERS[i]);
	}
    id = m_pRegisterTree->AppendItem(root, "GIF");
    for( i = 0; i < 8; i++ ) {
        m_pRegisterTree->AppendItem(id, tGIF_REGISTERS[i]);
    }
    id = m_pRegisterTree->AppendItem(root, "VIF0");
	for( i = 0; i < 18; i++ ) {
	    m_pRegisterTree->AppendItem(id, tVIF0_REGISTERS[i]);
	}
    id = m_pRegisterTree->AppendItem(root, "VIF1");
	for( i = 0; i < 22; i++ ) {
	    m_pRegisterTree->AppendItem(id, tVIF1_REGISTERS[i]);
	}

    m_pRegisterSizer->Add(m_pRegisterTree, 1, wxALIGN_LEFT|wxEXPAND, 10);
    m_pRegisterSizer->Add(m_pRegisterInfo, 1, wxALIGN_LEFT|wxEXPAND, 10);
    this->SetSizer(m_pRegisterSizer);
}

// event handler for the register tree
void
MiscRegisterPanel::OnMiscRegSelect(wxTreeEvent &event) {
    wxString parentLabel, childLabel;
    wxTreeItemId parentId;
    wxTreeItemId childId = event.GetItem();

    m_pRegisterTree->Toggle(childId);

    parentId = m_pRegisterTree->GetItemParent(childId);

    parentLabel = m_pRegisterTree->GetItemText(parentId);
    childLabel = m_pRegisterTree->GetItemText(childId);

    if ( parentLabel == "Registers" || parentLabel == "" ) {
        // no register selected
    } else if ( parentLabel == "Intc" ) {
        // childLabel;
    } else if ( parentLabel == "Intc" ) {
    } else if ( parentLabel == "Intc" ) {
    } else if ( parentLabel == "Intc" ) {
    } else if ( parentLabel == "Intc" ) {
    } else if ( parentLabel == "Intc" ) {
    }
}

// updates the info box with data for selected register
void
MiscRegisterPanel::UpdateInfoBox(int a, int b) {
    
}
