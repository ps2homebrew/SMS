#ifndef __PREFDLG__
#define __PREFDLG__

//! wxWindows headers
#include <wx/notebook.h> // notebook support

//! application headers
#include "defsext.h"     // additional definitions
#include "prefs.h"       // preferences

//============================================================================
// declarations
//============================================================================

enum {
    ID_PREF_APPLY,
    ID_PREF_OK,
    ID_PREF_CANCEL,
    ID_PREF_HELP,
    ID_PREF_RESET,
    ID_PREF_STYLETYPE,
    // ID_PREFS_KEYWORDS,
    ID_PRIM,
    ID_PRIM_TEXT,
    ID_CLR_COLOR
};

//----------------------------------------------------------------------------
//! PreferenceDlg
class PreferenceDlg: public wxDialog {

public:
    //! constructor
    PreferenceDlg (wxWindow *parent,
                   Prefs *prefs
                   );

    //! destructor
    ~PreferenceDlg ();

    //! event handlers
    void            OnApply(wxCommandEvent& event);
    void            OnCancel(wxCommandEvent& event);
    void            OnOkay(wxCommandEvent& event);
    void            OnReset(wxCommandEvent& event);
    void            OnHelp(wxCommandEvent &event);

    void            OnStyletypeChoose (wxCommandEvent &event);
    void            OnKeywordsChange (wxCommandEvent &event);
    void            OnGIFPRIM(wxCommandEvent &event);
    void            OnTextPrim(wxCommandEvent &event);
    void            OnTextColor(wxCommandEvent &event);
    void            UpdateTextPrim(void);
    void            UpdatePrimChoices(void);

private:
    void            GetValuesPageLoad();
    void            SetValuesPageLoad();
    void            GetValuesPageRemote();
    void            SetValuesPageRemote();
    void            GetValuesPageStyles();
    void            SetValuesPageStyles();
    void            GetValuesPageGif();
    void            SetValuesPageGif();
    wxPanel*        CreatePageLoad();
    wxPanel*        CreatePageStyles();
    wxPanel*        CreatePageRemote();
    wxPanel*        CreatePageGif();

    Prefs*          m_pPrefs;

    wxString        m_default_fontname;

    //! preferences pages
    wxNotebook*     m_pPrefsBook;

    // declarations for Load
    wxRadioBox*     m_pRegOrder;
    wxRadioBox*     m_pAutoLoad;
    wxTextCtrl*     m_pRegStateFile;
    wxTextCtrl*     m_pMemStateFile;

    // declarations for PAGE_STYLE
    wxComboBox*     m_pFontName;
    wxTextCtrl*     m_pFontsize;

    // declarations for PAGE_REMOTE
    wxRadioBox*     m_pAutoGSExec;
    wxTextCtrl      *m_binTmpFile;
    wxTextCtrl      *m_datTmpFile;
    wxTextCtrl      *m_regTmpFile;
    wxTextCtrl      *m_gsTmpFile;
    wxTextCtrl      *m_memTool;
    wxTextCtrl      *m_regTool;
    wxTextCtrl      *m_gsTool;

    // declarations for PAGE_GIF
    wxTextCtrl      *m_xOffset;
    wxTextCtrl      *m_yOffset;
    wxTextCtrl      *m_primText;
    wxTextCtrl      *m_colorText;
    wxTextCtrl      *m_x1Scissor;
    wxTextCtrl      *m_y1Scissor;
    wxTextCtrl      *m_alpha;
    wxChoice        *m_prim;
    wxRadioBox      *m_tagShow;
    wxRadioBox      *m_sendPrim;
    wxRadioBox      *m_iip;
    wxRadioBox      *m_tme;
    wxRadioBox      *m_fge;
    wxRadioBox      *m_abe;
    wxRadioBox      *m_aa1;
    wxRadioBox      *m_fst;

    int             GIFPRIM;
    int             GIFIIP;
    int             GIFTME;
    int             GIFFGE;
    int             GIFABE;
    int             GIFAA1;
    int             GIFFST;
    int             PRIM;

    DECLARE_EVENT_TABLE()
};

#endif // _PREFDLG_H_

