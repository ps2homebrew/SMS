#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sys/stat.h>

#include "main.h"
#include "parser.h"
#include "util.h"
#include "prefdef.h"
#include "dump.h"
#include "errors.h"
#include "linkproto_stub.h"

#include "vu.h"
#include "sif.h"
#include "timer.h"
#include "dma.h"
#include "gif.h"
#include "intc.h"
#include "ipu.h"
#include "fifo.h"
#include "vif0.h"
#include "vif1.h"
#include "gs.h"
#include "gif.h"
#include "vuBreakDialog.h"
#include "breakpoint.h"

using namespace std;

//---------------------------------------------------------------------------
extern MicroCode Instr;
extern VU VUchip;

uint32 getNregs(uint64);
uint32 validateRegsField(uint64);
uint32 getNLOOP(uint64);
uint32 getEOP(uint64);
uint64 getPRE(uint64);
uint32 getPRIM(uint64);
uint32 getFLGField(uint64);

static const wxString regSelectChoices[] =
{
    "P", "Q", "R", "I", "ACC", 
    "VF0", "VF1", "VF2", "VF3", "VF4", "VF5", "VF6", "VF7", 
    "VF8", "VF9", "VF10", "VF11", "VF12", "VF13", "VF14", "VF15", 
    "VF16", "VF17", "VF18", "VF19", "VF20", "VF21", "VF22", "VF23", 
    "VF24", "VF25", "VF26", "VF27", "VF28", "VF29", "VF30", "VF31",
    "VI00", "VI01", "VI02", "VI03", "VI04", "VI05", "VI06", "VI07",
    "VI08", "VI09", "VI10", "VI11", "VI12","VI13","VI14","VI15",
    "VI16", "VI17", "VI18", "VI19", "VI20","VI21","VI22","VI23",
    "VI24", "VI25", "VI26", "VI27", "VI28","VI29","VI30","VI31"
};

static int running = 0;
static int accumulatedTicks = 0;

static uint32 tagInitGs[24] = {
    0x00008005, 0x10000000, 0xe, 0x0,
    10<<16, 0x0, 0x4c, 0x0,                     // frame_1
    (10<<24)+(640*512*4/8192), 0x0, 0x4E, 0x0,  // zbuf_1
    32000, 32000, 0x18, 0x0,                    // xyoffset_1
    (639<<16), (511<<16), 0x40, 0x0,            // scissor_1
    0x0, 0x0, 0x42, 0x0                         // alpha_1
};

static uint32 tagClearGs[24] = {
    0x00008005, 0x10000000, 0xe, 0x0,
    0x6, 0x0, 0x0, 0x0,                         // prim
    0x00030000, 0x0, 0x47, 0x0,                 // test_1
    0x00000000, 0x3F800000, 0x1, 0x0,           // rgbaq
    (32000<<16) + 32000, 0x0, 0x5, 0x0,         // xyz2
    ((32000+(512<<4))<<16) + 32000+(640<<4), 0x0, 0x5, 0x0 // xyz2
};

//---------------------------------------------------------------------------
// GUI builder functions
//---------------------------------------------------------------------------
void
VUFrame::buildRegisterGrid(wxNotebook *book) {
	int i;
    // Build the Integer register tab
    gridIntRegisters = new wxGrid(book, ID_GRIDREGINT, wxDefaultPosition);
    gridIntRegisters->CreateGrid(1, 16);
    gridIntRegisters->SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    gridIntRegisters->EnableEditing(FALSE);
    gridIntRegisters->SetRowLabelValue(0, "X");
    gridIntRegisters->SetColLabelSize(int(wxWindow::GetCharHeight()*1.5));
    gridIntRegisters->SetRowLabelSize(charWidth*2);
    gridIntRegisters->SetRowSize(0, int(GetCharHeight()*1.5));
    gridIntRegisters->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    for (i = 0; i < 16; i++) {
        gridIntRegisters->SetColLabelValue(i, wxString::Format("vi%02d", i));
    }

    // Build the Float register tab
    gridFloatRegisters = new wxGrid(book, ID_GRIDREGFLOAT, wxDefaultPosition);
    gridFloatRegisters->CreateGrid(4, 32);
    gridFloatRegisters->SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    gridFloatRegisters->EnableEditing(FALSE);
    gridFloatRegisters->SetRowLabelValue(0, "X");
    gridFloatRegisters->SetRowLabelValue(1, "Y");
    gridFloatRegisters->SetRowLabelValue(2, "Z");
    gridFloatRegisters->SetRowLabelValue(3, "W");
    gridFloatRegisters->SetColLabelSize(int(GetCharHeight()*1.5));
    gridFloatRegisters->SetRowLabelSize(charWidth*3);
    gridFloatRegisters->SetRowSize(0, int(GetCharHeight()*1.5));
    gridFloatRegisters->SetRowSize(1, int(GetCharHeight()*1.5));
    gridFloatRegisters->SetRowSize(2, int(GetCharHeight()*1.5));
    gridFloatRegisters->SetRowSize(3, int(GetCharHeight()*1.5));
    gridFloatRegisters->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    for (i = 0; i < 32; i++) {
        gridFloatRegisters->SetColLabelValue(i, wxString::Format("vf%02d", i));
    }

    // Build the Special register tab
    gridSpecialRegisters = new wxGrid(book, ID_GRIDREGSPECIAL, wxDefaultPosition);
    gridSpecialRegisters->CreateGrid(4, 5);
    gridSpecialRegisters->SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    gridSpecialRegisters->EnableEditing(FALSE);
    gridSpecialRegisters->SetRowLabelValue(0, "X");
    gridSpecialRegisters->SetRowLabelValue(1, "Y");
    gridSpecialRegisters->SetRowLabelValue(2, "Z");
    gridSpecialRegisters->SetRowLabelValue(3, "W");
    gridSpecialRegisters->SetColLabelSize(int(GetCharHeight()*1.5));
    gridSpecialRegisters->SetRowLabelSize(charWidth*3);
    gridSpecialRegisters->SetRowSize(0, int(GetCharHeight()*1.5));
    gridSpecialRegisters->SetRowSize(1, int(GetCharHeight()*1.5));
    gridSpecialRegisters->SetRowSize(2, int(GetCharHeight()*1.5));
    gridSpecialRegisters->SetRowSize(3, int(GetCharHeight()*1.5));
    gridSpecialRegisters->SetColLabelValue(0, wxString("ACC"));
    gridSpecialRegisters->SetColLabelValue(1, wxString("Q"));
    gridSpecialRegisters->SetColLabelValue(2, wxString("P"));
    gridSpecialRegisters->SetColLabelValue(3, wxString("R"));
    gridSpecialRegisters->SetColLabelValue(4, wxString("I"));
    gridSpecialRegisters->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
}

wxPanel *
VUFrame::buildMemoryTable(wxNotebook *book) {
    uint32 i;
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    wxPanel *panel = new wxPanel(book, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER);

    wxString choices[5] = {"Fixed 0", "Fixed 4", "Fixed 12", "Fixed 15", "Float"};
    regRadioBox = new wxRadioBox(panel, ID_REGRADIOBOX,
        "Number representation", wxDefaultPosition,
        wxDefaultSize, 5, choices, 5);
    regRadioBox->SetSelection(4);
    gridMemory = new wxGrid(panel, ID_GRIDMEMORY, wxDefaultPosition,
        wxDefaultSize);
    gridMemory->SetDefaultCellAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    gridMemory->CreateGrid(MAX_VUDATA_SIZE, 4);
    gridMemory->EnableEditing(TRUE);
    gridMemory->SetColLabelValue(0, "X");
    gridMemory->SetColLabelValue(1, "Y");
    gridMemory->SetColLabelValue(2, "Z");
    gridMemory->SetColLabelValue(3, "W");
    gridMemory->SetColLabelSize(int(GetCharHeight()*1.5));
    gridMemory->SetRowLabelSize(charWidth*5);
    gridMemory->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    for (i = 0; i < MAX_VUDATA_SIZE; i++) {
        gridMemory->SetRowLabelValue(i, wxString::Format("%04d", i));
        gridMemory->SetRowSize(i, int(GetCharHeight()*1.5));
    }
    gridMemory->DisableDragGridSize();
    gridMemory->SetGridLineColour(*wxBLACK);
    sizer->Add(regRadioBox);
    sizer->Add(gridMemory, 1, wxEXPAND);
    panel->SetSizer(sizer);
    return panel;
}

void VUFrame::buildGIFTable(wxNotebook *nbook) {
    uint32 i;
    gridGIF = new wxGrid(nbook, ID_GRIDGIF, wxDefaultPosition);
    gridGIF->CreateGrid(MAX_VUDATA_SIZE+16, 2);
    gridGIF->SetColMinimalWidth(0, charWidth*8);
    gridGIF->SetColMinimalWidth(1, charWidth*40);
    gridGIF->SetColSize(0, charWidth*8);
    gridGIF->SetColSize(1, charWidth*40);
    gridGIF->EnableEditing(FALSE);
    gridGIF->SetColLabelValue(0, "");
    gridGIF->SetColLabelValue(1, "");
    gridGIF->SetColLabelSize(int(GetCharHeight()*1.5));
    gridGIF->SetRowLabelSize(charWidth*5);
    gridGIF->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    for (i = 0; i < MAX_VUDATA_SIZE; i++) {
        gridGIF->SetRowLabelValue(i, wxString::Format("%04d", i));
        gridGIF->SetRowSize(i, int(GetCharHeight()*1.5));
        gridGIF->SetCellAlignment(i, 0, wxALIGN_LEFT, wxALIGN_CENTRE);
        gridGIF->SetCellAlignment(i, 1, wxALIGN_LEFT, wxALIGN_CENTRE);
    }
    gridGIF->DisableDragGridSize();
    gridGIF->SetGridLineColour(*wxBLACK);
}

