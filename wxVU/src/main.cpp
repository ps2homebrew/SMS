#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>

#include "main.h"
#include "prefdef.h"
#include "Remote.h"

#include "vif.h"
#include "vif0.h"
#include "vif1.h"
#include "vu.h"
#include "sif.h"
#include "timer.h"
#include "gif.h"
#include "intc.h"
#include "ipu.h"
#include "fifo.h"
#include "gs.h"
#include "vuBreakDialog.h"
#include "breakpoint.h"

#include "MiscRegisterPanel.h"

using namespace std;

//---------------------------------------------------------------------------
extern MicroCode Instr;

static int running = 0;
static int accumulatedTicks = 0;

BEGIN_EVENT_TABLE(VUFrame, wxFrame)
    EVT_MENU(ID_FILE_LOADCODE, VUFrame::OnLoadCode)
    EVT_MENU(ID_FILE_LOADMEM, VUFrame::OnLoadMem)
    EVT_MENU(ID_FILE_LOADPROJECT, VUFrame::OnLoadProject)
    EVT_MENU(ID_FILE_LOADVIF, VUFrame::OnLoadVIF)
    EVT_MENU(ID_FILE_LOADDMA, VUFrame::OnLoadDMA)
    EVT_MENU(ID_FILE_SAVECODE, VUFrame::OnSaveCode)
    EVT_MENU(ID_FILE_SAVESTATE, VUFrame::OnSaveState)
    EVT_MENU(ID_TOOL_RESET, VUFrame::OnReset)
    EVT_MENU(ID_TOOL_RESTART, VUFrame::OnRestart)
    EVT_MENU(ID_TOOL_STEP, VUFrame::OnStep)
    EVT_MENU(ID_TOOL_RUN, VUFrame::OnRun)
    EVT_MENU(ID_OPTION_SETTINGS, VUFrame::OnSettings)
    EVT_MENU(ID_HELP, VUFrame::OnHelp)
    EVT_MENU(ID_FILE_QUIT, VUFrame::OnQuit)
    EVT_MENU(ID_REMOTE_VU0, VUFrame::OnVu0)
    EVT_MENU(ID_REMOTE_VU1, VUFrame::OnVu1)
    EVT_MENU(ID_REMOTE_VU0ALL, VUFrame::OnVu0All)
    EVT_MENU(ID_REMOTE_VU1ALL, VUFrame::OnVu1All)
    EVT_MENU(ID_REMOTE_REGS, VUFrame::OnGetMiscRegs)
    EVT_MENU(ID_REMOTE_REGSVU0, VUFrame::OnRegsVu0)
    EVT_MENU(ID_REMOTE_REGSVU1, VUFrame::OnRegsVu1)
    EVT_MENU(ID_REMOTE_GSINIT, VUFrame::OnGsInit)
    EVT_MENU(ID_REMOTE_CLR, VUFrame::OnGsClear)
    EVT_MENU(ID_OPTION_TRACE_DMA, VUFrame::OnOptionTraceDma)
    EVT_MENU(ID_OPTION_TRACE_VIF, VUFrame::OnOptionTraceVif)
    EVT_MENU(ID_OPTION_TRACE_GIF, VUFrame::OnOptionTraceGif)
    EVT_MENU(ID_OPTION_TRACE_VU, VUFrame::OnOptionTraceVu)
    EVT_GRID_CELL_LEFT_DCLICK(VUFrame::OnBreakpoint)
END_EVENT_TABLE()

IMPLEMENT_APP(VUemu)

VUFrame* VUemu::ms_pFrame = NULL;

//---------------------------------------------------------------------------
// GUI builder functions
//---------------------------------------------------------------------------

void
VUFrame::buildCodeTable(wxNotebook *nbook) {
    uint32 i;
    m_pGridCode = new wxGrid(nbook, ID_GRIDCODE, wxDefaultPosition);
    m_pGridCode->CreateGrid(MAX_VUCODE_SIZE, 5);
    m_pGridCode->EnableEditing(FALSE);
    m_pGridCode->SetColLabelValue(0, "P");
    m_pGridCode->SetColLabelValue(1, "Label");
    m_pGridCode->SetColLabelValue(2, "T");
    m_pGridCode->SetColLabelValue(3, "Upper");
    m_pGridCode->SetColLabelValue(4, "Lower");
    m_pGridCode->SetColLabelSize(int(GetCharHeight()*1.5));
    m_pGridCode->SetRowLabelSize(m_charWidth*5);
    m_pGridCode->SetColSize(0, m_charWidth*2);
    m_pGridCode->SetColSize(1, m_charWidth*7);
    m_pGridCode->SetColSize(2, m_charWidth*2);
    m_pGridCode->SetColSize(2, m_charWidth*2);
    m_pGridCode->SetColSize(3, m_charWidth*20);
    m_pGridCode->SetColSize(4, m_charWidth*20);
    m_pGridCode->SetLabelFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
    for (i = 0; i < MAX_VUCODE_SIZE; i++) {
        m_pGridCode->SetRowLabelValue(i, wxString::Format("%04d", i));
        m_pGridCode->SetRowSize(i, int(GetCharHeight()*1.5));
    }
    m_pGridCode->DisableDragGridSize();
    m_pGridCode->SetGridLineColour(*wxBLACK);
}

