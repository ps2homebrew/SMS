#ifndef _WX_DEFSEXT_H_
#define _WX_DEFSEXT_H_

//----------------------------------------------------------------------------
// headers
//----------------------------------------------------------------------------

//! wxWindows headers
#include <wx/html/helpctrl.h>
#include <wx/print.h>
#include <wx/printdlg.h>

//============================================================================
// declarations
//============================================================================

#define PAGE_LOAD           _("Load")
#define PAGE_REMOTE         _("Remote")
#define PAGE_STYLE          _("Style")
#define PAGE_GIF            _("GIF")
#define PAGE_TRACE          _("Trace")

//! global application name
extern wxString *g_appname;

//! global help provider
extern wxHtmlHelpController *g_help;

#endif // _WX_DEFSEXT_H_