void
VUFrame::buildMiscRegistersTable(wxNotebook *nbook) {
    int i;
    boxMiscRegs = new wxBoxSizer(wxHORIZONTAL);
    panelMiscRegisters = new wxPanel(nbook, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER);
    regInfoBox = new wxGrid(panelMiscRegisters, ID_REGINFOBOX, wxDefaultPosition,
        wxDefaultSize);
    regInfoBox->CreateGrid(25, 1);
    regInfoBox->SetRowLabelValue(0, "");
    for (i = 0; i < 25; i++) {
        regInfoBox->SetRowLabelValue(i, "");
        regInfoBox->SetCellValue(i, 0, "");
    }

    miscRegTree = new wxTreeCtrl(panelMiscRegisters, ID_REGTREE, wxDefaultPosition,
        wxDefaultSize,
        wxSP_3D|wxTR_NO_LINES|wxTR_FULL_ROW_HIGHLIGHT|wxTR_ROW_LINES);
    wxTreeItemId root;
    wxTreeItemId id;

    root = miscRegTree->AddRoot("Registers");
    id = miscRegTree->AppendItem(root, "DMA");
    for( i = 0; i < DMAnREGISTERS; i++ ) {
        dmaItemIds[i] = miscRegTree->AppendItem(id, tDMA_REGISTERS[i]);
    }
    id = miscRegTree->AppendItem(root, "Intc");
	// for( i = 0; i < INTC::nREGISTERS; i++ ) {
	//     intcItemIds[i] = miscRegTree->AppendItem(id, tINTC_REGISTERS[i]);
	// }
    id = miscRegTree->AppendItem(root, "Timer");
    for( i = 0; i < TIMERnREGISTERS; i++ ) {
        timerItemIds[i] = miscRegTree->AppendItem(id, tTIMER_REGISTERS[i]);
    }
    id = miscRegTree->AppendItem(root, "GS");
	// for( i = 0; i < GS::nREGISTERS; i++ ) {
	//     gsItemIds[i] = miscRegTree->AppendItem(id, tGS_REGISTERS[i]);
	// }
    id = miscRegTree->AppendItem(root, "SIF");
    for( i = 0; i < SIFnREGISTERS; i++ ) {
        dmaItemIds[i] = miscRegTree->AppendItem(id, tSIF_REGISTERS[i]);
    }
    id = miscRegTree->AppendItem(root, "FIFO");
	// for( i = 0; i < FIFO::nREGISTERS; i++ ) {
	//     fifoItemIds[i] = miscRegTree->AppendItem(id, tFIFO_REGISTERS[i]);
	// }
    id = miscRegTree->AppendItem(root, "GIF");
    for( i = 0; i < GIFnREGISTERS; i++ ) {
        gifItemIds[i] = miscRegTree->AppendItem(id, tGIF_REGISTERS[i]);
    }
    id = miscRegTree->AppendItem(root, "VIF0");
	// for( i = 0; i < VIF0::nREGISTERS; i++ ) {
	//     vif0ItemIds[i] = miscRegTree->AppendItem(id, tVIF0_REGISTERS[i]);
	// }
    id = miscRegTree->AppendItem(root, "VIF1");
	// for( i = 0; i < VIF1::nREGISTERS; i++ ) {
	//     vif1ItemIds[i] = miscRegTree->AppendItem(id, tVIF1_REGISTERS[i]);
	// }

    boxMiscRegs->Add(miscRegTree, 1, wxALIGN_LEFT|wxEXPAND, 10);
    boxMiscRegs->Add(regInfoBox, 1, wxALIGN_LEFT|wxEXPAND, 10);
    panelMiscRegisters->SetSizer(boxMiscRegs);
}

void
VUFrame::OnMiscRegSelect(wxTreeEvent &event) {
    miscRegTree->Toggle(event.GetItem());
}

void
VUFrame::updateInfoBox(int a, int b) {
}

void
VUFrame::buildCodeTable(wxNotebook *nbook) {
    uint32 i;
    gridCode = new wxGrid(nbook, ID_GRIDCODE, wxDefaultPosition);
    // take num opcodes * 2 to allow space for labels.
    gridCode->CreateGrid(MAX_VUCODE_SIZE*2, 5);
    gridCode->EnableEditing(FALSE);
    gridCode->SetColLabelValue(0, "P");
    gridCode->SetColLabelValue(1, "B");
    gridCode->SetColLabelValue(2, "T");
    gridCode->SetColLabelValue(3, "Upper");
    gridCode->SetColLabelValue(4, "Lower");
    gridCode->SetColLabelSize(int(GetCharHeight()*1.5));
    gridCode->SetRowLabelSize(charWidth*5);
    gridCode->SetColSize(0, charWidth*2);
    gridCode->SetColSize(1, charWidth*2);
    gridCode->SetColSize(2, charWidth*2);
    gridCode->SetColSize(2, charWidth*2);
    gridCode->SetColSize(3, charWidth*20);
    gridCode->SetColSize(4, charWidth*20);
    gridCode->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    for (i = 0; i < MAX_VUCODE_SIZE; i++) {
        gridCode->SetRowLabelValue(i, wxString::Format("%04d", i));
        gridCode->SetCellValue(i, 1, wxString("."));
        gridCode->SetRowSize(i, int(GetCharHeight()*1.5));
    }
    gridCode->DisableDragGridSize();
    gridCode->SetGridLineColour(*wxBLACK);
}