void
VUFrame::buildFlagsPanel(wxNotebook *book) {
    m_pFlagsDetail = new wxPanel(book, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER);

    wxBoxSizer  *topSizer = new wxBoxSizer(wxVERTICAL);
    wxGridSizer *clipflags = new wxGridSizer(2, 4, 1, 1);
    wxGridSizer *statusflags = new wxGridSizer(2, 12, 1, 1);
    wxGridSizer *macflags = new wxGridSizer(2, 4, 1, 1);
    wxSize size;

    // clip flags
    size = wxSize(7*m_charWidth, (int)(1.5*GetCharHeight()));
    clipflags->Add(new wxStaticText(m_pFlagsDetail, -1, "3rd prv", wxDefaultPosition,
            size), 0, wxADJUST_MINSIZE, 1);
    clipflags->Add(new wxStaticText(m_pFlagsDetail, -1, "2nd prv", wxDefaultPosition,
            size), 0, wxADJUST_MINSIZE, 1);
    clipflags->Add(new wxStaticText(m_pFlagsDetail, -1, "previous", wxDefaultPosition,
            size), 0, wxADJUST_MINSIZE, 1);
    clipflags->Add(new wxStaticText(m_pFlagsDetail, -1, "current", wxDefaultPosition,
            size), 0, wxADJUST_MINSIZE, 1);

    clip3 = new wxTextCtrl(m_pFlagsDetail, -1, "000000", wxDefaultPosition,
        size, wxTE_READONLY|wxTE_RICH);
    clip2 = new wxTextCtrl(m_pFlagsDetail, -1, "000000", wxDefaultPosition,
        size, wxTE_READONLY|wxTE_RICH);
    clip1 = new wxTextCtrl(m_pFlagsDetail, -1, "000000", wxDefaultPosition,
        size, wxTE_READONLY|wxTE_RICH);
    clip0 = new wxTextCtrl(m_pFlagsDetail, -1, "000000", wxDefaultPosition,
        size, wxTE_READONLY|wxTE_RICH);

    clipflags->Add(clip3, 0, wxLEFT, 1);
    clipflags->Add(clip2, 0, wxLEFT, 1);
    clipflags->Add(clip1, 0, wxLEFT, 1);
    clipflags->Add(clip0, 0, wxLEFT, 1);
    
    // statusflags
    size = wxSize(3*m_charWidth, (int)(1.5*GetCharHeight()));
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "DS", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "IS", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "OS", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "US", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "SS", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "ZS", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "D", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "I", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "O", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "U", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "S", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);
    statusflags->Add(new wxStaticText(m_pFlagsDetail, -1, "Z", wxDefaultPosition, size), 0, wxADJUST_MINSIZE, 1);

    fds = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fis = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fos = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fzs = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fss = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fus = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fi = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fo = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fs = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fz = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fu = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);
    fd = new wxTextCtrl(m_pFlagsDetail, -1, "0", wxDefaultPosition, size, wxTE_READONLY);

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
    size = wxSize(5*m_charWidth, (int)(1.5*GetCharHeight()));
    macflags->Add(new wxStaticText(m_pFlagsDetail, -1, "O xyzw"), 0,
        wxADJUST_MINSIZE, 1);
    macflags->Add(new wxStaticText(m_pFlagsDetail, -1, "U xyzw"), 0,
        wxADJUST_MINSIZE, 1);
    macflags->Add(new wxStaticText(m_pFlagsDetail, -1, "S xyzw"), 0,
        wxADJUST_MINSIZE, 1);
    macflags->Add(new wxStaticText(m_pFlagsDetail, -1, "Z xyzw"), 0,
        wxADJUST_MINSIZE, 1);

    MOf = new wxTextCtrl(m_pFlagsDetail, -1, "0000", wxDefaultPosition, size,
        wxTE_READONLY);
    MUf = new wxTextCtrl(m_pFlagsDetail, -1, "0000", wxDefaultPosition, size,
        wxTE_READONLY);
    MSf = new wxTextCtrl(m_pFlagsDetail, -1, "0000", wxDefaultPosition, size,
        wxTE_READONLY);
    MZf = new wxTextCtrl(m_pFlagsDetail, -1, "0000", wxDefaultPosition, size,
        wxTE_READONLY);

    macflags->Add(MOf, 0, wxLEFT, 1);
    macflags->Add(MUf, 0, wxLEFT, 1);
    macflags->Add(MSf, 0, wxLEFT, 1);
    macflags->Add(MZf, 0, wxLEFT, 1);

    // show
    topSizer->Add(new wxStaticText(m_pFlagsDetail, -1, "CLIP -z+z-y+y-x+x ( 4 times )"),
            0, wxADJUST_MINSIZE, 1);
    topSizer->Add(clipflags);
    topSizer->Add(new wxStaticText(m_pFlagsDetail, -1, "Status Flags"),
            0, wxADJUST_MINSIZE, 1);
    topSizer->Add(statusflags);
    topSizer->Add(new wxStaticText(m_pFlagsDetail, -1, "Mac Flags"),
            0, wxADJUST_MINSIZE, 1);
    topSizer->Add(macflags);
    m_pFlagsDetail->SetSizerAndFit(topSizer);
    m_pFlagsDetail->SetAutoLayout(TRUE);
    m_pFlagsDetail->Layout();
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
    m_pGridCode->SetCellBackgroundColour(m_previous, 3, *wxWHITE);
    m_pGridCode->SetCellBackgroundColour(m_previous, 4, *wxWHITE);

    m_pVu1->Reset();
    m_pParser->InitCodeMem();
    DrawProgram();
    if(m_pParser->LoadCode((char *)m_codeFile.GetFullPath().c_str())) {
        for(i=0; i< Instr.nInstructionDef; i++) {
            for(j=0; j<15; j++) {
                Instr.Instr[i].lastthr[j]=0;
            }
        }
        Status=READY;
        DrawProgram();
        m_pTextDebug->AppendText("VU Code reloaded from file: " +
            m_codeFile.GetFullPath() + "\n");
        m_pTextDebug->AppendText("Status=READY\n");
        InstuctionStatus();
    } else{
        wxMessageBox(wxString::Format("Syntax Error in line: %d",
                m_pVu1->NInstructions+1), "", wxOK|wxICON_INFORMATION, this);
        TextDebugFailed("Failed to load VU Code from file: " + 
            m_codeFile.GetFullPath() + "\n");
        m_pTextDebug->AppendText("Status=EMPTY\n");
        m_pVu1->Reset();
        Status = wxRESET;
    }

    if(m_dataFile.GetFullPath() != "") {
        LoadMemory(m_dataFile);
    } else {
        TextDebugFailed(wxString("Failed to load VU Data from file: " + 
            m_dataFile.GetFullPath() + "\n"));
    }
    m_pGridCode->SetGridCursor(0, 3);
}

//---------------------------------------------------------------------------
void
VUFrame::OnStep(wxCommandEvent &WXUNUSED(event)) {
    int cur = m_pVu1->PC;
    m_pVu1->SetCallback(this, VUFrame::wrapper_DebugTic);
    m_pVu1->SetXGKICKCallback(this, VUFrame::wrapper_XGKICK);
    m_pVu1->Tic();
    m_pGridCode->SetCellValue(m_previous, 0, wxString(""));
    m_pGridCode->SetCellValue(m_pVu1->PC, 2, wxString::Format("%d",
            m_pVu1->program[cur].tics));

    // KLUDGE
    if ( running == 0 ) {
        m_pGridCode->SetCellBackgroundColour(m_previous, 3, *wxWHITE);
        m_pGridCode->SetCellBackgroundColour(m_previous, 4, *wxWHITE);
        m_pGridCode->SetCellValue(m_pVu1->PC, 0, wxString(">"));
        m_pGridCode->SetCellBackgroundColour(m_pVu1->PC, 3, cCurCode);
        m_pGridCode->SetCellBackgroundColour(m_pVu1->PC, 4, cCurCode);
        m_pGridCode->SetGridCursor(m_pVu1->PC, 4);

        m_previous = m_pVu1->PC;
        if ( m_previous < cur ) {
            m_pGridCode->MoveCursorDown(FALSE);
        } else if ( cur < m_previous ) {
            m_pGridCode->MoveCursorUp(FALSE);
        }
    }

    accumulatedTicks += m_pVu1->program[cur].tics;

    if ( running == 0 ) {
        RegisterUpdate();
        FlagsUpdate(); 
    }
    InstuctionStatus();
}

