#include <iostream>
#include <wx/wx.h>
#include <wx/accel.h>    // accelerator support
#include <wx/string.h>   // strings support
#include <wx/fontenum.h> // font support
#include <wx/choice.h>

#include "defsext.h"     // Additional definitions

#include "prefdlg.h"

using namespace std;
//----------------------------------------------------------------------------
// PreferenceDlg
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE (PreferenceDlg, wxDialog)
    EVT_BUTTON (ID_PREF_APPLY, PreferenceDlg::OnApply)
    EVT_BUTTON (ID_PREF_CANCEL, PreferenceDlg::OnCancel)
    EVT_BUTTON (ID_PREF_OK, PreferenceDlg::OnOkay)
    EVT_BUTTON (ID_PREF_RESET, PreferenceDlg::OnReset)
    // preferencews pages
    EVT_COMBOBOX(ID_PREF_STYLETYPE, PreferenceDlg::OnStyletypeChoose)
    // EVT_LISTBOX (myID_PREF_KEYWORDS,   PreferenceDlg::OnKeywordsChange)
    // Prim builder
    EVT_CHOICE(ID_PRIM, PreferenceDlg::OnGIFPRIM)
    EVT_RADIOBOX(ID_PRIM, PreferenceDlg::OnGIFPRIM)
    EVT_TEXT_ENTER(ID_PRIM_TEXT, PreferenceDlg::OnTextPrim)
    EVT_TEXT_ENTER(ID_CLR_COLOR, PreferenceDlg::OnTextColor)
END_EVENT_TABLE()

