/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
 /*YOU USE THIS PROGRAM AT YOUR OWN RISK, I CANNOT BE HELD RESPONSIBLE FOR ANY LOSS OR DAMAGE CAUSED. 

THIS PROGRAM REQUIRES THE ORIGINAL SNK CD ROMS. THESE CD ROMS ARE OWNED BY AND COPYRIGHTED BY SNK. I CANNOT BE HELD RESPONSIBLE FOR ANY REACH OF THE COPYRIGHT. 

PLEASE DO NOT ASK ME TO SUPPLY YOU CD ROMS, OR BIOS IMAGE: I CANNOT HELP YOU. 
PLEASE DO NOT SEND ME ANY CD-ROM. 
DO NOT SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST. 

NEOCD IS FREE, SOURCE CODE IS FREE. SELLING IS NOT ALLOWED. 
YOU CANNOT PROVIDE NEOCD AND NEOGEO GAME SOFTWARE ON THE SAME PHYSICAL MEDIUM. 

YOU CAN REUSE SOURCE CODE AND TECHNICAL KNOWLEDGE AS LONG AS IT IS NOT FOR COMMERCIAL PURPOSES.
LPGL */

#ifndef SOUND_H
#define SOUND_H

//#include "../neocd.h"

#define NB_SEGMENT 20

int init_audio(void);
void sound_shutdown(void);
void sound_toggle(void);

#endif