void
VUFrame::OnRun(wxCommandEvent &event) {
    uint32 i = 0;
    running = 1;
    Breakpoint *bp = Breakpoint::Instance(); 
    while( (m_pVu1->program[m_pVu1->PC].flg!='E') &&
            (i < MAX_VUCODE_SIZE) &&
            !bp->check()
            ) {
        OnStep(event);
        i++;
    }
    m_pGridCode->SetGridCursor(LineInstruction(m_pVu1->PC), 4);
    m_pGridCode->MoveCursorDown(FALSE);
    RegisterUpdate();
    FlagsUpdate(); 
    // KLUDGE 
    running = 0;
}

//---------------------------------------------------------------------------
void
VUFrame::OnSettings(wxCommandEvent &WXUNUSED(event)) {
    m_pPrefs = new Prefs();
    PreferenceDlg (this, m_pPrefs);
    // Save away current settings
    VUFrame::SetSettings();
}

void
VUFrame::SetSettings() {
    wxConfig *conf = new wxConfig(wxTheApp->GetAppName());
    wxString key = PAGE_LOAD;
    autoLoadLast = conf->Read(key + _T("/") + AUTOLOADLAST, 0L);
    m_dataFile.Assign(conf->Read(key + _T("/") + LASTFILEMEM)); 
    m_codeFile.Assign(conf->Read(key + _T("/") + LASTFILECODE)); 
    m_memStateFile.Assign(conf->Read(key + _T("/") + MEMSTATEFILE));
    m_regStateFile.Assign(conf->Read(key + _T("/") + REGSTATEFILE));
    m_mnemonicFile.Assign(conf->Read(key + _T("/") + MNEMONICFILE));
    codeAdressStyle = 0;

    key = PAGE_REMOTE;
    autoGSExec = conf->Read(key + _T("/") + AUTOGSEXEC, 0L);
    m_binTmpFile = conf->Read(key + _T("/") + BINTMPFILE);
    m_datTmpFile = conf->Read(key + _T("/") + DATTMPFILE);
    m_regTmpFile = conf->Read(key + _T("/") + REGTMPFILE);
    m_gsTmpFile = conf->Read(key + _T("/") + GSTMPFILE);

    Remote::SetTmpFiles(
        m_datTmpFile, m_binTmpFile, m_regTmpFile,
        m_regTmpFile, m_gsTmpFile
        );

    m_dumpMemCmd = conf->Read(key + _T("/") + DUMPMEMCMD);
    m_dumpRegCmd = conf->Read(key + _T("/") + DUMPREGCMD);
    m_gsExecCmd = conf->Read(key + _T("/") + GSEXECCMD);

    key = PAGE_STYLE;
    wxString fontname = conf->Read(key + _T("/") + FONTNAME);
    fontname = conf->Read(key + _T("/") + FONTNAME);
    if ( m_pGridCode != NULL ) {
        wxFont font (10, wxMODERN, wxNORMAL, wxNORMAL, false, fontname);
        m_pGridCode->SetDefaultCellFont(font);
    }
    // set font here.

    key = PAGE_GIF;
    yoffset = conf->Read(key + _T("/") + YOFFSET, 0L);
    xoffset = conf->Read(key + _T("/") + XOFFSET, 0L);
    sendPrim = conf->Read(key + _T("/") + SENDPRIM, 0L);
    tagShow = conf->Read(key + _T("/") + TAGSHOW, 0L);
    ClrColor = conf->Read(key + _T("/") + CLRCOLOR, 0L);
    scissorX = conf->Read(key + _T("/") + SCISSOR_X, 0L);
    scissorY = conf->Read(key + _T("/") + SCISSOR_Y, 0L);
    prim = conf->Read(key + _T("/") + PRIM, 0L);

    Remote::SetGsInit(xoffset, yoffset, scissorX, scissorY, ClrColor);
    Remote::SetGsClear(0, 0, 640, 512, ClrColor);
}

//---------------------------------------------------------------------------
void VUFrame::OnQuit(wxCommandEvent &WXUNUSED(event)) {
    wxMessageDialog *ask = new wxMessageDialog(this, "Are you sure?", "Exit",
        wxOK | wxCANCEL);
    if (ask->ShowModal() == wxID_OK) {
        wxConfig *conf = new wxConfig(wxTheApp->GetAppName());
        if ( m_dataFile.GetFullPath()  != "" ) {
            wxString key = PAGE_LOAD;
            conf->Write(key + _T("/") + LASTFILEMEM, m_dataFile.GetFullPath());
        }
        if (m_codeFile.GetFullPath() != "") {
            wxString key = PAGE_LOAD;
            conf->Write(key + _T("/") + LASTFILECODE, m_codeFile.GetFullPath());
        }
        if (m_mnemonicFile.GetFullPath() != "") {
            wxString key = PAGE_LOAD;
            conf->Write(key + _T("/") + MNEMONICFILE, m_mnemonicFile.GetFullPath());
        }

        wxString key = PAGE_TRACE;
        conf->Write(key + _T("/") + TRACEDMA,
            m_pMenuOptions->IsChecked(ID_OPTION_TRACE_DMA));
        conf->Write(key + _T("/") + TRACEGIF,
               m_pMenuOptions->IsChecked(ID_OPTION_TRACE_GIF));
        conf->Write(key + _T("/") + TRACEVIF,
            m_pMenuOptions->IsChecked(ID_OPTION_TRACE_VIF));
        conf->Write(key + _T("/") + TRACEVU,
            m_pMenuOptions->IsChecked(ID_OPTION_TRACE_VU));
        conf->Flush();
        Close(TRUE);
    }
}

