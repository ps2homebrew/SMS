// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#ifndef __NUMBERFORMAT__
#define __NUMBERFORMAT__
#include <wx/wx.h>
#include "datatypes.h"

class NumberFormat {
public:
    static wxString    ToString(const int32 num, const int32 format);
    static wxString    ToString(const float num, const int32 format);
private:
};

#endif
