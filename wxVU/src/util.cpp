#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <wx/wx.h>
#include "datatypes.h"

#ifndef WIN32
char * strupr(char *);
char * strrev(char *);
char * strlwr(char *);
#endif

void
setdata(void *a, int b, int c, int bits) {
	int res=0;
	int des=1,fl=0,i;

	for(i=0; i<bits; i++)
		des*=2;

	if(b<0) fl=1;
	b=abs(b);
	res=(unsigned int)b; //clear sign bit

	res*=des; //<< bits displacement
	res+=abs(c);
	if(fl)
		res*=-1;//4294967296; //set sign bit

	memcpy(a,&res,4);
}


long iToBin(char value)
{
	char dta[100];
	int i;
	for (i = 0; i < 8; i++)
		dta[i] = (value & (1 << i)) ? '1' : '0';
	dta[i] = '\0';
	return atol(dta);
}

int round(long double a)
{
	long double b;
	b=floor(a);
	if((a-b)>0.5)
		return (int) ceil(a);
	return (int)floor(a);
}

char *strtrim(char *a)
{
	int i=0,j;
	while(a[i]) {
		if(a[i]=='\t' || a[i]=='\r' || a[i]=='\n')
			a[i]=' ';
		i++;
	}
	i=0;
	j=strlen(a)-1;
	if(j<0) return a;
	while(a[i]==' ') i++;
	while(a[j]==' ') j--;
	j++;
	a[j]=0;
	strcpy(a,a+i);
	return a;
}

long htol(char *a)
{
	char hex[]="0123456789ABCDEF";
	long val=0, desp=1;
	int i=0,j;

	strupr(a);
	while(a[i]!='X' && a[i]) i++;
	if (a[i])
		strcpy(a,a+i+1);
	i=0;
	strrev(a);
	while(a[i]) {
		j=0;
		while(a[i]!=hex[j] && hex[j]) j++;
		if(hex[j])
			val+=j*desp;
		i++;
		desp*=16;
	}
	return val;
}

#ifndef WIN32
char *
strupr(char *str) {
	char *p = str;
	for (; *p != NULL; p++) {
		*p = toupper(*p);
	}
	return str;
}

char *
strlwr(char *str) {
	char *p = str;
	for (; *p != NULL ; p++) {
		*p = tolower(*p);
	}
	return str;
}

char *
strrev(char *str) {
	char *p1, *p2;

	if (! str || ! *str) {
		return str;
	}
	for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2) {
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}
	return str;
}
#endif