// All the Remote gui functions here.
//---------------------------------------------------------------------------
void
VUFrame::OnVu0All(wxCommandEvent &WXUNUSED(event)) {
    int32 ret = 0;
    if ( m_binTmpFile == "" ) {
        wxMessageBox("No binary temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( m_datTmpFile == "" ) {
        wxMessageBox("No data temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( (ret = Remote::GetVu(0)) == 0) {
        m_pParser->LoadCode((char *)m_binTmpFile.c_str());
        DrawProgram();
        LoadMemory(m_datTmpFile);
    } else {
        m_pLog->Error(ret);
    }

    if ( (ret = Remote::GetVuRegisters(0)) == 0) {
        m_pVu1->LoadRegisters(m_regTmpFile.c_str()); 
        RegisterUpdate();
    } else {
        m_pLog->Error(ret);
    }
}

//---------------------------------------------------------------------------
void
VUFrame::OnVu1All(wxCommandEvent &WXUNUSED(event)) {
    int32 ret = 0;
    if ( m_binTmpFile == "" ) {
        wxMessageBox("No binary temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( m_datTmpFile == "" ) {
        wxMessageBox("No data temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( (ret = Remote::GetVu(1)) == 0) {
        m_pParser->LoadCode((char *)m_binTmpFile.c_str());
        DrawProgram();
        LoadMemory(m_datTmpFile);
    } else {
        m_pLog->Error(ret);
    }

    if ( (ret =Remote::GetVuRegisters(1)) == 0) {
        m_pVu1->LoadRegisters(m_regTmpFile.c_str()); 
        RegisterUpdate();
    } else {
        m_pLog->Error(ret);
    }
}

//---------------------------------------------------------------------------
void VUFrame::OnVu0(wxCommandEvent &WXUNUSED(event)) {
    int32 ret = 0;
    if ( m_binTmpFile == "" ) {
        wxMessageBox("No binary temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( m_datTmpFile == "" ) {
        wxMessageBox("No data temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( (ret = Remote::GetVu(0)) == 0) {
        m_pParser->LoadCode((char *)m_binTmpFile.c_str());
        DrawProgram();
        LoadMemory(m_datTmpFile);
    } else {
        m_pLog->Error(ret);
    }
}

//---------------------------------------------------------------------------
void VUFrame::OnVu1(wxCommandEvent &WXUNUSED(event)) {
    int32 ret = 0;
    if ( m_binTmpFile == "" ) {
        wxMessageBox("No binary temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( m_datTmpFile == "" ) {
        wxMessageBox("No data temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( (ret = Remote::GetVu(1)) == 0) {
        m_pParser->LoadCode((char *)m_binTmpFile.c_str());
        DrawProgram();
        LoadMemory(m_datTmpFile);
    } else {
        m_pLog->Error(ret);
    }
}

//---------------------------------------------------------------------------
void VUFrame::OnGetMiscRegs(wxCommandEvent &WXUNUSED(event)) {
    int32 ret = 0;
    if ( m_regTmpFile == "" ) {
        wxMessageBox("No register temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( (ret = Remote::GetMiscRegisters()) == 0) {
        // MiscRegisterPanel::UpdateMiscRegs((char *)m_regTmpFile.c_str());
    } else {
        m_pLog->Error(ret);
    }
}

void VUFrame::OnRegsVu0(wxCommandEvent &WXUNUSED(event)) {
    int32 ret = 0;
    if ( m_regTmpFile == "" ) {
        wxMessageBox("No register temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( (ret = Remote::GetVuRegisters(0)) == 0) {
        m_pVu1->LoadRegisters(m_regTmpFile.c_str()); 
        RegisterUpdate();
    } else {
        m_pLog->Error(ret);
    }
}

void VUFrame::OnRegsVu1(wxCommandEvent &WXUNUSED(event)) {
    int32 ret = 0;
    if ( m_regTmpFile == "" ) {
        wxMessageBox("No register temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( (ret = Remote::GetVuRegisters(1)) == 0) {
        m_pVu1->LoadRegisters(m_regTmpFile.c_str()); 
        RegisterUpdate();
    } else {
        m_pLog->Error(ret);
    }
}

void
VUFrame::OnGsInit(wxCommandEvent &WXUNUSED(event)) {
    int32 ret = 0;
    if ( m_gsTmpFile == "" ) {
        wxMessageBox("No GS temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }
    if ( (ret = Remote::GsInit()) != 0 ) {
        m_pLog->Error(ret);
    }
}

void
VUFrame::OnGsClear(wxCommandEvent &WXUNUSED(event)) {
    int32 ret = 0;
    if ( m_gsTmpFile == "" ) {
        wxMessageBox("No GS temp file set in preferences.", "", wxOK|wxICON_INFORMATION, this);
        return;
    }

    if ( (ret = Remote::GsSetColor()) != 0 ) {
        m_pLog->Error(ret);
    }
}

//
void VUFrame::OnHelp(wxCommandEvent &WXUNUSED(event)) {
    wxMessageBox("Unimplemented.", "Help", wxOK|wxICON_INFORMATION, this);
}

void
VUFrame::OnLoadMem(wxCommandEvent &WXUNUSED(event)) {
    wxFileDialog* dlg = new wxFileDialog(this, "Choose a data file");
    if (dlg->ShowModal() == wxID_OK &&
        dlg->GetFilename() != "") {
        m_dataFile.Assign(dlg->GetPath());
        LoadMemory(m_dataFile);
    }
    delete dlg;
}

int
VUFrame::LoadMemory(wxFileName file) {
    FILE *fd;
    struct stat sb;
    uint32 i;

    if ((stat(file.GetFullPath().c_str(), &sb)) != 0 ) {
        return E_FILE_OPEN;
    }
    // KLUDGE
    // replace with a Vu::MemReadFromFile()
    if ((fd = fopen(file.GetFullPath().c_str(), "rb")) != NULL) {
        for(i = 0; i < MAX_VUDATA_SIZE && i < sb.st_size/16; i++) {
            uint32 x, y, z, w;
            fread(&x, sizeof(int32), 1, fd);
            fread(&y, sizeof(int32), 1, fd);
            fread(&z, sizeof(int32), 1, fd);
            fread(&w, sizeof(int32), 1, fd);
            m_pVu1->WriteMemX(i, x);
            m_pVu1->WriteMemY(i, y);
            m_pVu1->WriteMemZ(i, z);
            m_pVu1->WriteMemW(i, w);
            if ( tagShow == 0 ) {
            } else {
            }
        }
        fclose(fd);
        m_pTextDebug->AppendText("VU Data loaded from file: " +
            file.GetFullPath() + "\n");
    } else {
        TextDebugFailed(wxString("Failed to load VU Data from file: " +
                m_dataFile.GetFullPath() + "\n"));
        return E_FILE_OPEN;
    }
    return 0;
}

void VUFrame::TextDebugFailed(wxString message) {
        m_pTextDebug->SetDefaultStyle(wxTextAttr(wxNullColour, cWarning));
        m_pTextDebug->AppendText(message);
        m_pTextDebug->SetDefaultStyle(wxTextAttr(wxNullColour, *wxWHITE));
}

//---------------------------------------------------------------------------
void VUFrame::OnSaveCode(wxCommandEvent &WXUNUSED(event)) {
    wxFileDialog* m_pDlgSaveFile = new wxFileDialog(this, "Choose a file");
    if ( m_pDlgSaveFile->ShowModal() ) {
    }
    delete m_pDlgSaveFile;
}

void VUFrame::OnSaveState(wxCommandEvent &WXUNUSED(event)) {
    FILE *fd;
    float x,y,z,w;
    int r;
    uint32 i;
    // Save memory
    if ( (fd = fopen(m_memStateFile.GetFullPath().c_str(), "wb")) != NULL ) {
        for(i = 0; i < MAX_VUDATA_SIZE; i++ ) {
            x = m_pVu1->ReadMemX(i);
            y = m_pVu1->ReadMemY(i);
            z = m_pVu1->ReadMemZ(i);
            w = m_pVu1->ReadMemW(i);
            fwrite(&x, sizeof(float), 1, fd);
            fwrite(&y, sizeof(float), 1, fd);
            fwrite(&z, sizeof(float), 1, fd);
            fwrite(&w, sizeof(float), 1, fd);
        }
    } else {
        TextDebugFailed(wxString("Save vu data to file: %s failed.: " +
            m_memStateFile.GetFullPath() + "\n"));
    }
    // Save regs
    if ( (fd = fopen(m_regStateFile.GetFullPath().c_str(), "wb")) != NULL ) {
        for(i = 0; i < 32; i++) {
            x = m_pVu1->RegFloat[i].x();
            y = m_pVu1->RegFloat[i].y();
            z = m_pVu1->RegFloat[i].z();
            w = m_pVu1->RegFloat[i].w();
            fwrite(&x, sizeof(float), 1, fd);
            fwrite(&y, sizeof(float), 1, fd);
            fwrite(&z, sizeof(float), 1, fd);
            fwrite(&w, sizeof(float), 1, fd);
        }
        for(i = 0; i < 16; i++) {
            r = m_pVu1->RegInt[i].value();
            fwrite(&r, sizeof(int), 1, fd);
        }
    } else {
        TextDebugFailed(wxString("Save vu registers to file: %s failed.: " +
            m_regStateFile.GetFullPath() + "\n"));
    }
}

void
VUFrame::OnReset(wxCommandEvent &WXUNUSED(event)) {
    accumulatedTicks = 0;
    m_pVu1->Reset();
    m_pParser->InitCodeMem();
    m_pTextDebug->Clear();
    m_pTextStatus->Clear();
    DrawProgram();
    RegisterUpdate();
    Status = RESET;
}


void
VUFrame::OnSelectCodeCell(wxCommandEvent &WXUNUSED(event)) {
    m_pGridCode->SetCellValue(m_pGridCode->GetCursorRow(), 0, wxString(""));
    m_pGridCode->SetCellValue(m_pGridCode->GetCursorRow(), 0, wxString(">"));
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
    wxFileDialog *dlg= new wxFileDialog(this, "Choose a project file");
    if(dlg->ShowModal() == wxID_OK) {
        ifstream fin(dlg->GetPath());
        fin.getline(line, 256);
        if(strstr(line, "codefile") != 0) {
            ptr = strchr(line, '=');
            m_codeFile.Assign(ptr+1);
        } else if ( strstr(line, "datafile")  != 0) {
            ptr = strchr(line, '=');
            m_dataFile.Assign(ptr+1);
        }
        fin.getline(line, 256);
        if(strstr(line, "codefile") != 0) {
            ptr = strchr(line, '=');
            m_codeFile.Assign(ptr+1);
        } else if ( strstr(line, "datafile")  != 0) {
            ptr = strchr(line, '=');
            m_dataFile.Assign(ptr+1);
        }
        fin.close();
        m_codeFile.PrependDir(dlg->GetDirectory());
        m_dataFile.PrependDir(dlg->GetDirectory());
        if ( m_pParser->LoadCode((char *)m_codeFile.GetFullPath().c_str()) ) {
            Status = READY;
            DrawProgram();
            m_pTextDebug->AppendText(wxString("VU Code loaded from file: " +
                    m_codeFile.GetFullPath() + "\n"));
            InstuctionStatus();
        } else {
            TextDebugFailed(wxString("Failed to load VU Code from file: " +
                    m_codeFile.GetFullPath() + "\n"));
        }
        LoadMemory(m_dataFile);
    }
    delete dlg;
}

//---------------------------------------------------------------------------
void
VUFrame::OnLoadVIF(wxCommandEvent &WXUNUSED(event)) {
    wxFileDialog* dlg = new wxFileDialog(this, "Choose a VIF file");
    dlg->SetWildcard("*.bin");
    if (dlg->ShowModal() == wxID_OK) {
        m_vifFile.Assign(dlg->GetPath());
        m_pVif1->Open(m_vifFile.GetFullPath().c_str());
        m_pVif1->Read();
        m_pVif1->Close();
        DrawProgram();
        RegisterUpdate();
        FlagsUpdate(); 
    }
    delete dlg;
    return;
}

//---------------------------------------------------------------------------
void
VUFrame::OnLoadDMA(wxCommandEvent &WXUNUSED(event)) {
    wxFileDialog* dlg = new wxFileDialog(this, "Choose a DMA file");
    if (dlg->ShowModal() == wxID_OK) {
        m_vifFile.Assign(dlg->GetPath());
        m_pDma->Open(m_vifFile.GetFullPath().c_str());
        m_pDma->Read();
        m_pDma->Close();
        DrawProgram();
    }
    delete dlg;
    return;
}

//---------------------------------------------------------------------------
void
VUFrame::OnLoadCode(wxCommandEvent &WXUNUSED(event)) {
    wxFileDialog* dlg = new wxFileDialog(this, "Choose a code file");
    if (dlg->ShowModal() == wxID_OK) {
        m_codeFile.Assign(dlg->GetPath());
        if(m_pParser->LoadCode((char *)dlg->GetPath().c_str())) {
            Status = READY;
            DrawProgram();
            m_pTextDebug->AppendText(wxString("VU Code loaded from file: " +
                    m_codeFile.GetFullPath() + "\n"));
            InstuctionStatus();
        } else {
            TextDebugFailed(wxString("Failed to load VU Code from file: " +
                    m_codeFile.GetFullPath() + "\n"));
        }
    }
    delete dlg;
}

//---------------------------------------------------------------------------
// Various helper functions.
//---------------------------------------------------------------------------
void
VUFrame::DrawParam(VuParam &p, char *a) {
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
                sprintf(a, "%d(VI%02d)", (uint32)p.data, p.index);
            }
            break;
        case 13: //Imm11(VI)dest
            if(p.label[0]) {
                sprintf(a,"%s(VI%02d)%s", p.label, p.index, strlwr(p.sufix));
            } else {
                sprintf(a,"%d(VI%02d)%s", (uint32)p.data, p.index, strlwr(p.sufix));
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

    m_previous = 0;
    m_pGridCode->SetCellBackgroundColour(0, 3, cCurCode);
    m_pGridCode->SetCellBackgroundColour(0, 4, cCurCode);
    for(i = 0; i < MAX_VUCODE_SIZE; i++) {
        m_pGridCode->SetCellValue(i, 1, wxString(""));
        m_pGridCode->SetCellValue(i, 2, wxString(""));
        m_pGridCode->SetCellValue(i, 3, wxString(""));
        m_pGridCode->SetCellValue(i, 4, wxString(""));
    }
    for (i = 0; i < MAX_VUCODE_SIZE; i++,l++){
        if(m_pVu1->program[i].SymbolIndex != -1) {
            // cout << "we got a label" << endl;
            strcpy(scode,m_pVu1->Labels[m_pVu1->program[i].SymbolIndex].symb);
            // cout << m_pVu1->Labels[m_pVu1->program[i].SymbolIndex].symb << endl;
            m_pGridCode->SetCellValue(l, 1, scode);
        }
        m_pVu1->program[i].addr = i;
        m_pGridCode->SetCellValue(l, 2, wxString::Format("%d",
                m_pVu1->program[i].tics));
        for(m=UPPER; m<=LOWER; m++) {
            strcpy(auxi,Instr.Instr[m_pVu1->program[i].InstIndex[m]].nemot);
            if(m_pVu1->program[i].flg && !m) {
                sprintf(aux2,"[%c]",m_pVu1->program[i].flg);
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
            strcpy(scode, auxi);
            if(m_pVu1->program[i].flavor[m])
                strcat(scode,flavors[m_pVu1->program[i].flavor[m]]);
            if(m_pVu1->program[i].dest[m][0]) {
                strcat(scode, ".");
                strcpy(auxi, m_pVu1->program[i].dest[m]);
                strcat(scode, strlwr(auxi));
            }
            for (j=0; j<Instr.Instr[m_pVu1->program[i].InstIndex[m]].operands; j++) {
                DrawParam(m_pVu1->program[i].Params[m][j], param);
                strcat(scode, " ");
                strcat(scode, param);
                if(j<Instr.Instr[m_pVu1->program[i].InstIndex[m]].operands-1) {
                    strcat(scode,",");
                }
            }
            m_pGridCode->SetCellValue(l, 3+m, scode);
        }
    }
}

int VUFrame::LineInstruction(int a) {
    int i, l = 0;
    for(i=0; i<=a; i++, l++) {
        if(m_pVu1->program[i].SymbolIndex!=-1) {
            l++;
        }
    }
    return (l);
}

// TODO
// remove in favor of vu core updating registers
void VUFrame::RegisterUpdate() {
    VuFloatReg* Reg;
    uint32 i;

    for(i = 0; i < 16; i++) {
        m_pRegisterView->WriteInt(i, m_pVu1->RegInt[i].value());
    }

    for(i = 0; i < 32; i++) {
        Reg = &m_pVu1->RegFloat[i];
        m_pRegisterView->WriteFloat(
            i,
            Reg->x(),
            Reg->y(),
            Reg->z(),
            Reg->w()
            );
    }

    m_pRegisterView->WriteAcc(
        m_pVu1->ACC.x(),
        m_pVu1->ACC.y(),
        m_pVu1->ACC.z(),
        m_pVu1->ACC.w()
        );
    m_pRegisterView->WriteQ(
        m_pVu1->Q.x(),
        m_pVu1->Q.y(),
        m_pVu1->Q.z(),
        m_pVu1->Q.w()
        );
    m_pRegisterView->WriteP(
        m_pVu1->P.x(),
        m_pVu1->P.y(),
        m_pVu1->P.z(),
        m_pVu1->P.w()
        );
    m_pRegisterView->WriteR(
        m_pVu1->R.x(),
        m_pVu1->R.y(),
        m_pVu1->R.z(),
        m_pVu1->R.w()
        );
    m_pRegisterView->WriteI(
        m_pVu1->I.x(),
        m_pVu1->I.y(),
        m_pVu1->I.z(),
        m_pVu1->I.w()
        );
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
    self->DrawGif((uint32)offset);
}

void
VUFrame::wrapper_DebugWarning(void *objPtr, wxString message) {
    VUFrame *self = (VUFrame*)objPtr;
    self->TextDebugFailed(message);
}

void
VUFrame::DrawGif(uint32 offset) {
    uint32 size = (MAX_VUDATA_SIZE-offset)*16;
    uint32 *data = (uint32 *)malloc(size);
    uint32 i, j;

    j = 0;
    // KLUDGE
    // will be pushed down to VU kicking data directly to GIF wich then kicks
    // to GS and GS will have the logic for sending to PS2 and/or 
    // displaying in a GL window/display list tab
    m_pVu1->ReadMem(data, offset, size);

    if (autoGSExec == 0) {
        if (sendPrim == 0) {
            Remote::GsExec((unsigned char *)data, size);
        }
    }

    Gif* localGif = new Gif(data, size);
    localGif->xoffset = xoffset;
    localGif->yoffset = yoffset;
    if ( localGif->Unpack() ) {
        vector<string> v = localGif->TagAsString();
        vector<string>::const_iterator it = v.begin();
        i = 0;
        while(it != v.end()) {
            m_pGifPanel->SetBackgroundColour(cGIFtag);
            m_pGifPanel->Write(
                wxString( ((string)*it).c_str()),
                wxString( ((string)*(it+1)).c_str() )
                );
            it = it + 2;
            i++;
        }
        i++;
        for(j = 0;j < localGif->GetNloop(); j++ ) {
            v = localGif->NloopData();
            it = v.begin();
            while(it != v.end()) {
                if (i > MAX_VUDATA_SIZE ) {
                    break;
                }
                if(j&0x1) {
                    m_pGifPanel->SetBackgroundColour(cNloop1);
                } else {
                    m_pGifPanel->SetBackgroundColour(cNloop2);
                }
                m_pGifPanel->Write(
                    wxString(((string)*it).c_str()),
                    wxString( ((string)*(it+1)).c_str() )
                );
                it = it + 2;
                i++;
            }
            if (i > MAX_VUDATA_SIZE ) {
                break;
            }
        }
    } else {
        m_pLog->Error(
            wxString::Format("Invalid Gif tag at offset: %d", offset)
            );
    }

    free(data);
    delete localGif;
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
        m_pLog->Trace("End of program reached\n");
        return;
    }
    if(mode==2) {
        i = LineInstruction(m_pVu1->PC-1);
        sprintf(buffer, "Instruction line %d [%s][%s] completed\n",m_pVu1->PC,
            m_pGridCode->GetCellValue(i, 3).c_str(),
            m_pGridCode->GetCellValue(i, 4).c_str());
        m_pTextDebug->AppendText(buffer);
    } else {
        i = LineInstruction(m_pVu1->PC);
        sprintf(buffer, "Executing instruction line %d %s [%s]\n", m_pVu1->PC,
            modedef[mode], m_pGridCode->GetCellValue(i, 3+mode).c_str());
        m_pTextDebug->AppendText(buffer);
        sprintf(buffer,"Result: %s\n",succes[error]);
        m_pTextDebug->AppendText(buffer);
    }
}

//---------------------------------------------------------------------------
// Update flag panel when receiving active event
void
VUFrame::FlagsUpdate() {
    char data[100];
    sprintf (data,"%06ld",iToBin(m_pVu1->ClipFlag[3]));
    clip3->SetValue(data);
    sprintf (data,"%06ld",iToBin(m_pVu1->ClipFlag[2]));
    clip2->SetValue(data);
    sprintf (data,"%06ld",iToBin(m_pVu1->ClipFlag[1]));
    clip1->SetValue(data);
    sprintf (data,"%06ld",iToBin(m_pVu1->ClipFlag[0]));
    clip0->SetValue(data);

    sprintf (data,"%04ld",iToBin(m_pVu1->MacZ));
    MZf->SetValue(data);
    sprintf (data,"%04ld",iToBin(m_pVu1->MacS));
    MSf->SetValue(data);
    sprintf (data,"%04ld",iToBin(m_pVu1->MacU));
    MUf->SetValue(data);
    sprintf (data,"%04ld",iToBin(m_pVu1->MacO));
    MOf->SetValue(data);

    fz->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 1)));
    fs->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 2)/2));
    fu->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 4)/4));
    fo->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 8)/8));
    fi->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 16)/16));
    fd->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 32)/32));
    fzs->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 64)/64));
    fss->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 128)/128));
    fus->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 256)/256));
    fos->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 512)/512));
    fis->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 1024)/1024));
    fds->SetValue(wxString::Format("%d\n", (m_pVu1->StatusFlag & 2048)/2048));
}

