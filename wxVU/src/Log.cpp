// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
// 
// Singleton
//

#include <iostream>
#include <map>
#include <iterator>

#include <wx/wx.h>
#include <wx/string.h>
#include "Log.h"

using namespace std;

static const wxColour cERROR	= wxColour(128, 0, 0);
static const wxColour cWARNING	= wxColour(255, 64, 64);
static const wxColour cTRACE	= wxColour(255, 128, 128);
static const wxColour cNORMAL	= wxColour(0, 0, 0);

typedef map<int32, wxString>::iterator m_mapIterator;
pair<int32, wxString>
m_map[] = {
    pair<int32, wxString>(0, ""),
    pair<int32, wxString>(E_TIMEOUT,        "Operation timed out"),
    pair<int32, wxString>(E_NO_LINK,        "No connection to ps2link server"),
    pair<int32, wxString>(E_SOCK_CLOSE,     "Connection reset by peer"),

    pair<int32, wxString>(E_FILE_OPEN,      "Unable to open file"),
    pair<int32, wxString>(E_FILE_READ,      "Unable to read from file"),
    pair<int32, wxString>(E_FILE_WRITE,     "Unable to write to file"),
    pair<int32, wxString>(E_FILE_EOF,       "EOF reached"),

    pair<int32, wxString>(E_VIF_DECODE,     "Bad VIF code"),

    // pair<int32, wxString>( ),
    // pair<int32, wxString>( ),
    // pair<int32, wxString>( ),
};

map<int32, wxString>
    object(&m_map[0], &m_map[9]);

/////////////////////////////// PUBLIC ///////////////////////////////////////
Log*    Log::_instance = 0;

Log* Log::Instance() {
    if(_instance == 0) {
        _instance = new Log;
    }
    return _instance;
}

void
Log::Error(const wxString& message) {
    if ( m_pOut == NULL ) {
        return;
    }
    m_pOut->SetDefaultStyle(wxTextAttr(cERROR));
    m_pOut->AppendText("Error: " + message);
    m_pOut->SetDefaultStyle(wxTextAttr(cNORMAL));
    // wxMessageBox(message, wxOK|wxICON_INFORMATION, m_pFrame);
    return;
}

void
Log::Error(const int32 errno) {
    if ( m_pOut == NULL ) {
        return;
    }
    m_mapIterator it = object.lower_bound(errno);
    m_pOut->SetDefaultStyle(wxTextAttr(cERROR));
    m_pOut->AppendText("Error: " + it->second + "\n");
    m_pOut->SetDefaultStyle(wxTextAttr(cNORMAL));
    // wxMessageBox(message, wxOK|wxICON_INFORMATION, m_pFrame);
    return;
}

void
Log::Error(const int32 errno, const wxString& message) {
    if ( m_pOut == NULL ) {
        return;
    }
    m_mapIterator it = object.lower_bound(errno);
    m_pOut->SetDefaultStyle(wxTextAttr(cERROR));
    m_pOut->AppendText("Error: " + it->second + " " + message + "\n");
    m_pOut->SetDefaultStyle(wxTextAttr(cNORMAL));
    // wxMessageBox(message, wxOK|wxICON_INFORMATION, m_pFrame);
}

void
Log::Warning(const wxString& message) {
    if ( m_pOut == NULL ) {
        return;
    }
    if ( !message.Cmp(m_pLastMessage->c_str()) ) {
        m_numLastMessage++;
        return;
    }
    m_pOut->SetDefaultStyle(wxTextAttr(cWARNING));
    if ( m_numLastMessage > 0 ) {
        m_pOut->AppendText(wxString::Format(
                "Last message repeated %d times\n", m_numLastMessage)
            );
    } else {
        m_pOut->AppendText("Warning: " + message);
    }
    delete m_pLastMessage;
    m_pLastMessage = new wxString(message);
    m_pOut->SetDefaultStyle(wxTextAttr(cNORMAL));
    m_numLastMessage = 0;
    return;
}

void
Log::Warning(const int32 errno) {
    if ( m_pOut == NULL ) {
        return;
    }
    m_pOut->SetDefaultStyle(wxTextAttr(cWARNING));
    // m_pOut->AppendText(warningMap<errno>);
    m_pOut->SetDefaultStyle(wxTextAttr(cNORMAL));
    return;
}

void
Log::Trace(const wxString& message) {
    if ( m_pOut == NULL ) {
        return;
    }
    m_pOut->AppendText(message);
    return;
}

void
Log::Trace(int32 level, const wxString& message) {
    if ( m_pOut == NULL ) {
        return;
    }
    switch(level) {
        case 0:
            m_pOut->AppendText(message);
            break;
        case 1:
            m_pOut->AppendText("  " + message);
            break;
        case 2:
            m_pOut->AppendText("    " + message);
            break;
        case 3:
            m_pOut->AppendText("      " + message);
            break;
        default:
            m_pOut->AppendText(message);
            break;
    }
	return;
}

void
Log::SetTextCtrl(wxTextCtrl* out) {
    m_pOut = out;
    return;
}

void
Log::SetFrame(wxFrame* frame) {
    m_pFrame = frame;
    return;
}

/////////////////////////////// PRIVATE ///////////////////////////////////////
Log::Log() {
    m_pOut = NULL;
    m_numLastMessage = 0;
    m_pLastMessage = new wxString();
}

Log::~Log() {
}
