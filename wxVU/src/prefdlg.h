#ifndef _PREFDLG_H_
#define _PREFDLG_H_

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
                   Prefs *prefs,
                   long style = 0);

    //! destructor
    ~PreferenceDlg ();

    //! event handlers
    void OnApply (wxCommandEvent& event);
    void OnCancel (wxCommandEvent& event);
    void OnOkay (wxCommandEvent& event);
    void OnReset (wxCommandEvent& event);
    void OnHelp (wxCommandEvent &event);

    void OnStyletypeChoose (wxCommandEvent &event);
    void OnKeywordsChange (wxCommandEvent &event);
    void OnGIFPRIM(wxCommandEvent &event);
    void OnGIFIIP(wxCommandEvent &event);
    void OnGIFTME(wxCommandEvent &event);
    void OnGIFFGE(wxCommandEvent &event);
    void OnGIFABE(wxCommandEvent &event);
    void OnGIFAA1(wxCommandEvent &event);
    void OnGIFFST(wxCommandEvent &event);
    void OnTextPrim(wxCommandEvent &event);
    void OnTextColor(wxCommandEvent &event);
    void updateTextPrim(void);
    void updatePrimChoices(void);

private:
    Prefs *m_prefs;

    wxString m_default_fontname;

    //! preferences pages
    wxNotebook *m_prefsBook;
    wxPanel *CreatePageLoad();
    wxPanel *CreatePageStyles();
    wxPanel *CreatePageRemote();
    wxPanel *CreatePageGIF();
    void GetValuesPageLoad();
    void SetValuesPageLoad();
    void GetValuesPageRemote();
    void SetValuesPageRemote();
    void GetValuesPageStyles();
    void SetValuesPageStyles();
    void GetValuesPageGIF();
    void SetValuesPageGIF();

    // declarations for Load
    wxRadioBox      *m_regOrder;
    wxRadioBox      *m_autoLoad;
    wxTextCtrl      *m_regStateFile;
    wxTextCtrl      *m_memStateFile;

    // declarations for PAGE_STYLE
    int             m_styleNr;
    wxComboBox      *m_styletype;
    StyleInfo       *m_curStyle;
    wxTextCtrl      *m_foreground;
    wxTextCtrl      *m_background;
    wxComboBox      *m_fontname;
    wxTextCtrl      *m_fontsize;
    wxCheckBox      *m_stylebold;
    wxCheckBox      *m_styleitalic;
    wxCheckBox      *m_styleunderl;
    wxCheckBox      *m_stylehidden;
    wxRadioButton   *m_unchanged;
    wxRadioButton   *m_uppercase;
    wxRadioButton   *m_lowercase;

    // declarations for PAGE_REMOTE
    wxRadioBox      *m_AutoGSExec;
    wxTextCtrl      *m_binTmpFile;
    wxTextCtrl      *m_datTmpFile;
    wxTextCtrl      *m_regTmpFile;
    wxTextCtrl      *m_gsTmpFile;

    // declarations for PAGE_GIF
    wxTextCtrl      *m_xOffset;
    wxTextCtrl      *m_yOffset;
    wxTextCtrl      *m_primText;
    wxTextCtrl      *m_colorText;
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

