#include <wx/wx.h>
void setdata(void *a, int b, int c, int bits);
long iToBin(char value);
int round(long double a);
char *strtrim(char *a);
long htol(char *a);
wxString floatToString(float a, int format);
wxString intToString(int a, int format);

#ifndef WIN32
char * strupr(char *);
char * strrev(char *);
char * strlwr(char *);
#endif