//---------------------------------------------------------------------------
// OnInstructionStatus
void VUFrame::InstuctionStatus() {
    char data[100];
    int i,j;

    if(Status!=READY) {
        return;
    }

    i = LineInstruction(m_pVu1->PC);
    m_pTextStatus->Clear();
    m_pTextStatus->AppendText("------------------------\n");
    m_pTextStatus->AppendText("    Instrucion UPPER    \n");
    m_pTextStatus->AppendText("------------------------\n");
    sprintf(data," [%s] ", m_pGridCode->GetCellValue(i, 3).c_str());
    m_pTextStatus->AppendText(data);
    m_pTextStatus->AppendText("Can be executed...\n");
    j=Instr.Instr[m_pVu1->program[m_pVu1->PC].InstIndex[0]].lastthr[m_pVu1->program[m_pVu1->PC].flavor[0]];
    if(j>1) {
        sprintf(data,"After %d cycles",j-1);
    } else {
        sprintf(data,"Inmediatly");
    }
    m_pTextStatus->AppendText(data);
    m_pTextStatus->AppendText("\n");
    m_pTextStatus->AppendText("------------------------\n");
    m_pTextStatus->AppendText("    Instrucion LOWER    \n");
    m_pTextStatus->AppendText("------------------------\n");
    sprintf(data," [%s] ", m_pGridCode->GetCellValue(i, 4).c_str());
    m_pTextStatus->AppendText(data);
    m_pTextStatus->AppendText("Can be executed...");
    j=Instr.Instr[m_pVu1->program[m_pVu1->PC].InstIndex[1]].lastthr[m_pVu1->program[m_pVu1->PC].flavor[1]];
    if(j>1) {
        sprintf(data,"After %d cycles\n",j-1);
    } else {
        sprintf(data,"Inmediately\n");
    }
    m_pTextStatus->AppendText(data);
    m_pTextStatus->AppendText("\n");
    m_pTextStatus->AppendText("NOTE: Information is regarding instruction throughput.\n");
    m_pTextStatus->AppendText("Other issues such Register STALL may occur.\n");
}

