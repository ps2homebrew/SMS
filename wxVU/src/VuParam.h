// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#ifndef __VUPARAM__
#define __VUPARAM__

class VuParam {
public:
	void    Reset();
	int     type;           //kind of parameter vf, vi, value, other
	int     index;          //if vf-vi-other index
	char    sufix[5];       //x-y-z-w-combination
	unsigned long udata;    //data value
	long    data;           //data value
	float   fdata;
	char    label[50];
	int     stalling;
	int     memdir;
};
#endif
