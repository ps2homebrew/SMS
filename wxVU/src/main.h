#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/grid.h>
#include <wx/choice.h>
#include <wx/bmpbuttn.h>

#include "prefdlg.h"
#include "prefs.h"
#include "vu.h"

static unsigned char statusBarReg[2] = {0, 0};

// Colors used
const wxColour cCurCode     = wxColour(192, 192, 192);
const wxColour cGIFtag      = wxColour(128, 255, 128);
const wxColour cNloop1      = wxColour(128, 128, 255);
const wxColour cNloop2      = wxColour(192, 192, 255);
const wxColour cWarning     = wxColour(255, 128, 128);

enum {
	ID_FILE_LOADCODE,
	ID_FILE_LOADMEM,
	ID_FILE_LOADVIF,
	ID_FILE_LOADDMA,
	ID_FILE_LOADPROJECT,
	ID_FILE_SAVECODE,
	ID_FILE_SAVESTATE,
	ID_FILE_QUIT,
    // 
	ID_TOOL_RESET,
	ID_TOOL_RESTART,
	ID_TOOL_STEP,
	ID_TOOL_RUN,
	ID_OPTION_SETTINGS,
	ID_HELP,
	ID_TEXT_DEBUG,
    ID_TEXT_STATUS,
    ID_TEXT_GIF,
    ID_CODE_CELL_SELECTED,
    ID_MEM_CELL_CHANGED,
	ID_REGSELECT,
    ID_REGINT_REP,
    ID_REGFLOAT_REP,
    ID_REGSPEC_REP,
    ID_STATUSBAR,
    ID_NOTEBOOK1,
    ID_NOTEBOOK2,
    ID_REGRADIOBOX,
    ID_GRIDMEMORY,
    ID_GRIDCODE,
    ID_GRIDREGFLOAT,
    ID_GRIDREGINT,
    ID_GRIDREGSPECIAL,
    ID_GRIDGIF,
    //
    ID_REMOTE_VU0,
    ID_REMOTE_VU1,
    ID_REMOTE_VU0ALL,
    ID_REMOTE_VU1ALL,
    ID_REMOTE_REGS,
    ID_REMOTE_REGSVU0,
    ID_REMOTE_REGSVU1,
    ID_REMOTE_GSINIT,
    ID_REMOTE_CLR,
    //
    ID_REGTREE,
    ID_REGINFOBOX
};

class VUemu : public wxApp {
    virtual bool OnInit();
};

IMPLEMENT_APP(VUemu)

static int READY = 1;
static int RESET = 2;

//---------------------------------------------------------------------------
class VUFrame: public wxFrame {
public:
        wxNotebook      *down_book;
        wxNotebook      *right_book;
        wxNotebook      *left_book;
        wxToolBar       *toolbar;
        wxMenuBar       *menuBar;
        wxMenu          *menuRemote;
        wxMenu          *menuFile;
        wxMenu          *menuTools;
        wxMenu          *menuOptions;
        wxMenu          *menuHelp;

        wxFileDialog    *dlgOpenFile;
        wxFileDialog    *dlgSaveFile;

        wxTextCtrl      *txtDebug;
        wxTextCtrl      *txtStatus;
        wxPanel         *regDetail;
        wxPanel         *flagsDetail;
        wxStatusBar     *statusBar;

        wxTextCtrl      *rx;
        wxTextCtrl      *ry;
        wxTextCtrl      *rz;
        wxTextCtrl      *rw;
        wxTextCtrl      *rstall;
        wxChoice        *RegSeL;
        wxTextCtrl      *rlastr;
        wxTextCtrl      *rlastw;
        
        wxRadioBox      *regRadioBox;
        wxRadioBox      *m_regIntNum;
        wxRadioBox      *m_regFloatNum;
        wxRadioBox      *m_regSpecNum;

