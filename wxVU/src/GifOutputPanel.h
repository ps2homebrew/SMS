// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#ifndef __GIFOUTPUTPANEL__
#define __GIFOUTPUTPANEL__

#include "datatypes.h"

class wxGrid;

class GifOutputPanel : public wxPanel {
public:
    GifOutputPanel();
    GifOutputPanel(
        wxWindow*           parent,
        wxWindowID          id,
        const wxPoint&      pos = wxDefaultPosition,
        const wxSize&       size = wxDefaultSize,
        long                style = wxTAB_TRAVERSAL
        );
    void            Write(const wxString& col1, const wxString& col2);
    void            Clear(void);
    bool            SetBackgroundColour(const wxColour& colour);
private:
    DECLARE_EVENT_TABLE()
    void            BuildGifOutputPanel(void);

    wxGrid*         m_pGifGrid;
    uint32          m_counter;
    uint32          m_charWidth;
};

enum {
    ID_GRIDGIF
};
#endif
