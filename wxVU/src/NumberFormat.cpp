// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include "NumberFormat.h"

wxString
NumberFormat::ToString(const int32 num, const int32 format) {
    wxString ret;
    float stuff;
    switch(format) {
        case 0:
            ret = wxString::Format("%ld", (long)num);
            break;
        case 1:
            ret = wxString::Format("%.4f", num/16.0f);
            break;
        case 2:
            ret = wxString::Format("%.12f",num/4096.0f);
            break;
        case 3:
            ret = wxString::Format("%.15f",num/32768.0f);
            break;
        case 4:
            memcpy(&stuff, &(num), 4);
            ret = wxString::Format("%f", stuff);
            break;
        case 5:
            ret = wxString::Format("0x%08x", num);
            break;
        default:
            break;
    }
    return ret;
}

wxString
NumberFormat::ToString(const float num, const int32 format) {
    wxString ret;
    switch(format) {
        case 0:
            ret = wxString::Format("%ld", *((long *)&num));
            break;
        case 1:
            ret = wxString::Format("%.4f", *((int *)&num)/16.0f);
            break;
        case 2:
            ret = wxString::Format("%.12f", *((int *)&num)/4096.0f);
            break;
        case 3:
            ret = wxString::Format("%.15f", *((int *)&num)/32768.0f);
            break;
        case 4:
            ret = wxString::Format("%f", num);
            break;
        case 5:
            ret = wxString::Format("0x%08x", *((int *)&num));
            break;
        default:
            break;
    }
    return ret;
}