PreferenceDlg::PreferenceDlg (wxWindow *parent,
                              Prefs *prefs,
                              long style)
             : wxDialog (parent, -1, _T("Settings"),
                         wxDefaultPosition, wxDefaultSize) {

    // set prefs
    m_prefs = prefs;

    //accelerators (for help)
    const int nEntries = 1 ;
    wxAcceleratorEntry entries[nEntries];
    // entries[0].Set (wxACCEL_NORMAL, WXK_F1, ID_PREF_HELP);
    wxAcceleratorTable accel (nEntries, entries);
    SetAcceleratorTable (accel);
    GIFPRIM = 0;
    GIFIIP = 0;
    GIFTME = 0;
    GIFFGE = 0;
    GIFABE = 0;
    GIFAA1 = 0;
    GIFFST = 0;
    PRIM = 0; 
    
    // determine default fontname
    wxFont font (10, wxMODERN, wxNORMAL, wxNORMAL);
    m_default_fontname = font.GetFaceName();

    // create preference panel
    wxPanel *panel = new wxPanel (this, -1, wxDefaultPosition, wxDefaultSize,
                                 wxTAB_TRAVERSAL|wxCLIP_CHILDREN|wxNO_BORDER);

    // initialize preferences
    m_prefsBook = new wxNotebook (panel, -1, wxDefaultPosition, wxDefaultSize,
                                  wxNB_FIXEDWIDTH);
    wxNotebookSizer *prefsSizer = new wxNotebookSizer (m_prefsBook);
    m_prefsBook->AddPage(CreatePageLoad(), PAGE_LOAD, true);
    SetValuesPageLoad();
    m_prefsBook->AddPage (CreatePageRemote(), PAGE_REMOTE, true);
    SetValuesPageRemote();
    m_prefsBook->AddPage(CreatePageStyles(), PAGE_STYLE, true);
    SetValuesPageStyles();
    m_prefsBook->AddPage(CreatePageGIF(), PAGE_GIF, true);
    SetValuesPageGIF();
    m_prefsBook->SetSelection (0);

    // lauout the buttons
    wxButton *resetButton = new wxButton (panel, ID_PREF_RESET, _("Default"));
    wxButton *helpButton = new wxButton (panel, ID_PREF_HELP, _("Help"));
    wxButton *okButton = new wxButton (panel, ID_PREF_OK, _("OK"));
    okButton->SetDefault();
    wxButton *cancelButton = new wxButton (panel, ID_PREF_CANCEL, _("Cancel"));
    wxButton *applyButton = new wxButton (panel, ID_PREF_APPLY, _("Apply"));
    wxBoxSizer *buttonpane = new wxBoxSizer (wxHORIZONTAL);
    buttonpane->Add(resetButton, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    buttonpane->Add(6, 0);
    buttonpane->Add(helpButton, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    buttonpane->Add(16, 0, 1);
    buttonpane->Add(okButton, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
    buttonpane->Add(6, 0);
    buttonpane->Add(cancelButton, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
    buttonpane->Add(6, 0);
    buttonpane->Add(applyButton, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);

    // set sizer
    wxBoxSizer *panelsizer = new wxBoxSizer (wxVERTICAL);
    panelsizer->Add(prefsSizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10);
    panelsizer->Add(0, 6);
    panelsizer->Add(buttonpane, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);

    panel->SetSizer(panelsizer);
    panelsizer->Fit(this);
    Centre(wxBOTH);
    ShowModal();
}

PreferenceDlg::~PreferenceDlg () {
}

//----------------------------------------------------------------------------
// event handlers
void PreferenceDlg::OnApply (wxCommandEvent &WXUNUSED(event)) {
    GetValuesPageLoad();
    m_prefs->SaveValuesPageLoad();
    GetValuesPageRemote();
    m_prefs->SaveValuesPageRemote();
    GetValuesPageStyles();
    m_prefs->SaveValuesPageStyles();
    GetValuesPageGIF();
    m_prefs->SaveValuesPageGIF();
}

void PreferenceDlg::OnCancel (wxCommandEvent &WXUNUSED(event)) {
    EndModal(ID_PREF_CANCEL);
}

void PreferenceDlg::OnHelp (wxCommandEvent &WXUNUSED(event)) {
    // g_help->Display(_T("preferences.html"));
}

void PreferenceDlg::OnOkay (wxCommandEvent &WXUNUSED(event)) {
    GetValuesPageLoad();
    m_prefs->SaveValuesPageLoad();
    GetValuesPageRemote();
    m_prefs->SaveValuesPageRemote();
    GetValuesPageStyles();
    m_prefs->SaveValuesPageStyles();
    GetValuesPageGIF();
    m_prefs->SaveValuesPageGIF();
    EndModal(ID_PREF_OK);
}

void PreferenceDlg::OnReset (wxCommandEvent &WXUNUSED(event)) {
    int pageNr = m_prefsBook->GetSelection ();
    if (pageNr == 0) {
        m_prefs->LoadValuesPageLoad(true);
        SetValuesPageLoad();
    }else if (pageNr == 1) {
        // m_prefs->LoadValuesPageLanguages (true);
        // SetValuesPageLanguages ();
    }else if (pageNr == 2) {
        m_prefs->LoadValuesPageStyles (true);
        SetValuesPageStyles();
    }
}

void PreferenceDlg::OnStyletypeChoose (wxCommandEvent &WXUNUSED(event)) {
    // GetValuesPageStyles ();
    // // m_styleNr = m_styletype->GetSelection();
    // m_curStyle = &g_StylePrefs [m_styleNr];
    // SetValuesPageStyles ();
}

void PreferenceDlg::OnKeywordsChange (wxCommandEvent &WXUNUSED(event)) {
    // m_lgStyleNr = m_styles->GetSelection();
    // if (m_curLanguage->styles[m_lgStyleNr].words) {
    //     m_words->SetValue (m_curLanguage->styles[m_lgStyleNr].words);
    // }else{
    //     m_words->SetValue (wxEmptyString);
    // }
}

void
PreferenceDlg::OnGIFPRIM(wxCommandEvent &WXUNUSED(event)) {
    GIFPRIM = m_prim->GetSelection();
    GIFIIP = m_iip->GetSelection();
    GIFTME = m_tme->GetSelection();
    GIFFGE = m_fge->GetSelection();
    GIFABE = m_abe->GetSelection();
    GIFAA1 = m_aa1->GetSelection();
    GIFFST = m_fst->GetSelection();
    updateTextPrim();
}
void
PreferenceDlg::updateTextPrim(void) {
    PRIM = (GIFFST<<8)+(GIFAA1<<7)+(GIFABE<<6)+(GIFFGE<<5)+
        (GIFTME<<4)+(GIFIIP<<3)+GIFPRIM;
    m_primText->SetValue(wxString::Format("0x%x", PRIM));
}
void
PreferenceDlg::OnTextPrim(wxCommandEvent &WXUNUSED(event)) {
    PRIM = strtol(m_primText->GetValue().c_str(), (char **)NULL, 0);
    updatePrimChoices();
}
void
PreferenceDlg::OnTextColor(wxCommandEvent &WXUNUSED(event)) {
    g_GIFPrefs.clrcol = strtol(m_primText->GetValue().c_str(), (char **)NULL, 0);
}
void
PreferenceDlg::updatePrimChoices(void) {
    GIFFST = (PRIM>>8)&0x1;
    GIFAA1 = (PRIM>>7)&0x1;
    GIFABE = (PRIM>>6)&0x1;
    GIFFGE = (PRIM>>5)&0x1;
    GIFTME = (PRIM>>4)&0x1;
    GIFIIP = (PRIM>>3)&0x1;
    GIFPRIM = PRIM&0x7;
    m_prim->SetSelection(GIFPRIM);
    m_iip->SetSelection(GIFIIP);
    m_tme->SetSelection(GIFTME);
    m_fge->SetSelection(GIFFGE);
    m_abe->SetSelection(GIFABE);
    m_aa1->SetSelection(GIFAA1);
    m_fst->SetSelection(GIFFST);
}

//----------------------------------------------------------------------------
// Load panel related functions
wxPanel *PreferenceDlg::CreatePageLoad() {
    wxPanel *panel = new wxPanel (m_prefsBook);

    const int ch = GetCharWidth();
    const int sz1 = 20*ch; // width of a text lable

    // GIF/GS preferences
    wxString choices3[2] = {"Row", "Column"};
    m_regOrder = new wxRadioBox(panel, -1, "Register layout order",
            wxDefaultPosition, wxDefaultSize, 2, choices3, 2, wxRA_SPECIFY_ROWS);

    wxString cAutoLoad[2] = {"Yes", "No"};
    m_autoLoad = new wxRadioBox(panel, -1, "Autoload latest used files on startup",
        wxDefaultPosition, wxDefaultSize, 2, cAutoLoad, 2, wxRA_SPECIFY_ROWS);

    // mem state temp file.
    wxStaticBoxSizer *stateFileBox = new wxStaticBoxSizer (
                     new wxStaticBox (panel, -1, _("Filenames for remote saves.")),
                     wxVERTICAL);
    m_memStateFile = new wxTextCtrl (panel, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize(sz1,-1));
    wxBoxSizer *memStateFile = new wxBoxSizer (wxHORIZONTAL);
    memStateFile->Add (4, 0);
    memStateFile->Add (new wxStaticText (panel, -1, _("File for saving memory state"),
                                       wxDefaultPosition, wxSize(sz1, -1)),
                     0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    memStateFile->Add (6, 0);
    memStateFile->Add (m_memStateFile, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    memStateFile->Add (4, 0);
    stateFileBox->Add(memStateFile, 0, wxALIGN_LEFT|wxEXPAND);
    
    // reg state temp file.
    m_regStateFile = new wxTextCtrl (panel, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize(sz1,-1));
    wxBoxSizer *regStateFile = new wxBoxSizer (wxHORIZONTAL);
    regStateFile->Add (4, 0);
    regStateFile->Add (new wxStaticText (panel, -1, _("File for saving register state"),
                                       wxDefaultPosition, wxSize(sz1, -1)),
                     0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    regStateFile->Add (6, 0);
    regStateFile->Add (m_regStateFile, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    regStateFile->Add (4, 0);
    stateFileBox->Add(regStateFile, 0, wxALIGN_LEFT|wxEXPAND);

    // layout panel
    wxBoxSizer *panelpane = new wxBoxSizer(wxVERTICAL);
    panelpane->Add(m_regOrder, 0, wxEXPAND|wxALL, 5);
    panelpane->Add(m_autoLoad, 0, wxEXPAND|wxALL, 5);
    panelpane->Add(stateFileBox, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    // set and return panel
    panel->SetSizer(panelpane);
    return panel;
}

//----------------------------------------------------------------------------
void PreferenceDlg::GetValuesPageLoad() {
    // autoload
    g_CommonPrefs.autoLoadLast = m_autoLoad->GetSelection();
    g_CommonPrefs.regOrder = m_regOrder->GetSelection();
    // project
    g_CommonPrefs.regStateFile = strdup(m_regStateFile->GetValue());
    g_CommonPrefs.memStateFile = strdup(m_memStateFile->GetValue());
}

//----------------------------------------------------------------------------
void PreferenceDlg::SetValuesPageLoad() {
    m_autoLoad->SetSelection(g_CommonPrefs.autoLoadLast);
    m_regOrder->SetSelection(g_CommonPrefs.regOrder);
    m_regStateFile->SetValue(g_CommonPrefs.regStateFile);
    m_memStateFile->SetValue(g_CommonPrefs.memStateFile);
}

//----------------------------------------------------------------------------
// Remote panel functions
wxPanel *PreferenceDlg::CreatePageRemote() {
    wxPanel *panel = new wxPanel (m_prefsBook);
    const int ch = GetCharWidth();
    const int sz1 = 20*ch; // width of a text lable
    
    // toggle for Automatic gsexec on xgkick
    wxString cAutoGSexec[2] = {"Yes", "No"};
    m_AutoGSExec = new wxRadioBox(panel, -1, "Automatic gsexec on xgkick",
        wxDefaultPosition, wxDefaultSize, 2, cAutoGSexec, 2, wxRA_SPECIFY_ROWS);

    // temp register filename to use for dumping from ps2
    wxStaticBoxSizer *tmpFileBox = new wxStaticBoxSizer (
                     new wxStaticBox (panel, -1, _("Filenames for remote saves.")),
                     wxVERTICAL);
    m_binTmpFile = new wxTextCtrl (panel, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize(sz1,-1));
    wxBoxSizer *binTmpFile = new wxBoxSizer (wxHORIZONTAL);
    binTmpFile->Add (4, 0);
    binTmpFile->Add (new wxStaticText (panel, -1, _("Temp file for VU code dump"),
                                       wxDefaultPosition, wxSize(sz1, -1)),
                     0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    binTmpFile->Add (6, 0);
    binTmpFile->Add (m_binTmpFile, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    binTmpFile->Add (4, 0);
    tmpFileBox->Add(binTmpFile, 0, wxALIGN_LEFT|wxEXPAND);

    // dat file tmp
    m_datTmpFile = new wxTextCtrl (panel, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize(sz1,-1));
    wxBoxSizer *datTmpFile = new wxBoxSizer (wxHORIZONTAL);
    datTmpFile->Add (4, 0);
    datTmpFile->Add (new wxStaticText (panel, -1, _("Temp file for VU data dump"),
                                       wxDefaultPosition, wxSize(sz1, -1)),
                     0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    datTmpFile->Add (6, 0);
    datTmpFile->Add (m_datTmpFile, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    datTmpFile->Add (4, 0);
    tmpFileBox->Add(datTmpFile, 0, wxALIGN_LEFT|wxEXPAND);
    // reg tmp file
    m_regTmpFile = new wxTextCtrl (panel, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize(sz1,-1));
    wxBoxSizer *regTmpFile = new wxBoxSizer (wxHORIZONTAL);
    regTmpFile->Add (4, 0);
    regTmpFile->Add (new wxStaticText (panel, -1, _("Temp file for register dump"),
                                       wxDefaultPosition, wxSize(sz1, -1)),
                     0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    regTmpFile->Add (6, 0);
    regTmpFile->Add (m_regTmpFile, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    regTmpFile->Add (4, 0);
    tmpFileBox->Add(regTmpFile, 0, wxALIGN_LEFT|wxEXPAND);

    // GS temp file.
    m_gsTmpFile = new wxTextCtrl (panel, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize(sz1,-1));
    wxBoxSizer *gsTmpFile = new wxBoxSizer (wxHORIZONTAL);
    gsTmpFile->Add (4, 0);
    gsTmpFile->Add (new wxStaticText (panel, -1, _("Temp file for GIF packet"),
                                       wxDefaultPosition, wxSize(sz1, -1)),
                     0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    gsTmpFile->Add (6, 0);
    gsTmpFile->Add (m_gsTmpFile, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    gsTmpFile->Add (4, 0);
    tmpFileBox->Add(gsTmpFile, 0, wxALIGN_LEFT|wxEXPAND);
      // 
    wxBoxSizer *panelpane = new wxBoxSizer (wxVERTICAL);
    panelpane->Add(m_AutoGSExec, 0, wxALIGN_LEFT|wxEXPAND, 10);
    panelpane->Add(tmpFileBox, 0, wxALIGN_LEFT|wxEXPAND, 10);
    panel->SetSizer(panelpane);
    return panel;
}

//----------------------------------------------------------------------------
void PreferenceDlg::GetValuesPageRemote() {
    g_RemotePrefs.autoGSExec = m_AutoGSExec->GetSelection();
    g_RemotePrefs.binTmpFile = strdup(m_binTmpFile->GetValue());
    g_RemotePrefs.datTmpFile = strdup(m_datTmpFile->GetValue());
    g_RemotePrefs.regTmpFile = strdup(m_regTmpFile->GetValue());
    g_RemotePrefs.gsTmpFile = strdup(m_gsTmpFile->GetValue());
}

//----------------------------------------------------------------------------
void PreferenceDlg::SetValuesPageRemote() {
    m_AutoGSExec->SetSelection(g_RemotePrefs.autoGSExec);
    m_binTmpFile->SetValue(g_RemotePrefs.binTmpFile);
    m_datTmpFile->SetValue(g_RemotePrefs.datTmpFile);
    m_regTmpFile->SetValue(g_RemotePrefs.regTmpFile);
    m_gsTmpFile->SetValue(g_RemotePrefs.gsTmpFile);
}

//----------------------------------------------------------------------------
wxPanel *PreferenceDlg::CreatePageStyles () {
    wxPanel *panel = new wxPanel (m_prefsBook);

    const int ch = GetCharWidth();
    const int sz1 = 20*ch; // width of a text lable

    // foreground prefs
    m_foreground = new wxTextCtrl (panel, -1, wxEmptyString,
                                   wxDefaultPosition, wxSize(sz1,-1));
    wxBoxSizer *foreground = new wxBoxSizer (wxHORIZONTAL);
    foreground->Add (4, 0);
    foreground->Add (new wxStaticText (panel, -1, _("Foreground"),
                                       wxDefaultPosition, wxSize(sz1, -1)),
                     0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    foreground->Add (6, 0);
    foreground->Add (m_foreground, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    foreground->Add (4, 0);

    // background prefs
    // m_background = new wxTextCtrl (panel, -1, wxEmptyString,
    //                                wxDefaultPosition, wxSize(sz1,-1));
    // wxBoxSizer *background = new wxBoxSizer (wxHORIZONTAL);
    // background->Add(4, 0);
    // background->Add(new wxStaticText (panel, -1, _("Background"),
    //                                    wxDefaultPosition, wxSize(sz1, -1)),
    //                  0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    // background->Add(6, 0);
    // background->Add(m_background, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    // background->Add(4, 0);

    // font name prefs
    wxFontEnumerator fontEnum;
    fontEnum.EnumerateFacenames(wxFONTENCODING_SYSTEM, true);
    wxArrayString *fontNames = fontEnum.GetFacenames();
    fontNames->Sort();
    m_fontname = new wxComboBox (panel, -1, wxEmptyString,
                                 wxDefaultPosition, wxSize(sz1, -1),
                                 0);
    int i;
    for (i = 0; i < (int)fontNames->GetCount(); i++) {
        m_fontname->Append (fontNames->Item (i));
    }
    wxBoxSizer *fontname = new wxBoxSizer (wxHORIZONTAL);
    fontname->Add(4, 0);
    fontname->Add(new wxStaticText (panel, -1, _("Font name"),
                                     wxDefaultPosition, wxSize(sz1, -1)),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    fontname->Add(6, 0);
    fontname->Add(m_fontname, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    fontname->Add(4, 0);

    // font size prefs
    m_fontsize = new wxTextCtrl (panel, -1, wxEmptyString,
                                 wxDefaultPosition, wxSize(3*ch,-1));
    wxBoxSizer *fontsize = new wxBoxSizer (wxHORIZONTAL);
    fontsize->Add(4, 0);
    fontsize->Add(new wxStaticText (panel, -1, _("Font size"),
                                      wxDefaultPosition, wxSize(sz1, -1)),
                    0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    fontsize->Add(6, 0);
    fontsize->Add(m_fontsize, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    fontsize->Add(4, 0);

    // font style prefs
    m_stylebold = new wxCheckBox(panel, -1, _("Bold"),
                                  wxDefaultPosition, wxSize(12*ch, -1));
    m_styleitalic = new wxCheckBox(panel, -1, _("Italic"),
                                    wxDefaultPosition, wxSize(12*ch, -1));
    m_styleunderl = new wxCheckBox(panel, -1, _("Underline"),
                                       wxDefaultPosition, wxSize(12*ch, -1));
    m_stylehidden = new wxCheckBox(panel, -1, _("Hidden"),
                                    wxDefaultPosition, wxSize(12*ch, -1));
    wxBoxSizer *fontstyle = new wxBoxSizer (wxHORIZONTAL);
    fontstyle->Add(4, 0);
    fontstyle->Add(new wxStaticText (panel, -1, _("Font style"),
                                      wxDefaultPosition, wxSize(sz1, -1)),
                    0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    fontstyle->Add(6, 0);
    fontstyle->Add(m_stylebold, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    fontstyle->Add(m_styleitalic, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    fontstyle->Add(m_styleunderl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    fontstyle->Add(m_stylehidden, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    fontstyle->Add(4, 0);

    // font case prefs
    m_unchanged = new wxRadioButton (panel, -1, _T("Normal"),
                                     wxDefaultPosition, wxSize(12*ch, -1), wxRB_GROUP);
    m_uppercase = new wxRadioButton (panel, -1, _T("Uppercase"),
                                     wxDefaultPosition, wxSize(12*ch, -1));
    m_lowercase = new wxRadioButton (panel, -1, _T("Lowercase"),
                                     wxDefaultPosition, wxSize(12*ch, -1));
    wxBoxSizer *lettercase = new wxBoxSizer (wxHORIZONTAL);
    lettercase->Add (4, 0);
    lettercase->Add (new wxStaticText (panel, -1, _("Letter case"),
                                       wxDefaultPosition, wxSize(sz1, -1)),
                     0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    lettercase->Add (6, 0);
    lettercase->Add (m_unchanged, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    lettercase->Add (m_uppercase, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    lettercase->Add (m_lowercase, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    lettercase->Add (4, 0);

    // layout the style types prefs
    wxStaticBoxSizer *prefspane = new wxStaticBoxSizer (
                     new wxStaticBox (panel, -1, _("Style types settings")),
                     wxVERTICAL);
    // prefspane->Add (foreground, 0, wxEXPAND);
    // prefspane->Add (0, 6);
    // prefspane->Add (background, 0, wxEXPAND);
    // prefspane->Add (0, 6);
    prefspane->Add (fontname, 0, wxEXPAND);
    prefspane->Add (0, 6);
    prefspane->Add (fontsize, 0, wxEXPAND);
    prefspane->Add (0, 6);
    prefspane->Add (fontstyle, 0, wxEXPAND);
    prefspane->Add (0, 6);
    prefspane->Add (lettercase, 0, wxEXPAND);
    prefspane->Add (0, 4);

    // layout panel
    wxBoxSizer *panelpane = new wxBoxSizer (wxVERTICAL);
    panelpane->Add (prefspane, 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 10);

    // set and return panel
    panel->SetSizer (panelpane);
    return panel;
}

//----------------------------------------------------------------------------
void PreferenceDlg::GetValuesPageStyles () {
    long fontsize = 0;
    int fontstyle = 0;
    int lettercase = 0;
    // g_StylePrefs.foreground = strdup (m_foreground->GetValue());
    // g_StylePrefs.background = strdup (m_background->GetValue());
    g_StylePrefs.fontname = strdup (m_fontname->GetValue());
    m_fontsize->GetValue().ToLong (&fontsize);
    g_StylePrefs.fontsize = fontsize;
    g_StylePrefs.fontstyle = fontstyle;
    g_StylePrefs.lettercase = lettercase;
}

//----------------------------------------------------------------------------
void PreferenceDlg::SetValuesPageStyles () {
    if ( strcmp(g_StylePrefs.fontname, _T("")) != 0 ) {
        m_fontname->SetValue(g_StylePrefs.fontname);
    } else {
        m_fontname->SetValue(m_default_fontname);
    }
    // m_foreground->SetValue(g_StylePrefs.foreground);
    // m_background->SetValue(g_StylePrefs.background);
    m_fontsize->SetValue(wxString::Format (_T("%d"), g_StylePrefs.fontsize));
}

//----------------------------------------------------------------------------
// GIF panel functions
wxPanel *PreferenceDlg::CreatePageGIF() {
    wxPanel *panel = new wxPanel (m_prefsBook);
    const int ch = GetCharWidth();
    // width of a text lable
    const int sz1 = 20*ch;

    m_xOffset = new wxTextCtrl(panel, -1, "2048");
    m_yOffset = new wxTextCtrl(panel, -1, "2048");
    wxBoxSizer *oXffset = new wxBoxSizer (wxHORIZONTAL);
    oXffset->Add (4, 0);
    oXffset->Add (new wxStaticText (panel, -1, _("X offset"),
                                     wxDefaultPosition, wxSize(sz1, -1)),
                   0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    oXffset->Add (6, 0);
    oXffset->Add (m_xOffset, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);

    wxBoxSizer *oYffset = new wxBoxSizer (wxHORIZONTAL);
    oYffset->Add (4, 0);
    oYffset->Add (new wxStaticText (panel, -1, _("Y offset"),
                                        wxDefaultPosition, wxSize(sz1, -1)),
                      0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    oYffset->Add (6, 0);
    oYffset->Add (m_yOffset, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);

    wxStaticBoxSizer *gifbox = new wxStaticBoxSizer (
                     new wxStaticBox (panel, -1, _("GIF/GS Preferences")),
                     wxVERTICAL);
    gifbox->Add (oXffset, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    gifbox->Add (0, 6);
    gifbox->Add (oYffset, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    gifbox->Add (0, 4);

    wxString cSendPrim[2] = {"Yes", "No"};
    m_sendPrim = new wxRadioBox(panel, -1, "Always send PRIM before xgkick",
        wxDefaultPosition, wxDefaultSize, 2, cSendPrim, 2, wxRA_SPECIFY_ROWS);

    // Prim textual representation
    wxBoxSizer *primText = new wxBoxSizer (wxHORIZONTAL);
    m_primText = new wxTextCtrl(panel, ID_PRIM_TEXT, wxEmptyString,
                               wxDefaultPosition, wxSize(sz1,-1));
    primText->Add (4, 0);
    primText->Add (new wxStaticText (panel, -1, _("GIF PRIM:"),
                                       wxDefaultPosition, wxSize(sz1, -1)),
                     0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    primText->Add (6, 0);
    primText->Add (m_primText, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    primText->Add (4, 0);

    wxString choices[7] = {"Point", "Line", "Line Strip", "Triangle",
        "Triangle Strip", "Triangle Fan", "Sprite"};
    m_prim = new wxChoice(panel, ID_PRIM, wxDefaultPosition,
        wxDefaultSize, 7, choices);
    wxString cIIP[2] = {"Flat", "Gouraud"};
    m_iip = new wxRadioBox(panel, ID_PRIM, "Shading method",
        wxDefaultPosition, wxDefaultSize, 2, cIIP, 2,
        wxRA_SPECIFY_ROWS);
    wxString cOnOff[2] = {"Off", "On"};
    m_tme = new wxRadioBox(panel, ID_PRIM, "Texture mapping",
        wxDefaultPosition, wxDefaultSize, 2, cOnOff, 2,
        wxRA_SPECIFY_ROWS);
    m_fge = new wxRadioBox(panel, ID_PRIM, "Fogging",
        wxDefaultPosition, wxDefaultSize, 2, cOnOff, 2,
        wxRA_SPECIFY_ROWS);
    m_abe = new wxRadioBox(panel, ID_PRIM, "Alpha blending",
        wxDefaultPosition, wxDefaultSize, 2, cOnOff, 2,
        wxRA_SPECIFY_ROWS);
    m_aa1 = new wxRadioBox(panel, ID_PRIM, "Antialiasing",
        wxDefaultPosition, wxDefaultSize, 2, cOnOff, 2,
        wxRA_SPECIFY_ROWS);
    m_fst = new wxRadioBox(panel, ID_PRIM, "Texture coordinates",
        wxDefaultPosition, wxDefaultSize, 2, cOnOff, 2,
        wxRA_SPECIFY_ROWS);

    wxStaticBoxSizer *primbox = new wxStaticBoxSizer (
                     new wxStaticBox (panel, -1, _("Prim register settings")),
                     wxVERTICAL);
    primbox->Add(primText, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    primbox->Add(m_sendPrim, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    primbox->Add(m_prim, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    primbox->Add(m_iip, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    primbox->Add(m_tme, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    primbox->Add(m_fge, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    primbox->Add(m_abe, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    primbox->Add(m_aa1, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    primbox->Add(m_fst, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);

    wxString cTagRecog[2] = {"Yes", "No"};
    m_tagShow = new wxRadioBox(panel, -1, "Color giftags in memory tab.",
        wxDefaultPosition, wxDefaultSize, 2, cTagRecog, 2, wxRA_SPECIFY_ROWS);

    // colorbox sizer 
    wxBoxSizer *colorText = new wxBoxSizer (wxHORIZONTAL);
    m_colorText = new wxTextCtrl(panel, ID_PRIM_TEXT, wxEmptyString,
                               wxDefaultPosition, wxSize(sz1,-1));
    colorText->Add (4, 0);
    colorText->Add (new wxStaticText (panel, -1, _("Fill screen with:"),
                                       wxDefaultPosition, wxSize(sz1, -1)),
                     0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    colorText->Add (6, 0);
    colorText->Add (m_colorText, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    colorText->Add (4, 0);
    
    // panelpane->Add(colorBox, 0, wxALIGN_LEFT|wxEXPAND, 10);
    wxBoxSizer *panelpane = new wxBoxSizer (wxVERTICAL);
    panelpane->Add(m_tagShow, 0, wxALIGN_LEFT|wxEXPAND, 10);
    panelpane->Add(gifbox, 0, wxALIGN_LEFT|wxEXPAND, 10);
    panelpane->Add(primbox, 0, wxALIGN_LEFT|wxEXPAND, 10);
    panelpane->Add(colorText, 0, wxALIGN_LEFT|wxEXPAND, 10);

    panel->SetSizer(panelpane);
    return panel;
}

//----------------------------------------------------------------------------
void PreferenceDlg::GetValuesPageGIF() {
    g_GIFPrefs.xOffset = atoi(m_xOffset->GetValue().c_str());
    g_GIFPrefs.yOffset = atoi(m_yOffset->GetValue().c_str());
    g_GIFPrefs.prim = strtol(m_primText->GetValue().c_str(), (char **)NULL, 0);
    g_GIFPrefs.sendPrim = m_sendPrim->GetSelection();
    g_GIFPrefs.tagShow = m_tagShow->GetSelection();
    g_GIFPrefs.clrcol = strtol(m_colorText->GetValue().c_str(), (char **)NULL, 0);
}

//----------------------------------------------------------------------------
void PreferenceDlg::SetValuesPageGIF() {
    m_xOffset->SetValue(wxString::Format("%d", g_GIFPrefs.xOffset));
    m_yOffset->SetValue(wxString::Format("%d", g_GIFPrefs.yOffset));
    m_primText->SetValue(wxString::Format("0x%x", g_GIFPrefs.prim));
    m_colorText->SetValue(wxString::Format("0x%x", g_GIFPrefs.clrcol));
    m_sendPrim->SetSelection(g_GIFPrefs.sendPrim);
    m_tagShow->SetSelection(g_GIFPrefs.tagShow);
    PRIM = g_GIFPrefs.prim;
    updatePrimChoices();
}
