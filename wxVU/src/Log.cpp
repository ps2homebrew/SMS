// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
// 
// Singleton
//

#include <iostream>
#include <wx/wx.h>
#include "Log.h"

static const wxColour cERROR	= wxColour(128, 0, 0);
static const wxColour cWARNING	= wxColour(255, 64, 64);
static const wxColour cTRACE	= wxColour(255, 128, 128);
static const wxColour cNORMAL	= wxColour(0, 0, 0);

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
    m_pOut->SetDefaultStyle(wxTextAttr(cERROR));
    // m_pOut->AppendText(errorMap<errno>);
    m_pOut->SetDefaultStyle(wxTextAttr(cNORMAL));
    // wxMessageBox(message, wxOK|wxICON_INFORMATION, m_pFrame);
    return;
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
