#ifndef	MISC_C
#define MISC_C

#include <tamtypes.h>
#include <kernel.h> 
#include <stdio.h>
#include <fileio.h>
#include <stdlib.h> 
#include <stdio.h>
#include <malloc.h>
#include <string.h> 
#include "../defines.h" 


void swab( const void* src1, const void* src2, int isize)
{
	char*	ptr1;
	char*	ptr2;
	char	tmp;
	int	ic1;
	
	ptr1 = (char*)src1;
	ptr2 = (char*)src2;
	for ( ic1=0 ; ic1<isize ; ic1+=2){
		tmp = ptr1[ic1+0];
		ptr2[ic1+0] = ptr1[ic1+1];
		ptr2[ic1+1] = tmp;
	}
}


#endif
