#include <iostream>
#include <wx/wx.h>
#include "defsext.h"     // Additional definitions
#include "prefs.h"       // Preferences
#include "prefdef.h"

//----------------------------------------------------------------------------
// Common types.
const CommonInfo g_CommonTable = {
    0,      // regOrder
    0,      // autoLoadLast;
    _T(""),
    _T("")
};
CommonInfo g_CommonPrefs = g_CommonTable;
//----------------------------------------------------------------------------
// Remote types.
const RemoteInfo g_RemoteTable = {
    false,
    _T(""),
    _T(""),
    _T(""),
    _T("")
};
RemoteInfo g_RemotePrefs = g_RemoteTable;
//----------------------------------------------------------------------------
// Style types.
const StyleInfo g_StyleTable = {
    _T(""),
    _T(""),
    _T(""),
    _T(""),
    0,
    0,
    0
};
StyleInfo g_StylePrefs = g_StyleTable;
//----------------------------------------------------------------------------
// GIF types
const GIFInfo g_GIFTable = {
    0, 
    0,
    0,
    0,
    0 
};
GIFInfo g_GIFPrefs = g_GIFTable;

//----------------------------------------------------------------------------
// Prefs
//----------------------------------------------------------------------------
Prefs::Prefs () {
    m_config = new wxConfig(wxTheApp->GetAppName());
    LoadValuesPageLoad();
    LoadValuesPageRemote();
    LoadValuesPageStyles();
    LoadValuesPageGIF();
}

Prefs::~Prefs () {
    delete m_config;
    DeleteValuesPageLoad();
    DeleteValuesPageRemote();
    DeleteValuesPageStyles();
    DeleteValuesPageGIF();
}

//----------------------------------------------------------------------------
// Load
void Prefs::LoadValuesPageLoad(bool dflt) {
    g_CommonPrefs = g_CommonTable;
    if (dflt) {
        return;
    }
    wxString key = PAGE_LOAD;
    key.Append (_T("/"));
    // AutoLoad
    if (m_config->Exists (key + AUTOLOADLAST)) {
        m_config->Read(key + AUTOLOADLAST, &g_CommonPrefs.autoLoadLast);
    }
    if (m_config->Exists (key + REGORDER)) {
        m_config->Read(key + REGORDER, &g_CommonPrefs.regOrder);
    }
    if (m_config->Exists (key + MEMSTATEFILE)) {
        wxString memStateFile;
        m_config->Read(key + MEMSTATEFILE, &memStateFile);
        g_CommonPrefs.memStateFile = strdup(memStateFile);
    }
    if (m_config->Exists (key + REGSTATEFILE)) {
        wxString regStateFile;
        m_config->Read(key + REGSTATEFILE, &regStateFile);
        g_CommonPrefs.regStateFile = strdup(regStateFile);
    }
}

void Prefs::SaveValuesPageLoad() {
    wxString key = PAGE_LOAD;
    key.Append (_T("/"));
    // AutoLoad 
    m_config->Write(key + AUTOLOADLAST, g_CommonPrefs.autoLoadLast);
    m_config->Write(key + REGORDER, g_CommonPrefs.regOrder);
    // Project
    m_config->Write(key + REGSTATEFILE, g_CommonPrefs.regStateFile);
    m_config->Write(key + MEMSTATEFILE, g_CommonPrefs.memStateFile);
    m_config->Flush();
}

void Prefs::DeleteValuesPageLoad() {
    // nothing to delete
}

//----------------------------------------------------------------------------
// Remote
void Prefs::LoadValuesPageRemote(bool dflt) {
    g_RemotePrefs = g_RemoteTable;
    if (dflt) {
        return;
    }
    wxString key = PAGE_REMOTE;
    key.Append (_T("/"));
    // Auto
    if (m_config->Exists (key + AUTOGSEXEC)) {
        m_config->Read(key + AUTOGSEXEC, &g_RemotePrefs.autoGSExec);
    }
    if (m_config->Exists (key + BINTMPFILE)) {
        wxString binTmpFile;
        m_config->Read(key + BINTMPFILE, &binTmpFile);
        g_RemotePrefs.binTmpFile = strdup(binTmpFile);
    }
    if (m_config->Exists (key + DATTMPFILE)) {
        wxString datTmpFile;
        m_config->Read(key + DATTMPFILE, &datTmpFile);
        g_RemotePrefs.datTmpFile = strdup(datTmpFile);
    }
    if (m_config->Exists (key + REGTMPFILE)) {
        wxString regTmpFile;
        m_config->Read(key + REGTMPFILE, &regTmpFile);
        g_RemotePrefs.regTmpFile = strdup(regTmpFile);
    }
    if (m_config->Exists (key + GSTMPFILE)) {
        wxString gsTmpFile;
        m_config->Read(key + GSTMPFILE, &gsTmpFile);
        g_RemotePrefs.gsTmpFile = strdup(gsTmpFile);
    }
}

