/*
 *  ps2print.c
 *  Copyright (C) 2004-2005 Olivier "Evilo" Biot (PS2 Port)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <tamtypes.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "font5200.h"
#include "../gs/hw.h"
#include "../gs/gs.h"
#include "../gs/gfxpipe.h"



/* Text printing function
   kindly took from PSMS !!!
*/
void printch(int x, int y, unsigned couleur,unsigned char ch,int taille,int pl,int zde)
{
          
	int i,j;
	unsigned char *font;
    	int rectx,recty;
    
  	//font=&msx[(int)ch * 8];
  	font=&font5200[(int)(ch-32)*8];
  	//font=&m0508fnt[(int)ch * 8];
  	
	for(i=0;i<8;i++,font++)
	{
	     for(j=0;j<8;j++)
	     {
         	if ((*font &(128>>j)))
         	{
			     rectx = x+(j<<0);
                 if(taille==1)recty = y+(i<<1);
                 else recty = y+(i<<0);
                 
                 gp_point( &thegp, rectx<<4, (recty)<<4,zde,couleur);     
	             if(pl==1)gp_point( &thegp, rectx<<4,(recty+1)<<4,zde,couleur); 
	             
            }
		  }
     }
}

void textpixel(int x,int y,unsigned color,int tail,int plein,int zdep, char *string,...)
{
   int boucle=0;  
   char	text[256];	   	
   va_list	ap;			
   
   if (string == NULL)return;		
		
   va_start(ap, string);		
      vsprintf(text, string, ap);	
   va_end(ap);	
   
   while(text[boucle]!=0){
     printch(x,y,color,text[boucle],tail,plein,zdep);
     boucle++;x+=6;
   }
	
}

void textCpixel(int x,int x2,int y,unsigned color,int tail,int plein,int zdep,char *string,...)
{
   int boucle=0;  
   char	text[256];	   	
   va_list	ap;			
   
   if (string == NULL)return;		
		
   va_start(ap, string);		
      vsprintf(text, string, ap);	
   va_end(ap);
   	
   while(text[boucle]!=0)boucle++;   
   boucle=(x2-x)/2 -(boucle*3);
   x=boucle;
   
   boucle=0;
   while(text[boucle]!=0){
     printch(x,y,color,text[boucle],tail,plein,zdep);
     boucle++;x+=6;
   }
	
}