        wxTextCtrl      *clip3;
        wxTextCtrl      *clip2;
        wxTextCtrl      *clip1;
        wxTextCtrl      *clip0;

        wxTextCtrl      *fds;
        wxTextCtrl      *fis;
        wxTextCtrl      *fos;
        wxTextCtrl      *fd;
        wxTextCtrl      *fzs;
        wxTextCtrl      *fss;
        wxTextCtrl      *fus;
        wxTextCtrl      *fi;
        wxTextCtrl      *fo;
        wxTextCtrl      *fu;
        wxTextCtrl      *fs;
        wxTextCtrl      *fz;
        wxTextCtrl      *MOf;
        wxTextCtrl      *MUf;
        wxTextCtrl      *MSf;
        wxTextCtrl      *MZf;
        wxGrid          *gridGIF;
        wxGrid          *gridMemory;
        wxGrid          *gridCode;
        wxGrid          *gridIntRegisters;
        wxGrid          *gridFloatRegisters;
        wxGrid          *gridSpecialRegisters;

        // Misc registers panel
        wxPanel         *panelMiscRegisters;
        wxGrid          *regInfoBox;
        wxTreeCtrl      *miscRegTree;
        wxBoxSizer      *boxMiscRegs;

        // holders for wxTree Id's, feels hackish.
        wxTreeItemId    dmaItemIds[52];
        wxTreeItemId    gifItemIds[8];
        wxTreeItemId    timerItemIds[14];
        wxTreeItemId    sifItemIds[1];
        wxTreeItemId    gsItemIds[2];
        wxTreeItemId    intcItemIds[2];
        wxTreeItemId    vif0ItemIds[18];
        wxTreeItemId    vif1ItemIds[22];
        wxTreeItemId    fifoItemIds[2];

        //      Menubar file functions
        void    OnLoadCode(wxCommandEvent &WXUNUSED(event));
        void    OnLoadMem(wxCommandEvent &WXUNUSED(event));
        void    OnLoadProject(wxCommandEvent &WXUNUSED(event));
        void    OnLoadVIF(wxCommandEvent &WXUNUSED(event));
        void    OnLoadDMA(wxCommandEvent &WXUNUSED(event));
        void    OnSaveCode(wxCommandEvent &WXUNUSED(event));
        void    OnSaveState(wxCommandEvent &WXUNUSED(event));
		//		Menubar tool functions
        void    OnReset(wxCommandEvent &WXUNUSED(event));
        void    OnRestart(wxCommandEvent &WXUNUSED(event));
        void    OnStep(wxCommandEvent &WXUNUSED(event));
        void    OnRun(wxCommandEvent &WXUNUSED(event));
        void    OnSettings(wxCommandEvent &WXUNUSED(event));
        void    OnHelp(wxCommandEvent &WXUNUSED(event));
        void    OnQuit(wxCommandEvent &WXUNUSED(event));
        //      Menubar remote functions
        void    OnVu0(wxCommandEvent &WXUNUSED(event));
        void    OnVu1(wxCommandEvent &WXUNUSED(event));
        void    OnVu0All(wxCommandEvent &WXUNUSED(event));
        void    OnVu1All(wxCommandEvent &WXUNUSED(event));
        void    OnGSInit(wxCommandEvent &WXUNUSED(event));
        void    OnRegs(wxCommandEvent &WXUNUSED(event));
        void    OnRegsVu0(wxCommandEvent &WXUNUSED(event));
        void    OnRegsVu1(wxCommandEvent &WXUNUSED(event));
        void    OnCLR(wxCommandEvent &WXUNUSED(event));

        // Panel1 on event functions
        void    OnBreakpoint(wxGridEvent &event);

        // Panel2 on event functions
        void    FlagsUpdate(void);
        void    FastRegisterUpdate(void);
        void    SetSettings(void);

        void    OnMiscRegSelect(wxTreeEvent &);
        void    updateInfoBox(int a, int b);