void
VUFrame::BuildToolbar(void) {
    m_pMenuBar = new wxMenuBar;
    m_pMenuFile = new wxMenu;
    m_pMenuTools = new wxMenu;
    m_pMenuOptions = new wxMenu;
    m_pMenuHelp = new wxMenu;
    m_pMenuRemote = new wxMenu;

    m_pMenuFile->Append(ID_FILE_LOADCODE,   "&Load code\tCtrl-l");
    m_pMenuFile->Append(ID_FILE_LOADMEM,    "&Load mem\tCtrl-m");
    m_pMenuFile->Append(ID_FILE_LOADPROJECT,"&Load project\tCtrl-p");
    m_pMenuFile->AppendSeparator();
    m_pMenuFile->Append(ID_FILE_LOADVIF,    "&Load VIF data");
    m_pMenuFile->Append(ID_FILE_LOADDMA,    "&Load DMA data");
    m_pMenuFile->AppendSeparator();
    m_pMenuFile->Append(ID_FILE_SAVESTATE,  "&Save state");
    m_pMenuFile->Append(ID_FILE_QUIT,       "&Exit\tCtrl-q");

    m_pMenuTools->Append(ID_TOOL_RESET,    "Reset\tF1");
    m_pMenuTools->Append(ID_TOOL_RESTART,  "Restart\tF2");
    m_pMenuTools->Append(ID_TOOL_STEP,     "Step\tF3");
    m_pMenuTools->Append(ID_TOOL_RUN,      "Run\tF4");
    // m_pMenuTools->Append(ID_TOOL_CLEARMEM,      "Clear Memory");
    // m_pMenuTools->Append(ID_TOOL_CLEARCODE,     "Clear Code");

    m_pMenuOptions->Append(ID_OPTION_SETTINGS,  "Preferences\tCtrl-p");
    m_pMenuOptions->AppendSeparator();
    m_pMenuOptions->AppendCheckItem(ID_OPTION_TRACE_DMA, "Trace DMA", "DMA");
    m_pMenuOptions->AppendCheckItem(ID_OPTION_TRACE_GIF, "Trace GIF", "GIF");
    m_pMenuOptions->AppendCheckItem(ID_OPTION_TRACE_VIF, "Trace VIF", "VIF");
    m_pMenuOptions->AppendCheckItem(ID_OPTION_TRACE_VU,  "Trace VU", "VU");
    wxString key = PAGE_TRACE;
    wxConfig *conf = new wxConfig(wxTheApp->GetAppName());
    m_pMenuOptions->Check(ID_OPTION_TRACE_DMA, conf->Read(
            key + _T("/") + TRACEDMA, 0L));
    m_pMenuOptions->Check(ID_OPTION_TRACE_GIF, conf->Read(
            key + _T("/") + TRACEGIF, 0L));
    m_pMenuOptions->Check(ID_OPTION_TRACE_VIF, conf->Read(
            key + _T("/") + TRACEVIF, 0L));
    m_pMenuOptions->Check(ID_OPTION_TRACE_VU, conf->Read(
            key + _T("/") + TRACEVU, 0L));
    delete conf;
    // m_pMenuOptions->Append(ID_OPTION_CLEARMEM,   "Always clear memory on reset");
    // m_pMenuOptions->Append(ID_OPTION_CLEARCODE,  "Always clear registers on reset");
    // m_pMenuOptions->Append(ID_OPTION_HIDE_PANEL3, "Hide right panel");
    // m_pMenuOptions->Append(ID_OPTION_HIDE_PANEL2, "Hide bottom panel");
    
    m_pMenuRemote->Append(ID_REMOTE_VU0,    "Get VPU0 content\tF7");
    m_pMenuRemote->Append(ID_REMOTE_VU1,    "Get VPU1 content\tF8");
    m_pMenuRemote->Append(ID_REMOTE_REGSVU0,"Get VPU0 Registers\tF9");
    m_pMenuRemote->Append(ID_REMOTE_REGSVU1,"Get VPU1 Registers\tF10");
    m_pMenuRemote->Append(ID_REMOTE_VU0ALL, "Get VPU0 content and registers\tF11");
    m_pMenuRemote->Append(ID_REMOTE_VU1ALL, "Get VPU1 content and registers\tF12");
    m_pMenuRemote->AppendSeparator();
    m_pMenuRemote->Append(ID_REMOTE_REGS,   "Get misc registers\tCtrl-0");
    m_pMenuRemote->AppendSeparator();
    m_pMenuRemote->Append(ID_REMOTE_GSINIT, "Init GS on PS2");
    m_pMenuRemote->Append(ID_REMOTE_CLR,    "Clear screen");

    m_pMenuBar->Append(m_pMenuFile,     "&File");
    m_pMenuBar->Append(m_pMenuTools,    "&Tools");
    m_pMenuBar->Append(m_pMenuRemote,   "&Remote");
    m_pMenuBar->Append(m_pMenuOptions,  "&Options");
    SetMenuBar(m_pMenuBar);
}