void
VUFrame::buildFlagsPanel(wxNotebook *book) {
    flagsDetail = new wxPanel(book, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER);

    wxBoxSizer  *topSizer = new wxBoxSizer(wxVERTICAL);
    wxGridSizer *clipflags = new wxGridSizer(2, 4, 1, 1);
    wxGridSizer *statusflags = new wxGridSizer(2, 12, 1, 1);
    wxGridSizer *macflags = new wxGridSizer(2, 4, 1, 1);
    wxSize size;

    // clip flags
    size = wxSize(7*charWidth, (int)(1.5*GetCharHeight()));
    clipflags->Add(new wxStaticText(flagsDetail, -1, "3rd prv", wxDefaultPosition,
            size), 0, wxADJUST_MINSIZE, 1);
    clipflags->Add(new wxStaticText(flagsDetail, -1, "2nd prv", wxDefaultPosition,
            size), 0, wxADJUST_MINSIZE, 1);
    clipflags->Add(new wxStaticText(flagsDetail, -1, "previous", wxDefaultPosition,
            size), 0, wxADJUST_MINSIZE, 1);
    clipflags->Add(new wxStaticText(flagsDetail, -1, "current", wxDefaultPosition,
            size), 0, wxADJUST_MINSIZE, 1);

    clip3 = new wxTextCtrl(flagsDetail, -1, "000000", wxDefaultPosition, size, wxTE_READONLY);
    clip2 = new wxTextCtrl(flagsDetail, -1, "000000", wxDefaultPosition, size, wxTE_READONLY);
    clip1 = new wxTextCtrl(flagsDetail, -1, "000000", wxDefaultPosition, size, wxTE_READONLY);
    clip0 = new wxTextCtrl(flagsDetail, -1, "000000", wxDefaultPosition, size, wxTE_READONLY);

    clipflags->Add(clip3, 0, wxLEFT, 1);
    clipflags->Add(clip2, 0, wxLEFT, 1);
    clipflags->Add(clip1, 0, wxLEFT, 1);
    clipflags->Add(clip0, 0, wxLEFT, 1);
    
    // statusflags
    size = wxSize(3*charWidth, (int)(1.5*GetCharHeight()));
    statusflags->Add(new wxStaticText(flagsDetail, -1, "DS", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "IS", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "OS", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "US", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "SS", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "ZS", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "D", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "I", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "O", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "U", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "S", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(flagsDetail, -1, "Z", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);

    fds = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fis = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fos = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fzs = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fss = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fus = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fi = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fo = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fs = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fz = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fu = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fd = new wxTextCtrl(flagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);

    statusflags->Add(fds, 0, wxLEFT, 1);
    statusflags->Add(fis, 0, wxLEFT, 1);
    statusflags->Add(fos, 0, wxLEFT, 1);
    statusflags->Add(fus, 0, wxLEFT, 1);
    statusflags->Add(fss, 0, wxLEFT, 1);
    statusflags->Add(fzs, 0, wxLEFT, 1);
    statusflags->Add(fd, 0, wxLEFT, 1);
    statusflags->Add(fi, 0, wxLEFT, 1);
    statusflags->Add(fo, 0, wxLEFT, 1);
    statusflags->Add(fu, 0, wxLEFT, 1);
    statusflags->Add(fs, 0, wxLEFT, 1);
    statusflags->Add(fz, 0, wxLEFT, 1);

    // mac flags
    size = wxSize(5*charWidth, (int)(1.5*GetCharHeight()));
    macflags->Add(new wxStaticText(flagsDetail, -1, "O xyzw"), 0,
        wxADJUST_MINSIZE, 1);
    macflags->Add(new wxStaticText(flagsDetail, -1, "U xyzw"), 0,
        wxADJUST_MINSIZE, 1);
    macflags->Add(new wxStaticText(flagsDetail, -1, "S xyzw"), 0,
        wxADJUST_MINSIZE, 1);
    macflags->Add(new wxStaticText(flagsDetail, -1, "Z xyzw"), 0,
        wxADJUST_MINSIZE, 1);

    MOf = new wxTextCtrl(flagsDetail, -1, "0000", wxDefaultPosition, size,
        wxTE_READONLY);
    MUf = new wxTextCtrl(flagsDetail, -1, "0000", wxDefaultPosition, size,
        wxTE_READONLY);
    MSf = new wxTextCtrl(flagsDetail, -1, "0000", wxDefaultPosition, size,
        wxTE_READONLY);
    MZf = new wxTextCtrl(flagsDetail, -1, "0000", wxDefaultPosition, size,
        wxTE_READONLY);

    macflags->Add(MOf, 0, wxLEFT, 1);
    macflags->Add(MUf, 0, wxLEFT, 1);
    macflags->Add(MSf, 0, wxLEFT, 1);
    macflags->Add(MZf, 0, wxLEFT, 1);

    // show
    topSizer->Add(new wxStaticText(flagsDetail, -1, "CLIP -z+z-y+y-x+x ( 4 times )"),
            0, wxADJUST_MINSIZE, 1);
    topSizer->Add(clipflags);
    topSizer->Add(new wxStaticText(flagsDetail, -1, "Status Flags"),
            0, wxADJUST_MINSIZE, 1);
    topSizer->Add(statusflags);
    topSizer->Add(new wxStaticText(flagsDetail, -1, "Mac Flags"),
            0, wxADJUST_MINSIZE, 1);
    topSizer->Add(macflags);
	flagsDetail->SetSizerAndFit(topSizer);
	flagsDetail->SetAutoLayout(TRUE);
	flagsDetail->Layout();
}

//---------------------------------------------------------------------------
// On Event functions
//---------------------------------------------------------------------------

void
VUFrame::OnRestart(wxCommandEvent &WXUNUSED(event)) {
    int i,j;

	accumulatedTicks = 0;

    if(Status != READY) {
        return;
    }
    gridCode->SetCellBackgroundColour(Previous, 3, *wxWHITE);
    gridCode->SetCellBackgroundColour(Previous, 4, *wxWHITE);

    VUchip.Reset();
    DrawMemory();
    DrawProgram();
    if(LoadCode((char *)codeFile.GetFullPath().c_str())) {
        for(i=0; i< Instr.nInstructionDef; i++) {
            for(j=0; j<15; j++) {
                Instr.Instr[i].lastthr[j]=0;
            }
        }
        Status=READY;
        DrawProgram();
        txtDebug->AppendText("VU Code reloaded from file: " +
            codeFile.GetFullPath() + "\n");
        txtDebug->AppendText("Status=READY\n");
        InstuctionStatus();
    } else{
        wxMessageBox(wxString::Format("Syntax Error in line: %d",
                VUchip.NInstructions+1), "", wxOK|wxICON_INFORMATION, this);
        txtDebugFailed("Failed to load VU Code from file: " + 
            codeFile.GetFullPath() + "\n");
        txtDebug->AppendText("Status=EMPTY\n");
        VUchip.Reset();
        Status = wxRESET;
    }

    if(dataFile.GetFullPath() != "") {
        LoadMemory(dataFile);
    } else {
        txtDebugFailed(wxString("Failed to load VU Data from file: " + 
            dataFile.GetFullPath() + "\n"));
    }
    gridCode->SetGridCursor(0, 3);
}

//---------------------------------------------------------------------------
void
VUFrame::OnStep(wxCommandEvent &WXUNUSED(event)) {
    int cur = VUchip.PC;
    VUchip.SetCallback(this, VUFrame::wrapper_DebugTic);
    VUchip.SetXGKICKCallback(this, VUFrame::wrapper_XGKICK);
    VUchip.Tic();
    gridCode->SetCellValue(Previous, 0, wxString(""));
    gridCode->SetCellValue(LineInstruction(cur), 2, wxString::Format("%d",
            VUchip.program[cur].tics));

	// sort of a kludge
	if ( running == 0 ) {
		gridCode->SetCellBackgroundColour(Previous, 3, *wxWHITE);
		gridCode->SetCellBackgroundColour(Previous, 4, *wxWHITE);
		gridCode->SetCellValue(LineInstruction(cur), 0, wxString(">"));
		gridCode->SetCellBackgroundColour(LineInstruction(cur), 3, cCurCode);
		gridCode->SetCellBackgroundColour(LineInstruction(cur), 4, cCurCode);
		gridCode->SetGridCursor(LineInstruction(cur), 4);

		Previous = LineInstruction(cur);
		if ( Previous < cur ) {
			gridCode->MoveCursorDown(FALSE);
		} else if ( cur < Previous ) {
			gridCode->MoveCursorUp(FALSE);
		}
	}

	accumulatedTicks += VUchip.program[cur].tics;

	if ( running == 0 ) {
		if ( VUchip.memoryUpdate ) {
			DrawMemory();
		}
		FastRegisterUpdate();
		FlagsUpdate(); 
	}
    InstuctionStatus();
}

void
VUFrame::OnRun(wxCommandEvent &event) {
    uint32 i = 0;
	running = 1;
    Breakpoint *bp = Breakpoint::Instance(); 
    while( (VUchip.program[VUchip.PC].flg!='E') &&
            (i < MAX_VUCODE_SIZE) &&
            !bp->check()
            ) {
        OnStep(event);
        i++;
    }
	gridCode->SetGridCursor(LineInstruction(VUchip.PC), 4);
	gridCode->MoveCursorDown(FALSE);
	DrawMemory();
	FastRegisterUpdate();
	FlagsUpdate(); 
	// kludge
	running = 0;
}

//---------------------------------------------------------------------------
void
VUFrame::OnSettings(wxCommandEvent &WXUNUSED(event)) {
    m_prefs = new Prefs();
    PreferenceDlg (this, m_prefs);
    // Save away current settings
    VUFrame::SetSettings();
}

void
VUFrame::SetSettings() {
    wxConfig *conf = new wxConfig(wxTheApp->GetAppName());
    wxString key = PAGE_LOAD;
    autoLoadLast = conf->Read(key + _T("/") + AUTOLOADLAST, 0L);
    dataFile.Assign(conf->Read(key + _T("/") + LASTFILEMEM)); 
    codeFile.Assign(conf->Read(key + _T("/") + LASTFILECODE)); 
	memStateFile.Assign(conf->Read(key + _T("/") + MEMSTATEFILE));
	regStateFile.Assign(conf->Read(key + _T("/") + REGSTATEFILE));
	mnemonicFile.Assign(conf->Read(key + _T("/") + MNEMONICFILE));
    codeAdressStyle = 0;

    key = PAGE_REMOTE;
    autoGSExec = conf->Read(key + _T("/") + AUTOGSEXEC, 0L);
    binTmpFile = conf->Read(key + _T("/") + BINTMPFILE);
    datTmpFile = conf->Read(key + _T("/") + DATTMPFILE);
    regTmpFile = conf->Read(key + _T("/") + REGTMPFILE);
    gsTmpFile = conf->Read(key + _T("/") + GSTMPFILE);
    dumpMemCmd = conf->Read(key + _T("/") + DUMPMEMCMD);
    dumpRegCmd = conf->Read(key + _T("/") + DUMPREGCMD);
    gsExecCmd = conf->Read(key + _T("/") + GSEXECCMD);

    key = PAGE_STYLE;
    wxString fontname = conf->Read(key + _T("/") + FONTNAME);
    fontname = conf->Read(key + _T("/") + FONTNAME);
    if ( gridCode != NULL ) {
        wxFont font (10, wxMODERN, wxNORMAL, wxNORMAL, false, fontname);
        gridCode->SetDefaultCellFont(font);
    }
    // set font here.
    // set color for text here
    // set bg color here.

    key = PAGE_GIF;
    yoffset = conf->Read(key + _T("/") + YOFFSET, 0L);
    xoffset = conf->Read(key + _T("/") + XOFFSET, 0L);
    sendPrim = conf->Read(key + _T("/") + SENDPRIM, 0L);
    tagShow = conf->Read(key + _T("/") + TAGSHOW, 0L);
    ClrColor = conf->Read(key + _T("/") + CLRCOLOR, 0L);
    scissorX = conf->Read(key + _T("/") + SCISSOR_X, 0L);
    scissorY = conf->Read(key + _T("/") + SCISSOR_Y, 0L);
    prim = conf->Read(key + _T("/") + PRIM, 0L);
}

//---------------------------------------------------------------------------
void VUFrame::OnQuit(wxCommandEvent &WXUNUSED(event)) {
    wxMessageDialog *ask = new wxMessageDialog(this, "Are you sure?", "Exit",
        wxOK | wxCANCEL);
    if (ask->ShowModal() == wxID_OK) {
        if ( dataFile.GetFullPath()  != "" ) {
            wxConfig *conf = new wxConfig(wxTheApp->GetAppName());
            wxString key = PAGE_LOAD;
            conf->Write(key + _T("/") + LASTFILEMEM, dataFile.GetFullPath());
            delete conf;
        }
        if (codeFile.GetFullPath() != "") {
            wxConfig *conf = new wxConfig(wxTheApp->GetAppName());
            wxString key = PAGE_LOAD;
            conf->Write(key + _T("/") + LASTFILECODE, codeFile.GetFullPath());
            delete conf;
        }
        if (mnemonicFile.GetFullPath() != "") {
            wxConfig *conf = new wxConfig(wxTheApp->GetAppName());
            wxString key = PAGE_LOAD;
            conf->Write(key + _T("/") + MNEMONICFILE, mnemonicFile.GetFullPath());
            delete conf;
        }
        if ( dumpIsOpen() >= 0 ) {
            dumpClose();
        }
        Close(TRUE);
    }
}

// All the Remote gui functions here.

//---------------------------------------------------------------------------
void
VUFrame::OnVu0All(wxCommandEvent &WXUNUSED(event)) {
    if ( binTmpFile == "" ) {
        wxMessageBox("No binary temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( datTmpFile == "" ) {
        wxMessageBox("No data temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( dumpVU((char *)binTmpFile.c_str(), (char *)datTmpFile.c_str(), 0) == 0) {
        LoadCode((char *)binTmpFile.c_str());
        DrawProgram();
        LoadMemory(datTmpFile);
    } else {
        wxMessageBox("Unable to fetch vpu0 content\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }

    if ( dumpVURegisters((char *)regTmpFile.c_str(), 0) == 0) {
        VUchip.LoadRegisters((char *)regTmpFile.c_str()); 
        FastRegisterUpdate();
    } else {
        wxMessageBox("Unable to fetch vpu0 registers from PS2\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }
}

//---------------------------------------------------------------------------
void
VUFrame::OnVu1All(wxCommandEvent &WXUNUSED(event)) {
    if ( binTmpFile == "" ) {
        wxMessageBox("No binary temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( datTmpFile == "" ) {
        wxMessageBox("No data temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( dumpVU((char *)binTmpFile.c_str(), (char *)datTmpFile.c_str(), 1) == 0) {
        LoadCode((char *)binTmpFile.c_str());
        DrawProgram();
        LoadMemory(datTmpFile);
    } else {
        wxMessageBox("Unable to fetch vpu1 content\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }

    if ( dumpVURegisters((char *)regTmpFile.c_str(), 1) == 0) {
        VUchip.LoadRegisters((char *)regTmpFile.c_str()); 
        FastRegisterUpdate();
    } else {
        wxMessageBox("Unable to fetch vpu1 registers from PS2\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }
}

//---------------------------------------------------------------------------
void VUFrame::OnVu0(wxCommandEvent &WXUNUSED(event)) {
    if ( binTmpFile == "" ) {
        wxMessageBox("No binary temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( datTmpFile == "" ) {
        wxMessageBox("No data temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( dumpVU((char *)binTmpFile.c_str(), (char *)datTmpFile.c_str(), 0) == 0) {
        LoadCode((char *)binTmpFile.c_str());
        DrawProgram();
        LoadMemory(datTmpFile);
    } else {
        wxMessageBox("Unable to fetch vpu0 content\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }
}

//---------------------------------------------------------------------------
void VUFrame::OnVu1(wxCommandEvent &WXUNUSED(event)) {
    if ( binTmpFile == "" ) {
        wxMessageBox("No binary temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( datTmpFile == "" ) {
        wxMessageBox("No data temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( dumpVU((char *)binTmpFile.c_str(), (char *)datTmpFile.c_str(), 1) == 0) {
        LoadCode((char *)binTmpFile.c_str());
        DrawProgram();
        LoadMemory(datTmpFile);
    } else {
        wxMessageBox("Unable to fetch vpu1 content\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }
}

//---------------------------------------------------------------------------
void VUFrame::OnRegs(wxCommandEvent &WXUNUSED(event)) {
    if ( regTmpFile == "" ) {
        wxMessageBox("No register temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( dumpRegisters((char *)regTmpFile.c_str()) == 0) {
        printf("parse dma, intc, gif, vif register dump\n");
    } else {
        wxMessageBox("Unable to fetch registers from PS2\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }
}

void VUFrame::OnRegsVu0(wxCommandEvent &WXUNUSED(event)) {
    if ( regTmpFile == "" ) {
        wxMessageBox("No register temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( dumpVURegisters((char *)regTmpFile.c_str(), 0) == 0) {
        VUchip.LoadRegisters((char *)regTmpFile.c_str()); 
        FastRegisterUpdate();
    } else {
        wxMessageBox("Unable to fetch vpu0 registers from PS2\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }
}

void VUFrame::OnRegsVu1(wxCommandEvent &WXUNUSED(event)) {
    if ( regTmpFile == "" ) {
        wxMessageBox("No register temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( dumpVURegisters((char *)regTmpFile.c_str(), 1) == 0) {
        VUchip.LoadRegisters((char *)regTmpFile.c_str()); 
        FastRegisterUpdate();
    } else {
        wxMessageBox("Unable to fetch vpu1 registers from PS2\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }
}

void
VUFrame::OnGSInit(wxCommandEvent &WXUNUSED(event)) {
    if ( gsTmpFile == "" ) {
        wxMessageBox("No GS temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }

    tagInitGs[12] = yoffset<<4;
    tagInitGs[13] = xoffset<<4;
    tagInitGs[16] = scissorX<<16;
    tagInitGs[17] = scissorY<<16;
    
    if ( dumpDisplayList((char *)gsTmpFile.c_str(), tagInitGs, 96) != 0) {
        wxMessageBox("Unable to init GS on PS2\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }
}

void
VUFrame::OnCLR(wxCommandEvent &WXUNUSED(event)) {
    if ( gsTmpFile == "" ) {
        wxMessageBox("No GS temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }

    tagClearGs[12] = ClrColor;
    tagClearGs[20] = (((scissorY<<4)+(yoffset<<4))<<16) +
        ((scissorX<<4)+(xoffset<<4));

    if ( dumpDisplayList((char *)gsTmpFile.c_str(), tagClearGs, 96) != 0) {
        wxMessageBox("Unable to init GS on PS2\nNo contact with ps2link client.", "",
            wxOK|wxICON_INFORMATION, this);
    }
}

//
void VUFrame::OnHelp(wxCommandEvent &WXUNUSED(event)) {
    wxMessageBox("Unimplemented.", "Help", wxOK|wxICON_INFORMATION, this);
}

void
VUFrame::OnLoadMem(wxCommandEvent &WXUNUSED(event)) {
    dlgOpenFile = new wxFileDialog(this, "Choose a data file");
    if (dlgOpenFile->ShowModal() == wxID_OK &&
        dlgOpenFile->GetFilename() != "") {
        dataFile.Assign(dlgOpenFile->GetPath());
        LoadMemory(dataFile);
    }
}

int
VUFrame::LoadMemory(wxFileName file) {
    FILE *fd;
    struct stat sb;
    uint32 i;

    if ((stat(file.GetFullPath().c_str(), &sb)) != 0 ) {
        return E_FILE_OPEN;
    }
    if ((fd = fopen(file.GetFullPath().c_str(), "rb")) != NULL) {
        for(i = 0; i < MAX_VUDATA_SIZE && i < sb.st_size/16; i++) {
            uint32 x, y, z, w;
            fread(&x, sizeof(int32), 1, fd);
            fread(&y, sizeof(int32), 1, fd);
            fread(&z, sizeof(int32), 1, fd);
            fread(&w, sizeof(int32), 1, fd);
            VUchip.dataMem[i].x = x;
            VUchip.dataMem[i].y = y;
            VUchip.dataMem[i].z = z;
            VUchip.dataMem[i].w = w;
            if ( tagShow == 0 ) {
                if (
                    (validateRegsField(((uint64)z<<32)+w) == getNregs(((uint64)x<<32)+y))
                    && ((getFLGField(((uint64)x<<32)+y) == 0))
                   ) {
                    VUchip.dataMem[i].gif = true;
                } else {
                    VUchip.dataMem[i].gif = false;
                }
            } else {
                VUchip.dataMem[i].gif = false;
            }
        }
        fclose(fd);
        DrawMemory();
        txtDebug->AppendText("VU Data loaded from file: " +
            file.GetFullPath() + "\n");
    } else {
        txtDebugFailed(wxString("Failed to load VU Data from file: " +
                dataFile.GetFullPath() + "\n"));
        return E_FILE_OPEN;
    }
    return 0;
}

void VUFrame::txtDebugFailed(wxString message) {
        txtDebug->SetDefaultStyle(wxTextAttr(wxNullColour, cWarning));
        txtDebug->AppendText(message);
        txtDebug->SetDefaultStyle(wxTextAttr(wxNullColour, *wxWHITE));
}

//---------------------------------------------------------------------------
void VUFrame::OnSaveCode(wxCommandEvent &WXUNUSED(event)) {
    dlgSaveFile = new wxFileDialog(this, "Choose a file");
    if ( dlgSaveFile->ShowModal() ) {
    }
}

void VUFrame::OnSaveState(wxCommandEvent &WXUNUSED(event)) {
	FILE *fd;
	float x,y,z,w;
	int r;
    uint32 i;
	// Save memory
	if ( (fd = fopen(memStateFile.GetFullPath().c_str(), "wb")) != NULL ) {
		for(i = 0; i < MAX_VUDATA_SIZE; i++ ) {
			x = VUchip.dataMem[i].x;
			y = VUchip.dataMem[i].y;
			z = VUchip.dataMem[i].z;
			w = VUchip.dataMem[i].w;
			fwrite(&x, sizeof(float), 1, fd);
			fwrite(&y, sizeof(float), 1, fd);
			fwrite(&z, sizeof(float), 1, fd);
			fwrite(&w, sizeof(float), 1, fd);
		}	
	} else {
		txtDebugFailed(wxString("Save vu data to file: %s failed.: " +
			memStateFile.GetFullPath() + "\n"));
	}
	// Save regs
	if ( (fd = fopen(regStateFile.GetFullPath().c_str(), "wb")) != NULL ) {
		for(i = 0; i < 32; i++) {
			x = VUchip.RegFloat[i].x();
			y = VUchip.RegFloat[i].y();
			z = VUchip.RegFloat[i].z();
			w = VUchip.RegFloat[i].w();
			fwrite(&x, sizeof(float), 1, fd);
			fwrite(&y, sizeof(float), 1, fd);
			fwrite(&z, sizeof(float), 1, fd);
			fwrite(&w, sizeof(float), 1, fd);
		}
		for(i = 0; i < 16; i++) {
			r = VUchip.RegInt[i].value();
			fwrite(&r, sizeof(int), 1, fd);
		}
	} else {
		txtDebugFailed(wxString("Save vu registers to file: %s failed.: " +
			regStateFile.GetFullPath() + "\n"));
	}
}

void
VUFrame::OnReset(wxCommandEvent &WXUNUSED(event)) {
	accumulatedTicks = 0;
    VUchip.Reset();
    InstFill();
    DrawMemory();
    DrawProgram();
    FastRegisterUpdate();
    Status = RESET;
    txtDebug->AppendText("Status=EMPTY\n");
}

void
VUFrame::InstFill() {
    int i;
    for(i = 0; i < MAX_VUCODE_SIZE; i++) {
        insert(strdup("nop"), strdup("loi"), strdup(""), strdup("0x0"), i);
    }
}


void
VUFrame::OnSelectCodeCell(wxCommandEvent &WXUNUSED(event)) {
    gridCode->SetCellValue(gridCode->GetCursorRow(), 0, wxString(""));
    gridCode->SetCellValue(gridCode->GetCursorRow(), 0, wxString(">"));
}

void
VUFrame::OnCellChange(wxGridEvent &event) {
    char *end_ptr;
    int num;
    float fnum;
    char cNum[50];
    wxString cell = gridMemory->GetCellValue(event.GetRow(),
        event.GetCol());
    num = (int)strtol(cell.c_str(), &end_ptr, 0);
    if ( !((cell.c_str() != '\0') && (*end_ptr == '\0'))) {
        fnum = (float)strtod(cell.c_str(), &end_ptr);
        num = *((int *)&fnum);
    }
    
    if ( (*end_ptr == '\0') ) {
        switch (event.GetCol()) {
            case 0:
                VUchip.dataMem[event.GetRow()].x = num;
                break;
            case 1:
                VUchip.dataMem[event.GetRow()].y = num;
                break;
            case 2:
                VUchip.dataMem[event.GetRow()].z = num;
                break;
            case 3:
                VUchip.dataMem[event.GetRow()].w = num;
                break;
        }
    } else {
        switch (event.GetCol()) {
            case 0:
                num = VUchip.dataMem[event.GetRow()].x;
                break;
            case 1:
                num = VUchip.dataMem[event.GetRow()].y;
                break;
            case 2:
                num = VUchip.dataMem[event.GetRow()].z;
                break;
            case 3:
                num = VUchip.dataMem[event.GetRow()].w;
                break;
            default:
                break;
        }
        // Redraw
    }

    switch(regRadioBox->GetSelection()) {
        case 0:
            sprintf(cNum,"%ld", num);
            break;
        case 1:
            sprintf(cNum,"%.4f", num/16.0f);
            break;
        case 2:
            sprintf(cNum,"%.12f", num/4096.0f);
            break;
        case 3:
            sprintf(cNum,"%.15f", num/32768.0f);
            break;
        case 4:
            sprintf(cNum,"%f", num);
        default:
            break;
    }
    gridMemory->SetCellValue(event.GetRow(), event.GetCol(), cNum);
}

//---------------------------------------------------------------------------
void
VUFrame::OnBreakpoint(wxGridEvent &event) {
    vuBreakDialog(this, event.GetRow());
}

//---------------------------------------------------------------------------
void
VUFrame::OnLoadProject(wxCommandEvent &WXUNUSED(event)) {
    char line[255];
    char *ptr;
	wxFileDialog *dlgOpenFile = new wxFileDialog(this, "Choose a project file");
	if(dlgOpenFile->ShowModal() == wxID_OK) {
        ifstream fin(dlgOpenFile->GetPath());
        fin.getline(line, 256);
        if(strstr(line, "codefile") != 0) {
            ptr = strchr(line, '=');
            codeFile.Assign(ptr+1);
        } else if ( strstr(line, "datafile")  != 0) {
            ptr = strchr(line, '=');
            dataFile.Assign(ptr+1);
        }
        fin.getline(line, 256);
        if(strstr(line, "codefile") != 0) {
            ptr = strchr(line, '=');
            codeFile.Assign(ptr+1);
        } else if ( strstr(line, "datafile")  != 0) {
            ptr = strchr(line, '=');
            dataFile.Assign(ptr+1);
        }
        fin.close();
        codeFile.PrependDir(dlgOpenFile->GetDirectory());
        dataFile.PrependDir(dlgOpenFile->GetDirectory());
        if ( LoadCode((char *)codeFile.GetFullPath().c_str()) ) {
            Status = READY;
            DrawProgram();
            txtDebug->AppendText(wxString("VU Code loaded from file: " +
                    codeFile.GetFullPath() + "\n"));
            InstuctionStatus();
        } else {
            txtDebugFailed(wxString("Failed to load VU Code from file: " +
                    codeFile.GetFullPath() + "\n"));
        }
        LoadMemory(dataFile);
	}
}

//---------------------------------------------------------------------------
void
VUFrame::OnLoadVIF(wxCommandEvent &WXUNUSED(event)) {
    dlgOpenFile = new wxFileDialog(this, "Choose a VIF file");
    dlgOpenFile->SetWildcard("*.bin");
    if (dlgOpenFile->ShowModal() == wxID_OK) {
        vifFile.Assign(dlgOpenFile->GetPath());
        VIF *vif = new VIF(vifFile.GetFullPath().c_str());
        while(!vif->eof()) {
            vif->read();
        }
        DrawMemory();
        DrawProgram();
    }
}

//---------------------------------------------------------------------------
void
VUFrame::OnLoadDMA(wxCommandEvent &WXUNUSED(event)) {
    dlgOpenFile = new wxFileDialog(this, "Choose a DMA file");
    if (dlgOpenFile->ShowModal() == wxID_OK) {
        vifFile.Assign(dlgOpenFile->GetPath());
        DMA *dma = new DMA(vifFile.GetFullPath().c_str());
        while(!dma->eof()) {
            dma->read();
        }
    }
}

//---------------------------------------------------------------------------
void
VUFrame::OnLoadCode(wxCommandEvent &WXUNUSED(event)) {
    dlgOpenFile = new wxFileDialog(this, "Choose a code file");
    if (dlgOpenFile->ShowModal() == wxID_OK) {
        codeFile.Assign(dlgOpenFile->GetPath());
        if(LoadCode((char *)dlgOpenFile->GetPath().c_str())) {
            Status = READY;
            DrawProgram();
            txtDebug->AppendText(wxString("VU Code loaded from file: " +
                    codeFile.GetFullPath() + "\n"));
            InstuctionStatus();
        } else {
            txtDebugFailed(wxString("Failed to load VU Code from file: " +
                    codeFile.GetFullPath() + "\n"));
        }
    }
}

void
VUFrame::OnNotebookOne(wxNotebookEvent &WXUNUSED(event)) {
    // if ( event.GetOldSelection() != -1 ) {
    //     DrawMemory();
    // }
}

void
VUFrame::OnRegRadioBox(wxCommandEvent &WXUNUSED(event)) {
    DrawMemory();
    gridMemory->ForceRefresh();
}

void
VUFrame::updateStatusBar(void) {
    uint32 col = statusBarReg[1];
    switch(statusBarReg[0]) {
        case ID_GRIDREGINT:
            statusBar->PushStatusText(
                wxString::Format("Reg: VI%02d", col), 0);
            statusBar->PushStatusText(
                wxString::Format("Stall: %d", VUchip.RegInt[col].stall()), 1);
            statusBar->PushStatusText(
                wxString::Format("Last Read: %d", VUchip.RegInt[col].stall()), 2);
            statusBar->PushStatusText(
                wxString::Format("Last Write: %d", VUchip.RegInt[col].stall()), 3);
            break;
        case ID_GRIDREGFLOAT:
            statusBar->PushStatusText(
                wxString::Format("Reg: VF%02d", col), 0);
            statusBar->PushStatusText(
                wxString::Format("Stall: %d", VUchip.RegFloat[col].stall()), 1);
            statusBar->PushStatusText(
                wxString::Format("Last Read: %d", VUchip.RegFloat[col].stall()), 2);
            statusBar->PushStatusText(
                wxString::Format("Last Write: %d", VUchip.RegFloat[col].stall()), 3);
            break;
        case ID_GRIDREGSPECIAL:
            statusBar->PushStatusText(
                wxString::Format("Reg: VI%02d", col), 0);
            statusBar->PushStatusText(
                wxString::Format("Stall: %d", VUchip.RegInt[col].stall()), 1);
            statusBar->PushStatusText(
                wxString::Format("Last Read: %d", VUchip.RegInt[col].stall()), 2);
            statusBar->PushStatusText(
                wxString::Format("Last Write: %d", VUchip.RegInt[col].stall()), 3);
            break;
    }
}

void
VUFrame::OnGridLabel(wxGridEvent &event) {
    statusBarReg[0] = event.GetId();
    statusBarReg[1] = event.GetCol();
    updateStatusBar();
}

//---------------------------------------------------------------------------
// Various helper functions.
//---------------------------------------------------------------------------
void
VUFrame::DrawParam(VUParam &p, char *a, uint32 hex) {
    switch (p.type) {
        case 1: //VInn
            sprintf(a,"VI%02d", p.index);
            break;
        case 2: //VFnn
            sprintf(a,"VF%02d", p.index);
            break;
        case 3: //VIdest
            sprintf(a, "VI%02d%s", p.index, strlwr(p.sufix));
            break;
        case 4: //VFdest
            sprintf(a, "VF%02d%s", p.index, strlwr(p.sufix));
            break;
        case 5: //ACCdest
            sprintf(a, "ACC%s", strlwr(p.sufix));
            break;
        case 6: //Imm24
            sprintf(a, "0x%06X", (uint32)p.udata);
            break;
        case 7: //Imm15
            sprintf(a, "%u", (uint32)p.udata);
            break;
        case 8: //Imm12
            sprintf(a, "%u", (uint32)p.udata);
            break;
        case 9: //Imm11
            if(p.label[0])
                sprintf(a, "%s", p.label);
            else
                sprintf(a, "%ld", p.data);
            break;
        case 10: //Imm5
            sprintf(a, "%ld", p.data);
            break;
        case 11: //I
            strcpy(a, "I");
            break;
        case 12: //Imm11(VI)
            if(p.label[0]) {
                sprintf(a, "%s(VI%02d)", p.label, p.index);
            } else {
                if ( hex == 0 ) {
                    sprintf(a, "0x%4X(VI%02d)", (uint32)p.data, p.index);
                } else {
                    sprintf(a, "%4d(VI%02d)", (uint32)p.data, p.index);
                }
            }
            break;
        case 13: //Imm11(VI)dest
            if(p.label[0]) {
                sprintf(a,"%s(VI%02d)%s", p.label, p.index, strlwr(p.sufix));
            } else {
                if ( hex == 0 ) {
                    sprintf(a,"0x%4X(VI%02d)%s", (uint32)p.data, p.index, strlwr(p.sufix));
                } else {
                    sprintf(a,"%4d(VI%02d)%s", (uint32)p.data, p.index, strlwr(p.sufix));
                }
            }
            break;
        case 14: //(VI)dest
            sprintf(a,"(VI%02d)%s", p.index, strlwr(p.sufix));
            break;
        case 15: //(--VI)
            sprintf(a,"(--VI%02d)", p.index);
            break;
        case 16: //(VI++)
            sprintf(a,"(VI%02d++)",p.index);
            break;
        case 17: //P
            strcpy(a,"P");
            break;
        case 18: //Q
            strcpy(a,"Q");
            break;
        case 19: //R
            strcpy(a,"R");
            break;
        case 20: //VI01
            strcpy(a,"VI01");
            break;
        case 21: //Imm32
            sprintf(a, "%f", p.fdata);
            break;
    }
}

void VUFrame::DrawProgram() {
    int j,k,m,l=0;
    uint32 i;
    char scode[100],param[50], auxi[50], aux2[10];
    char *flavors[]={"","i","x","y","z","w","q","A","Ai","Aq","Ax","Ay","Az","Aw"};

    Previous = 0;
    gridCode->SetCellBackgroundColour(0, 3, cCurCode);
    gridCode->SetCellBackgroundColour(0, 4, cCurCode);
    for(i = 0; i < MAX_VUCODE_SIZE; i++) {
        gridCode->SetCellValue(i, 1, wxString("."));
        gridCode->SetCellValue(i, 2, wxString(""));
        gridCode->SetCellValue(i, 3, wxString(""));
        gridCode->SetCellValue(i, 4, wxString(""));
    }
    for (i = 0; i < MAX_VUCODE_SIZE; i++,l++){
        if(VUchip.program[i].SymbolIndex != -1) {
            strcpy(scode,VUchip.Labels[VUchip.program[i].SymbolIndex].symb);
            strcat(scode,":");
            gridCode->SetCellValue(l++, 1, scode);
        }
        VUchip.program[i].addr = i;
        gridCode->SetCellValue(l, 2, wxString::Format("%d",
                VUchip.program[i].tics));
        for(m=UPPER; m<=LOWER; m++) {
            strcpy(auxi,Instr.Instr[VUchip.program[i].InstIndex[m]].nemot);
            if(VUchip.program[i].flg && !m) {
                sprintf(aux2,"[%c]",VUchip.program[i].flg);
                strcat(auxi, aux2);
            }
            k = 0;
            while(auxi[k]!='.' && auxi[k]) {
                k++;
            }
            auxi[k] = 0;
            //only code exception
            if (!strcmp(auxi, "CLIPW")) {
                strcpy(auxi,"CLIP");
            }
            strcpy(scode,auxi);
            if(VUchip.program [i].flavor[m])
                strcat(scode,flavors[VUchip.program [i].flavor[m]]);
            if(VUchip.program [i].dest[m][0]) {
                strcat(scode, ".");
                strcpy(auxi, VUchip.program [i].dest[m]);
                strcat(scode, strlwr(auxi));
            }
            for (j=0; j<Instr.Instr[VUchip.program [i].InstIndex[m]].operands; j++) {
                DrawParam(VUchip.program [i].Params[m][j],param, codeAdressStyle);
                strcat(scode," ");
                strcat(scode,param);
                if(j<Instr.Instr[VUchip.program [i].InstIndex[m]].operands-1)
                    strcat(scode,",");
            }
            gridCode->SetCellValue(l, 3+m, scode);
        }
    }
}

int VUFrame::LineInstruction(int a) {
    int i,l=0;
    for(i=0; i<=a; i++, l++) {
        if(VUchip.program[i].SymbolIndex!=-1) {
            l++;
        }
    }
    return (l);
}

void
VUFrame::DrawMemory() {
    uint32 i;
    char val[4][50];
    float stuff;
    cout << "DrawMemory" << endl;
    for (i = 0; i < MAX_VUDATA_SIZE; i++) {
        switch(regRadioBox->GetSelection()) {
            case 0:
                sprintf(val[0],"%ld", VUchip.dataMem[i].w);
                sprintf(val[1],"%ld", VUchip.dataMem[i].z);
                sprintf(val[2],"%ld", VUchip.dataMem[i].y);
                sprintf(val[3],"%ld", VUchip.dataMem[i].x);
                break;
            case 1:
                sprintf(val[0],"%.4f", VUchip.dataMem[i].w/16.0f);
                sprintf(val[1],"%.4f", VUchip.dataMem[i].z/16.0f);
                sprintf(val[2],"%.4f", VUchip.dataMem[i].y/16.0f);
                sprintf(val[3],"%.4f", VUchip.dataMem[i].x/16.0f);
                break;
            case 2:
                sprintf(val[0],"%.12f",VUchip.dataMem[i].w/4096.0f);
                sprintf(val[1],"%.12f",VUchip.dataMem[i].z/4096.0f);
                sprintf(val[2],"%.12f",VUchip.dataMem[i].y/4096.0f);
                sprintf(val[3],"%.12f",VUchip.dataMem[i].x/4096.0f);
            case 3:
                sprintf(val[0],"%.15f",VUchip.dataMem[i].w/32768.0f);
                sprintf(val[1],"%.15f",VUchip.dataMem[i].z/32768.0f);
                sprintf(val[2],"%.15f",VUchip.dataMem[i].y/32768.0f);
                sprintf(val[3],"%.15f",VUchip.dataMem[i].x/32768.0f);
            case 4:
                memcpy(&stuff,&(VUchip.dataMem[i].w),4);
                sprintf(val[0],"%f",stuff);
                memcpy(&stuff,&(VUchip.dataMem[i].z),4);
                sprintf(val[1],"%f",stuff);
                memcpy(&stuff,&(VUchip.dataMem[i].y),4);
                sprintf(val[2],"%f",stuff);
                memcpy(&stuff,&(VUchip.dataMem[i].x),4);
                sprintf(val[3],"%f",stuff);
                break;
        }

        gridMemory->SetCellValue(i, 3, val[0]);
        gridMemory->SetCellValue(i, 2, val[1]);
        gridMemory->SetCellValue(i, 1, val[2]);
        gridMemory->SetCellValue(i, 0, val[3]);

        if ( tagShow == 0 ) {
            if ( VUchip.dataMem[i].gif ) {
                gridMemory->SetCellBackgroundColour(i, 0, cGIFtag);
                gridMemory->SetCellBackgroundColour(i, 1, cGIFtag);
                gridMemory->SetCellBackgroundColour(i, 2, cGIFtag);
                gridMemory->SetCellBackgroundColour(i, 3, cGIFtag);
            } else {
                gridMemory->SetCellBackgroundColour(i, 0, *wxWHITE);
                gridMemory->SetCellBackgroundColour(i, 1, *wxWHITE);
                gridMemory->SetCellBackgroundColour(i, 2, *wxWHITE);
                gridMemory->SetCellBackgroundColour(i, 3, *wxWHITE);
            }
        }
    }
	VUchip.memoryUpdate = false;
}
void VUFrame::FastRegisterUpdate() {
    int i;
    VFReg *Reg;

    for(i = 0; i < 16; i++) {
        gridIntRegisters->SetCellValue(0, i, wxString::Format("%d",
                VUchip.RegInt[i].value()));
    }

    for(i = 0; i < 32; i++) {
        Reg = &VUchip.RegFloat[i];
        gridFloatRegisters->SetCellValue(0, i,
            wxString::Format("%f", Reg->x()));
        gridFloatRegisters->SetCellValue(1, i,
            wxString::Format("%f", Reg->y()));
        gridFloatRegisters->SetCellValue(2, i,
            wxString::Format("%f", Reg->z()));
        gridFloatRegisters->SetCellValue(3, i,
            wxString::Format("%f", Reg->w()));
    }
    
    gridSpecialRegisters->SetCellValue(0, 0,
        wxString::Format("%f", VUchip.ACC.x()));
    gridSpecialRegisters->SetCellValue(1, 0,
        wxString::Format("%f", VUchip.ACC.y()));
    gridSpecialRegisters->SetCellValue(2, 0,
        wxString::Format("%f", VUchip.ACC.z()));
    gridSpecialRegisters->SetCellValue(3, 0,
        wxString::Format("%f", VUchip.ACC.w()));

    gridSpecialRegisters->SetCellValue(0, 1,
        wxString::Format("%f", VUchip.Q.x()));
    gridSpecialRegisters->SetCellValue(1, 1,
        wxString::Format("%f", VUchip.Q.y()));
    gridSpecialRegisters->SetCellValue(2, 1,
        wxString::Format("%f", VUchip.Q.z()));
    gridSpecialRegisters->SetCellValue(3, 1,
        wxString::Format("%f", VUchip.Q.w()));

    gridSpecialRegisters->SetCellValue(0, 2,
        wxString::Format("%f", VUchip.P.x()));
    gridSpecialRegisters->SetCellValue(1, 2,
        wxString::Format("%f", VUchip.P.y()));
    gridSpecialRegisters->SetCellValue(2, 2,
        wxString::Format("%f", VUchip.P.z()));
    gridSpecialRegisters->SetCellValue(3, 2,
        wxString::Format("%f", VUchip.P.w()));

    gridSpecialRegisters->SetCellValue(0, 3,
        wxString::Format("%f", VUchip.R.x()));
    gridSpecialRegisters->SetCellValue(1, 3,
        wxString::Format("%f", VUchip.R.y()));
    gridSpecialRegisters->SetCellValue(2, 3,
        wxString::Format("%f", VUchip.R.z()));
    gridSpecialRegisters->SetCellValue(3, 3,
        wxString::Format("%f", VUchip.R.w()));

    gridSpecialRegisters->SetCellValue(0, 4,
        wxString::Format("%f", VUchip.I.x()));
    gridSpecialRegisters->SetCellValue(1, 4,
        wxString::Format("%f", VUchip.I.y()));
    gridSpecialRegisters->SetCellValue(2, 4,
        wxString::Format("%f", VUchip.I.z()));
    gridSpecialRegisters->SetCellValue(3, 4,
        wxString::Format("%f", VUchip.I.w()));

    // Latest register in statusbar
    updateStatusBar();
}

//---------------------------------------------------------------------------
void
VUFrame::wrapper_DebugTic(void *objPtr, int p1, int p2) {
    VUFrame *self = (VUFrame*)objPtr;
    self->DebugTic(p1, p2);
}

//---------------------------------------------------------------------------
void
VUFrame::wrapper_XGKICK(void *objPtr, int offset) {
    VUFrame *self = (VUFrame*)objPtr;
    self->drawGIF((uint32)offset);
}

void
VUFrame::wrapper_DebugWarning(void *objPtr, wxString message) {
    VUFrame *self = (VUFrame*)objPtr;
    self->txtDebugFailed(message);
}

void
VUFrame::drawGIF(uint32 offset) {
    uint32 size = (MAX_VUDATA_SIZE-offset)*16;
    uint32 *data = (uint32 *)malloc(size);
    uint32 i, j;

    j = 0;
    for(i = offset; i < MAX_VUDATA_SIZE; i++) {
        memcpy(&data[j*4+0], &VUchip.dataMem[i].x, 4);
        memcpy(&data[j*4+1], &VUchip.dataMem[i].y, 4);
        memcpy(&data[j*4+2], &VUchip.dataMem[i].z, 4);
        memcpy(&data[j*4+3], &VUchip.dataMem[i].w, 4);
        j++;
    }

    if (autoGSExec == 0) {
        if (sendPrim == 0) {
            // gsExec(gsTmpFile.c_str(), prim, size);
        }
        dumpDisplayList((char *)gsTmpFile.c_str(), data, size);
    }

    GIF *gif = new GIF(data, size);
    gif->xoffset = xoffset;
    gif->yoffset = yoffset;
    if ( gif->unpack() ) {
        vector<string> v = gif->TagAsString();
        vector<string>::const_iterator it = v.begin();
        i = 0;
        while(it != v.end()) {
            gridGIF->SetCellValue(i, 0, wxString(((string)*it).c_str()));
            gridGIF->SetCellValue(i, 1, wxString(((string)*(it+1)).c_str()));
            gridGIF->SetCellBackgroundColour(i, 0, cGIFtag);
            gridGIF->SetCellBackgroundColour(i, 1, cGIFtag);
            it = it + 2;
            i++;
        }
        i++;
        gridGIF->SetCellBackgroundColour(i, 0, cNloop1);
        gridGIF->SetCellBackgroundColour(i, 1, cNloop1);
        for(j = 0;j < gif->getNloop(); j++ ) {
            v = gif->NloopData();
            it = v.begin();
            while(it != v.end()) {
                if (i > MAX_VUDATA_SIZE ) {
                    break;
                }
                gridGIF->SetCellValue(i, 0, wxString(((string)*it).c_str()));
                gridGIF->SetCellValue(i, 1, wxString(((string)*(it+1)).c_str()));
                if(j&0x1) {
                    gridGIF->SetCellBackgroundColour(i, 0, cNloop1);
                    gridGIF->SetCellBackgroundColour(i, 1, cNloop1);
                } else {
                    gridGIF->SetCellBackgroundColour(i, 0, cNloop2);
                    gridGIF->SetCellBackgroundColour(i, 1, cNloop2);
                }
                it = it + 2;
                i++;
            }
            if (i > MAX_VUDATA_SIZE ) {
                break;
            }
        }
    } else {
        wxMessageBox(
            wxString::Format("Invalid GIF Tag at offset: %d", offset),
            "", wxOK|wxICON_INFORMATION, this);
    }

    free(data);
    delete gif;
}

void
VUFrame::DebugTic(int mode, int error) {
    int i;
    char buffer[200], *modedef[]={"UPPER","LOWER"};
    char *succes[]= {"success",
        "STALL on param 1!!",
        "STALL on param 2!!",
        "STALL on param 3!!",
        "WARNING P used before fixed (use WAITP)",
        "WARNING Q used before fixed (use WAITQ)",
        "ENTER delay slot...",
        "EXIT delay slot. Instruction Finished!!"};

    if(error==999) {
        wxMessageBox("End of Program", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if(mode==2) {
        i = LineInstruction(VUchip.PC-1);
        sprintf(buffer, "Instruction line %d [%s][%s] completed\n",VUchip.PC,
            gridCode->GetCellValue(i, 3).c_str(),
            gridCode->GetCellValue(i, 4).c_str());
        txtDebug->AppendText(buffer);
    } else {
        i = LineInstruction(VUchip.PC);
        sprintf(buffer, "Executing instruction line %d %s [%s]\n", VUchip.PC,
            modedef[mode], gridCode->GetCellValue(i, 3+mode).c_str());
        txtDebug->AppendText(buffer);
        sprintf(buffer,"Result: %s\n",succes[error]);
        txtDebug->AppendText(buffer);
    }
}

//---------------------------------------------------------------------------
// Update flag panel when receiving active event
void
VUFrame::FlagsUpdate() {
    char data[100];
    sprintf (data,"%06ld",iToBin(VUchip.ClipFlag[3]));
    clip3->SetValue(data);
    sprintf (data,"%06ld",iToBin(VUchip.ClipFlag[2]));
    clip2->SetValue(data);
    sprintf (data,"%06ld",iToBin(VUchip.ClipFlag[1]));
    clip1->SetValue(data);
    sprintf (data,"%06ld",iToBin(VUchip.ClipFlag[0]));
    clip0->SetValue(data);

    sprintf (data,"%04ld",iToBin(VUchip.MacZ));
    MZf->SetValue(data);
    sprintf (data,"%04ld",iToBin(VUchip.MacS));
    MSf->SetValue(data);
    sprintf (data,"%04ld",iToBin(VUchip.MacU));
    MUf->SetValue(data);
    sprintf (data,"%04ld",iToBin(VUchip.MacO));
    MOf->SetValue(data);

    fz->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 1)));
    fs->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 2)/2));
    fu->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 4)/4));
    fo->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 8)/8));
    fi->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 16)/16));
    fd->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 32)/32));
    fzs->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 64)/64));
    fss->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 128)/128));
    fus->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 256)/256));
    fos->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 512)/512));
    fis->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 1024)/1024));
    fds->SetValue(wxString::Format("%d\n", (VUchip.StatusFlag & 2048)/2048));
}

//---------------------------------------------------------------------------
// OnInstructionStatus
void VUFrame::InstuctionStatus() {
    char data[100];
    int i,j;

    if(Status!=READY) {
        return;
    }

    i = LineInstruction(VUchip.PC);
    txtStatus->Clear();
    txtStatus->AppendText("------------------------\n");
    txtStatus->AppendText("    Instrucion UPPER    \n");
    txtStatus->AppendText("------------------------\n");
    sprintf(data," [%s] ", gridCode->GetCellValue(i, 3).c_str());
    txtStatus->AppendText(data);
    txtStatus->AppendText("Can be executed...\n");
    j=Instr.Instr[VUchip.program[VUchip.PC].InstIndex[0]].lastthr[VUchip.program[VUchip.PC].flavor[0]];
    if(j>1) {
        sprintf(data,"After %d cycles",j-1);
    } else {
        sprintf(data,"Inmediatly");
    }
    txtStatus->AppendText(data);
    txtStatus->AppendText("\n");
    txtStatus->AppendText("------------------------\n");
    txtStatus->AppendText("    Instrucion LOWER    \n");
    txtStatus->AppendText("------------------------\n");
    sprintf(data," [%s] ", gridCode->GetCellValue(i, 4).c_str());
    txtStatus->AppendText(data);
    txtStatus->AppendText("Can be executed...");
    j=Instr.Instr[VUchip.program[VUchip.PC].InstIndex[1]].lastthr[VUchip.program[VUchip.PC].flavor[1]];
    if(j>1) {
        sprintf(data,"After %d cycles\n",j-1);
    } else {
        sprintf(data,"Inmediately\n");
    }
    txtStatus->AppendText(data);
    txtStatus->AppendText("\n");
    txtStatus->AppendText("NOTE: Information is regarding instruction throughput.\n");
    txtStatus->AppendText("Other issues such Register STALL may occur.\n");
}

void
VUFrame::buildToolbar(void) {
    // toolbar = CreateToolBar(wxTB_TEXT|wxTB_FLAT);
    menuBar = new wxMenuBar;
    menuFile = new wxMenu;
    menuTools = new wxMenu;
    menuOptions = new wxMenu;
    menuHelp = new wxMenu;
    menuRemote = new wxMenu;

    menuFile->Append(ID_FILE_LOADCODE,  "&Load code\tCtrl-l");
    menuFile->Append(ID_FILE_LOADMEM,   "&Load mem\tCtrl-m");
    menuFile->Append(ID_FILE_LOADPROJECT,   "&Load project\tCtrl-p");
    menuFile->Append(ID_FILE_LOADVIF,   "&Load VIF data");
    menuFile->Append(ID_FILE_LOADDMA,   "&Load DMA data");
    menuFile->Append(ID_FILE_SAVESTATE,  "&Save state");
    menuFile->Append(ID_FILE_QUIT,      "&Exit\tCtrl-q");

    menuTools->Append(ID_TOOL_RESET,    "Reset\tF1");
    menuTools->Append(ID_TOOL_RESTART,  "Restart\tF2");
    menuTools->Append(ID_TOOL_STEP,     "Step\tF3");
    menuTools->Append(ID_TOOL_RUN,      "Run\tF4");
    // menuTools->Append(ID_TOOL_CLEARMEM,      "Clear Memory");
    // menuTools->Append(ID_TOOL_CLEARCODE,     "Clear Code");

    menuOptions->Append(ID_OPTION_SETTINGS, "Preferences\tCtrl-p");
    // menuOptions->Append(ID_OPTION_CLEARMEM,      "Always clear memory on reset");
    // menuOptions->Append(ID_OPTION_CLEARCODE,     "Always clear registers on reset");
    // menuOptions->Append(ID_OPTION_HIDE_PANEL3, "Hide right panel");
    // menuOptions->Append(ID_OPTION_HIDE_PANEL2, "Hide bottom panel");
    
    menuRemote->Append(ID_REMOTE_VU0,       "Get VPU0 content\tF7");
    menuRemote->Append(ID_REMOTE_VU1,       "Get VPU1 content\tF8");
    menuRemote->Append(ID_REMOTE_REGSVU0,   "Get VPU0 Registers\tF9");
    menuRemote->Append(ID_REMOTE_REGSVU1,   "Get VPU1 Registers\tF10");
    menuRemote->Append(ID_REMOTE_REGS,      "Get misc registers\tF11");
    menuRemote->Append(ID_REMOTE_GSINIT,    "Init GS on PS2\tF12");
    menuRemote->Append(ID_REMOTE_VU0ALL,    "Get VPU0 Code, Memory and Registers\tCtrl-0");
    menuRemote->Append(ID_REMOTE_VU1ALL,    "Get VPU1 Code, Memory and Registers\tCtrl-1");
    menuRemote->Append(ID_REMOTE_CLR,       "Clear screen");

    menuBar->Append(menuFile,   "&File");
    menuBar->Append(menuTools,  "&Tools");
    menuBar->Append(menuRemote, "&Remote");
    menuBar->Append(menuOptions,"&Options");
    SetMenuBar(menuBar);
}

VUFrame::VUFrame(const wxString &title, const wxPoint &pos, const wxSize
    &size) : wxFrame((wxFrame*)NULL, -1, title, pos, size) {

#ifdef WIN32
#undef GetCharWidth
#endif
    charWidth = wxWindow::GetCharWidth();

    gridCode = NULL;

    SetSettings();
    // Toolbar
    buildToolbar();

    // Splitters
	wxSplitterWindow    *hzsplit = new wxSplitterWindow(this, -1, wxPoint(0, 0),
        wxDefaultSize, wxSP_3D);;
    wxSplitterWindow    *vertsplit = new wxSplitterWindow(hzsplit, -1,
        wxPoint(0, 0), wxDefaultSize, wxSP_3D);

    // Left notebook
    left_book = new wxNotebook(vertsplit, ID_NOTEBOOK1, wxDefaultPosition,
        wxDefaultSize); 
    buildCodeTable(left_book);
    buildGIFTable(left_book);
    buildMiscRegistersTable(left_book);
    left_book->AddPage(gridCode, "Code", TRUE, -1);
    left_book->AddPage(buildMemoryTable(left_book), "Memory", FALSE, -1);
    left_book->AddPage(gridGIF, "GIF output", FALSE, -1);
    left_book->AddPage(panelMiscRegisters, "Misc. Registers", FALSE, -1);
    left_book->Show(TRUE);

    // Right notebook
    right_book = new wxNotebook(vertsplit, -1, wxDefaultPosition,
        wxDefaultSize);
    regDetail = new wxPanel(right_book, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER);

    txtDebug = new wxTextCtrl(right_book, ID_TEXT_DEBUG, wxString(""),
        wxDefaultPosition, wxSize(400, 400), wxTE_MULTILINE|wxTE_READONLY);
    right_book->AddPage(txtDebug, "Debug", FALSE, -1);
    right_book->AddPage(regDetail, "Reg Detail", TRUE, -1);
    wxString choices[5] = {"Fixed 0", "Fixed 4", "Fixed 12", "Fixed 15", "Float"};
    m_regIntNum = new wxRadioBox(regDetail, ID_REGINT_REP, "Integer register representation",
            wxDefaultPosition, wxDefaultSize, 5, choices, 3, wxRA_SPECIFY_ROWS);
    m_regIntNum->SetSelection(0);
    m_regFloatNum = new wxRadioBox(regDetail, ID_REGFLOAT_REP, "Float register representation",
            wxDefaultPosition, wxDefaultSize, 5, choices, 3, wxRA_SPECIFY_ROWS);
    m_regFloatNum->SetSelection(4);
    m_regSpecNum = new wxRadioBox(regDetail, ID_REGSPEC_REP, "Special register representation",
            wxDefaultPosition, wxDefaultSize, 5, choices, 3, wxRA_SPECIFY_ROWS);
    m_regSpecNum->SetSelection(4);
    wxBoxSizer *regNumRep = new wxBoxSizer (wxVERTICAL);
    regNumRep->Add(m_regIntNum, 0, wxALIGN_CENTER_VERTICAL);
    regNumRep->Add(m_regFloatNum, 0, wxALIGN_CENTER_VERTICAL);
    regNumRep->Add(m_regSpecNum, 0, wxALIGN_CENTER_VERTICAL);
	regDetail->SetSizerAndFit(regNumRep);
	regDetail->SetAutoLayout(TRUE);
	regDetail->Layout();
 
    txtStatus = new wxTextCtrl(right_book, ID_TEXT_STATUS, wxString(""),
        wxDefaultPosition, wxSize(400, 400), wxTE_MULTILINE|wxTE_READONLY);
    right_book->AddPage(txtStatus, "Instruction Status", FALSE, -1);
    buildFlagsPanel(right_book);
    right_book->AddPage(flagsDetail, "Flags", FALSE, -1);

    vertsplit->SetMinimumPaneSize(100);
    vertsplit->SplitVertically(left_book, right_book, 550);

    // bottom tabs
    down_book = new wxNotebook(
        hzsplit, -1, wxDefaultPosition, wxDefaultSize);
    buildRegisterGrid(down_book);
    down_book->AddPage(gridIntRegisters, "Integer registers", TRUE, -1);
    down_book->AddPage(gridFloatRegisters, "Float registers", FALSE, -1);
    down_book->AddPage(gridSpecialRegisters, "Special registers", FALSE, -1);
    hzsplit->SetMinimumPaneSize(100);
    hzsplit->SplitHorizontally(vertsplit, down_book, 380);

	static const int widths[] = {-1, 6*charWidth, 6*charWidth, 6*charWidth,
        6*charWidth };
	CreateStatusBar(WXSIZEOF(widths), wxST_SIZEGRIP, ID_STATUSBAR);
    statusBar = GetStatusBar();


    // ok gui is up, lets load the mnemonics
	if (mnemonicFile.GetFullPath() == "") {	
		mnemonicFile.Assign("instructions.txt");
	}
	if (!LoadInstructions((char *)mnemonicFile.GetFullPath().c_str())) {
		wxMessageBox("Failed to load instructions", "",
			wxOK|wxICON_INFORMATION, this);
		dlgOpenFile = new wxFileDialog(this, "Open instructions.txt file");
		if (dlgOpenFile->ShowModal() == wxID_OK &&
			dlgOpenFile->GetFilename() != "") {
			mnemonicFile.Assign(dlgOpenFile->GetPath());
			if(!LoadInstructions((char *)mnemonicFile.GetFullPath().c_str())) {
				wxMessageBox("Failed to load instructions, Exiting", "",
					wxOK|wxICON_INFORMATION, this);
				Close(TRUE);
			}
		}
	}

    VUchip.Reset();
    InstFill();
	FastRegisterUpdate();
    if ( autoLoadLast == 0 ) {
        if ( dataFile.GetFullPath() != "" ) {
            Status = READY;
            LoadMemory(dataFile);
        }
        if ( codeFile.GetFullPath() != "" ) {
            Status = READY;
            LoadCode((char *)codeFile.GetFullPath().c_str());
            DrawProgram();
        }
    } else {
        codeFile.Assign("");
        dataFile.Assign("");
    }
}

bool VUemu::OnInit() {
    VUFrame *frame = new VUFrame("VU Emu", wxPoint(50, 50), wxSize(800, 600));
    frame->Show(TRUE);
    SetTopWindow(frame);
    return TRUE;
}

uint32
validateRegsField(uint64 regs) {
    int counter = 0;
    for(int i = 0; i < 16; i++) {
        if (((regs)&0xf) != 0xf) {
            counter++;
        } else {
            break;
        }
        regs = regs >> 4;
    }
    return counter;
}

uint32
getFLGField(uint64 data) {
    return((data>>58)&0x3);
}

uint32
getPRIM(uint64 data) {
    return((data>>47)&0x3ff);
}

uint64
getPRE(uint64 data) {
    return((data>>46)&0x1);
}

uint32
getEOP(uint64 data) {
    return((data>>15)&0x1);
}

uint32
getNLOOP(uint64 data) {
   return (data&0x7FFF);
}

uint32
getNregs(uint64 data) {
    return ((data>>60));
}