        VUFrame(const wxString &title, const wxPoint &pos, const wxSize
            &size);

private:
        wxPanel		*buildMemoryTable(wxNotebook *);
        void		buildCodeTable(wxNotebook *);
        void		buildGIFTable(wxNotebook *);
        void		buildToolbar(void);
        void		buildRegisterGrid(wxNotebook *);
        void		buildFlagsPanel(wxNotebook *);
        void        buildMiscRegistersTable(wxNotebook *);
        int			LoadMemory(wxFileName file);
        void		LoadRegisters(char *);
        void		updateStatusBar(void);
        void		txtDebugFailed(wxString message);
        void        InstFill(void);
        // Config stuff
        Prefs		*m_prefs;
        int			charWidth;
        int			charHeight;
        int			Previous;
        // Config Load
        int         autoLoadLast;
        int         autoGSExec;
        int         doProject;
        wxString    dataSuffix;
        wxString    codeSuffix;
        wxString    binLatest;
        wxString    datLatest;
        // Config Remote
        wxString    datTmpFile;
        wxString    binTmpFile;
        wxString    regTmpFile;
        wxString    gsTmpFile;
        wxString    dumpMemCmd;
        wxString    dumpRegCmd;
        wxString    gsExecCmd;
        wxFileName  codeFile;
        wxFileName  dataFile;
		wxFileName	memStateFile;
		wxFileName	regStateFile;
		wxFileName	mnemonicFile;
        wxFileName  vifFile;
        int			Status;
        // Config GIF
        int			xoffset;
        int			yoffset;
        int			sendPrim;
        int			tagShow;
        int         ClrColor;
        int         scissorX;
        int         scissorY;
        int			prim;
        // Config Misc
        uint32		codeAdressStyle;
public:
        void    DrawProgram();
        void    DrawMemory();
        void    DebugTic(int, int);
        void    drawGIF(uint32 offset);
        int     LineInstruction(int a);
        void    DrawParam(VUParam &p, char *a, uint32 hex);
        void    InstuctionStatus();
        static void wrapper_DebugTic(void *objPtr, int, int);
        static void wrapper_XGKICK(void *objPtr, int offset);
        static void wrapper_DebugWarning(void *objPtr, wxString message);
        void    OnSelectCodeCell(wxCommandEvent &);
        void    OnCellChange(wxGridEvent &event);
        void    OnNotebookOne(wxNotebookEvent &);
        void    OnRegRadioBox(wxCommandEvent &);
        void    OnGridLabel(wxGridEvent &);
        void    OnKeyDown(wxKeyEvent &);
        void    OnKeyUp(wxKeyEvent &);
        void    OnChar(wxKeyEvent &);
        // void    OnSize(wxSizeEvent &);
        DECLARE_EVENT_TABLE()
};

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
	EVT_MENU(ID_REMOTE_REGS, VUFrame::OnRegs)
	EVT_MENU(ID_REMOTE_REGSVU0, VUFrame::OnRegsVu0)
	EVT_MENU(ID_REMOTE_REGSVU1, VUFrame::OnRegsVu1)
	EVT_MENU(ID_REMOTE_GSINIT, VUFrame::OnGSInit)
	EVT_MENU(ID_REMOTE_CLR, VUFrame::OnCLR)
    EVT_GRID_CELL_LEFT_DCLICK(VUFrame::OnBreakpoint)
    EVT_GRID_CELL_CHANGE(VUFrame::OnCellChange)
    EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK1, VUFrame::OnNotebookOne)
    EVT_RADIOBOX(ID_REGRADIOBOX, VUFrame::OnRegRadioBox)
    EVT_GRID_LABEL_LEFT_CLICK(VUFrame::OnGridLabel)
    EVT_TREE_ITEM_ACTIVATED(ID_REGTREE, VUFrame::OnMiscRegSelect)
    // EVT_SIZE(VUFrame::OnSize)
END_EVENT_TABLE()