VUFrame::VUFrame(const wxString &title, const wxPoint &pos, const wxSize
    &size) : wxFrame((wxFrame*)NULL, -1, title, pos, size) {

#ifdef WIN32
#undef GetCharWidth
#endif
    m_charWidth = wxWindow::GetCharWidth();
    m_pGridCode = NULL;

    // setup all state machines

    SetSettings();
    // Toolbar
    BuildToolbar();

    // Splitters
    wxSplitterWindow    *m_hzSplit = new wxSplitterWindow(this, -1, wxPoint(0, 0),
        wxDefaultSize, wxSP_3D);;
    wxSplitterWindow    *vertsplit = new wxSplitterWindow(m_hzSplit, -1,
        wxPoint(0, 0), wxDefaultSize, wxSP_3D);

    // Left notebook
    m_pLeftBook = new wxNotebook(vertsplit, ID_NOTEBOOK1, wxDefaultPosition,
        wxDefaultSize); 
    buildCodeTable(m_pLeftBook);
    m_pLeftBook->AddPage(m_pGridCode, "Code", TRUE, -1);
    m_pMemoryPanel = new MemoryPanel(
        m_pLeftBook, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER,
        MAX_VUDATA_SIZE
        );
    m_pLeftBook->AddPage(m_pMemoryPanel, "Memory", FALSE, -1);
    m_pGifPanel = new GifOutputPanel(
        m_pLeftBook, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER
        );
    m_pLeftBook->AddPage(m_pGifPanel, "GIF output", FALSE, -1);

    m_pLeftBook->AddPage(new MiscRegisterPanel(
            m_pLeftBook, -1, wxDefaultPosition, wxDefaultSize,
            wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER), 
        "Misc. Registers", FALSE, -1);
    m_pLeftBook->Show(TRUE);

    // Right notebook
    m_pRightBook = new wxNotebook(vertsplit, -1, wxDefaultPosition,
        wxDefaultSize);

    m_pTextDebug = new wxTextCtrl(m_pRightBook, ID_TEXT_DEBUG, wxString(""),
        wxDefaultPosition, wxSize(400, 400),
        wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH);
    m_pRightBook->AddPage(m_pTextDebug, "Debug", FALSE, -1);
 
    m_pTextStatus = new wxTextCtrl(m_pRightBook, ID_TEXT_STATUS, wxString(""),
        wxDefaultPosition, wxSize(400, 400),
        wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH);
    m_pRightBook->AddPage(m_pTextStatus, "Instruction Status", FALSE, -1);
    buildFlagsPanel(m_pRightBook);
    m_pRightBook->AddPage(m_pFlagsDetail, "Flags", FALSE, -1);

    vertsplit->SetMinimumPaneSize(100);
    vertsplit->SplitVertically(m_pLeftBook, m_pRightBook, 550);

    // bottom tabs
    m_pRegisterView = new VuRegisterPanel(
        m_hzSplit, -1, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER
        );
    m_hzSplit->SetMinimumPaneSize(100);
    m_hzSplit->SplitHorizontally(vertsplit, m_pRegisterView, 380);

    static const int widths[] = {-1, 6*m_charWidth, 6*m_charWidth, 6*m_charWidth,
        6*m_charWidth };
    CreateStatusBar(WXSIZEOF(widths), wxST_SIZEGRIP, ID_STATUSBAR);
    m_pStatusBar = GetStatusBar();

    // Init vu core
    m_pVu1      = new Vu(VPU_1, m_pMemoryPanel);
    m_pParser   = new Parser(m_pVu1);
    m_pVu1->m_pVuRegisterPanel = m_pRegisterView;

    m_pGif      = new Gif();
    m_pVif1     = new Vif1(m_pParser, m_pVu1);
    m_pDma      = new Dma();
    remoteDma   = new Dma();
    // m_pVif1     = new Vif1();
    // m_pVif0     = new Vif0();
    m_pDma->SetVif1(m_pVif1);
    m_pLog      = Log::Instance();
    m_pLog->SetTextCtrl(m_pTextDebug);

    // ok gui is up, lets load the mnemonics
    if (m_mnemonicFile.GetFullPath() == "") {
        m_mnemonicFile.Assign("instructions.txt");
    }
    if (!m_pParser->LoadInstructions((char *)m_mnemonicFile.GetFullPath().c_str())) {
        m_pLog->Error("Failed to load instructions");
        wxFileDialog* dlg = new wxFileDialog(this, "Open instructions.txt file");
        if (dlg->ShowModal() == wxID_OK &&
            dlg->GetFilename() != "") {
            m_mnemonicFile.Assign(dlg->GetPath());
            if(!m_pParser->LoadInstructions((char *)m_mnemonicFile.GetFullPath().c_str())) {
                m_pLog->Error("Failed to load instructions, Exiting");
                Close(TRUE);
            }
        }
    }

    m_pVu1->Reset();
    m_pParser->InitCodeMem();
    RegisterUpdate();
    if ( autoLoadLast == 0 ) {
        if ( m_dataFile.GetFullPath() != "" ) {
            Status = READY;
            LoadMemory(m_dataFile);
        }
        if ( m_codeFile.GetFullPath() != "" ) {
            Status = READY;
            m_pParser->LoadCode((char *)m_codeFile.GetFullPath().c_str());
        }
    } else {
        m_codeFile.Assign("");
        m_dataFile.Assign("");
    }
    DrawProgram();
}

bool VUemu::OnInit() {
    ms_pFrame = new VUFrame("VU Emu", wxPoint(10, 10), wxSize(800, 600));
    ms_pFrame->Show(TRUE);
    SetTopWindow(ms_pFrame);
    return TRUE;
}

void
VUFrame::OnOptionTraceDma(wxCommandEvent &event) {
    m_pDma->Trace(event.IsChecked());
}
void
VUFrame::OnOptionTraceGif(wxCommandEvent &event) {
    m_pGif->Trace(event.IsChecked());
}
void
VUFrame::OnOptionTraceVif(wxCommandEvent &event) {
    // m_pVif0->Trace(event.IsChecked());
    m_pVif1->Trace(event.IsChecked());
}
void
VUFrame::OnOptionTraceVu(wxCommandEvent &event) {
    m_pVu1->Trace(event.IsChecked());
}
