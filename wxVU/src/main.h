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
#include "gif.h"

#include "dma.h"

#include "GifOutputPanel.h"
#include "MemoryPanel.h"
#include "VuRegisterPanel.h"
#include "parser.h"
#include "Log.h"

static unsigned char statusBarReg[2] = {0, 0};

// Colors used
const wxColour cCurCode     = wxColour(192, 192, 192);
const wxColour cGIFtag      = wxColour(128, 255, 128);
const wxColour cNloop1      = wxColour(128, 128, 255);
const wxColour cNloop2      = wxColour(192, 192, 255);
const wxColour cWarning     = wxColour(255, 128, 128);

enum {
    // Menu
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
	ID_OPTION_TRACE_DMA,
	ID_OPTION_TRACE_GIF,
	ID_OPTION_TRACE_VIF,
	ID_OPTION_TRACE_VU,
	ID_HELP,
	ID_TEXT_DEBUG,
    ID_TEXT_STATUS,
    ID_TEXT_GIF,
    ID_STATUSBAR,
    ID_NOTEBOOK1,
    ID_GRIDCODE,
    // Menu
    ID_REMOTE_VU0,
    ID_REMOTE_VU1,
    ID_REMOTE_VU0ALL,
    ID_REMOTE_VU1ALL,
    ID_REMOTE_REGS,
    ID_REMOTE_REGSVU0,
    ID_REMOTE_REGSVU1,
    ID_REMOTE_GSINIT,
    ID_REMOTE_CLR
    //
};

class VUFrame;

class VUemu : public wxApp {
public :
    virtual bool OnInit();
    // not very nice
    static inline VUFrame*  GetVuFrame() { return ms_pFrame; }

private :
    static VUFrame*         ms_pFrame;
};

static int READY = 1;
static int RESET = 2;

//---------------------------------------------------------------------------
class VUFrame: public wxFrame {
public:
        wxNotebook*     m_pRightBook;
        wxNotebook*     m_pLeftBook;
        wxToolBar*      m_pToolbar;
        wxMenuBar*      m_pMenuBar;
        wxMenu*         m_pMenuRemote;
        wxMenu*         m_pMenuFile;
        wxMenu*         m_pMenuTools;
        wxMenu*         m_pMenuOptions;
        wxMenu*         m_pMenuHelp;

        wxTextCtrl*     m_pTextDebug;
        wxTextCtrl*     m_pTextStatus;
        VuRegisterPanel*    m_pRegisterView;
        wxPanel*        m_pFlagsDetail;
        wxStatusBar*    m_pStatusBar;
        GifOutputPanel* m_pGifPanel;
        MemoryPanel*    m_pMemoryPanel;
        wxGrid*         m_pGridCode;
        Log*            m_pLog;

        Dma*        remoteDma;
        Dma*        m_pDma;
        Gif*        m_pGif;
        Vif1*       m_pVif1;
        Vif0*       m_pVif0;
        Vu*         m_pVu1;
        Parser*     m_pParser;
        int32       m_fileType;

        // KLUDGE
        // should be moved to VuRegisterPanel
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

        // Menubar file functions
        void        OnLoadCode(wxCommandEvent &WXUNUSED(event));
        void        OnLoadMem(wxCommandEvent &WXUNUSED(event));
        void        OnLoadProject(wxCommandEvent &WXUNUSED(event));
        void        OnLoadVif(wxCommandEvent &WXUNUSED(event));
        void        OnLoadDma(wxCommandEvent &WXUNUSED(event));
        void        OnSaveCode(wxCommandEvent &WXUNUSED(event));
        void        OnSaveState(wxCommandEvent &WXUNUSED(event));
		// Menubar tool functions
        void        OnReset(wxCommandEvent &WXUNUSED(event));
        void        OnRestart(wxCommandEvent &WXUNUSED(event));
        void        OnStep(wxCommandEvent &WXUNUSED(event));
        void        OnRun(wxCommandEvent &WXUNUSED(event));
        void        OnSettings(wxCommandEvent &WXUNUSED(event));
        void        OnHelp(wxCommandEvent &WXUNUSED(event));
        void        OnQuit(wxCommandEvent &WXUNUSED(event));
        // Menubar remote functions
        void        OnVu0(wxCommandEvent &WXUNUSED(event));
        void        OnVu1(wxCommandEvent &WXUNUSED(event));
        void        OnVu0All(wxCommandEvent &WXUNUSED(event));
        void        OnVu1All(wxCommandEvent &WXUNUSED(event));
        void        OnGsInit(wxCommandEvent &WXUNUSED(event));
        void        OnGetMiscRegs(wxCommandEvent &WXUNUSED(event));
        void        OnRegsVu0(wxCommandEvent &WXUNUSED(event));
        void        OnRegsVu1(wxCommandEvent &WXUNUSED(event));
        void        OnGsClear(wxCommandEvent &WXUNUSED(event));
        // Menubar options
        void        OnOptionTraceDma(wxCommandEvent &WXUNUSED(event));
        void        OnOptionTraceVif(wxCommandEvent &WXUNUSED(event));
        void        OnOptionTraceGif(wxCommandEvent &WXUNUSED(event));
        void        OnOptionTraceVu(wxCommandEvent &WXUNUSED(event));

        // Panel1 on event functions
        void        OnBreakpoint(wxGridEvent &event);

        // Panel2 on event functions
        void        FlagsUpdate(void);
        void        RegisterUpdate(void);
        void        SetSettings(void);


        VUFrame(const wxString &title, const wxPoint &pos, const wxSize
            &size);

private:
        void		BuildToolbar(void);

        void		buildCodeTable(wxNotebook *);
        void		buildFlagsPanel(wxNotebook *);
        int			LoadMemory(wxFileName file);
        void		LoadRegisters(char *);
        void		TextDebugFailed(wxString message);
        void        InstFill(void);
        // Config stuff
        Prefs*      m_pPrefs;
        int			m_charWidth;
        int			m_previous;
        // Config Load
        bool        m_autoLoadLast;
        int         autoGSExec;
        int         doProject;
        // Config Remote
        wxString    m_datTmpFile;
        wxString    m_binTmpFile;
        wxString    m_regTmpFile;
        wxString    m_gsTmpFile;
        wxString    m_dumpMemCmd;
        wxString    m_dumpRegCmd;
        wxString    m_gsExecCmd;
        wxFileName  m_codeFile;
        wxFileName  m_dataFile;
        wxFileName  m_binFile;
		wxFileName	m_memStateFile;
		wxFileName	m_regStateFile;
		wxFileName	m_mnemonicFile;
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
        void        DrawProgram();
        int         LineInstruction(int a);
        void        DrawParam(VuParam &p, char *a);
        void        InstuctionStatus();
        void        OnSelectCodeCell(wxCommandEvent &);
        // KLUDGE
        static void wrapper_DebugTic(void *objPtr, int, int);
        static void wrapper_XGKICK(void *objPtr, int offset);
        static void wrapper_DebugWarning(void *objPtr, wxString message);
        void        DebugTic(int, int);
        void        DrawGif(uint32 offset);
        void        AutoLoadLast(void);
        void        LoadVif(void);
        void        LoadDma(void);

        DECLARE_EVENT_TABLE()
};