void Prefs::SaveValuesPageRemote() {
    wxString key = PAGE_REMOTE;
    key.Append (_T("/"));
    // Auto
    m_config->Write(key + AUTOGSEXEC, g_RemotePrefs.autoGSExec);
    // tmp filenames for remote operations
    m_config->Write(key + BINTMPFILE, g_RemotePrefs.binTmpFile);
    m_config->Write(key + DATTMPFILE, g_RemotePrefs.datTmpFile);
    m_config->Write(key + REGTMPFILE, g_RemotePrefs.regTmpFile);
    m_config->Write(key + GSTMPFILE, g_RemotePrefs.gsTmpFile);
    m_config->Flush();
}

void Prefs::DeleteValuesPageRemote() {
}

//----------------------------------------------------------------------------
void Prefs::LoadValuesPageStyles (bool dflt) {
    g_StylePrefs = g_StyleTable;
    wxString key = PAGE_STYLE;
    key.Append (_T("/"));
    wxString fontname;
    if (m_config->Exists (key + FONTNAME)) {
        wxString fontname;
        m_config->Read (key + FONTNAME, &fontname);
        g_StylePrefs.fontname = strdup(fontname);
    }
    if (m_config->Exists (key + FONTSIZE)) {
        m_config->Read (key + FONTSIZE, &g_StylePrefs.fontsize);
    }
    if (m_config->Exists (key + FONTSTYLE)) {
        m_config->Read (key + FONTSTYLE, &g_StylePrefs.fontstyle);
    }
    if (m_config->Exists (key + LETTERCASE)) {
        m_config->Read (key + LETTERCASE, &g_StylePrefs.lettercase);
    }
}

//----------------------------------------------------------------------------
void Prefs::SaveValuesPageStyles () {
    wxString key = PAGE_STYLE;
    key.Append (_T("/"));
    m_config->Write (key + FONTNAME, g_StylePrefs.fontname);
    m_config->Write (key + FONTSIZE, g_StylePrefs.fontsize);
    m_config->Write (key + FONTSTYLE, g_StylePrefs.fontstyle);
    m_config->Write (key + LETTERCASE, g_StylePrefs.lettercase);
}

//----------------------------------------------------------------------------
void Prefs::DeleteValuesPageStyles () {
    StyleInfo *g_StylePrefs;

    // g_StylePrefs = &g_StylePrefs [typeNr];
    // if (g_StyleTable [typeNr].foreground != g_StylePrefs->foreground) {
    //     delete g_StylePrefs->foreground;
    // }
    // if (g_StyleTable [typeNr].background != g_StylePrefs->background) {
    //     delete g_StylePrefs->foreground;
    // }
    // if (g_StyleTable [typeNr].fontname != g_StylePrefs->fontname) {
    //     delete g_StylePrefs->fontname;
    // }
}

//----------------------------------------------------------------------------
// GIF
void Prefs::LoadValuesPageGIF(bool dflt) {
    g_GIFPrefs = g_GIFTable;
    if (dflt) {
        return;
    }
    wxString key = PAGE_GIF;
    key.Append (_T("/"));
    if (m_config->Exists (key + XOFFSET)) {
        m_config->Read(key + XOFFSET, &g_GIFPrefs.xOffset);
    }
    if (m_config->Exists (key + YOFFSET)) {
        m_config->Read(key + YOFFSET, &g_GIFPrefs.yOffset);
    }
    if (m_config->Exists (key + PRIM)) {
        m_config->Read(key + PRIM, &g_GIFPrefs.prim);
    }
    if (m_config->Exists (key + SENDPRIM)) {
        m_config->Read(key + SENDPRIM, &g_GIFPrefs.sendPrim);
    }
    if (m_config->Exists (key + TAGSHOW)) {
        m_config->Read(key + TAGSHOW, &g_GIFPrefs.tagShow);
    }
    if (m_config->Exists (key + CLRCOLOR)) {
        m_config->Read(key + CLRCOLOR, &g_GIFPrefs.clrcol);
    }
}

void Prefs::SaveValuesPageGIF() {
    wxString key = PAGE_GIF;
    key.Append (_T("/"));
    // Auto
    m_config->Write(key + XOFFSET, g_GIFPrefs.xOffset);
    m_config->Write(key + YOFFSET, g_GIFPrefs.yOffset);
    m_config->Write(key + PRIM, g_GIFPrefs.prim);
    m_config->Write(key + SENDPRIM, g_GIFPrefs.sendPrim);
    m_config->Write(key + TAGSHOW, g_GIFPrefs.tagShow);
    m_config->Write(key + CLRCOLOR, g_GIFPrefs.clrcol);
    m_config->Flush();
}

void Prefs::DeleteValuesPageGIF() {
}
