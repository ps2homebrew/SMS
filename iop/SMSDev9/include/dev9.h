/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id: dev9.h 629 2004-10-11 00:45:00Z mrbrown $
# DEV9 Device Driver definitions and imports.
*/

#ifndef IOP_DEV9_H
# define IOP_DEV9_H

# include "types.h"
# include "irx.h"

typedef int  ( *dev9_intr_cb_t ) ( int      );
typedef void ( *dev9_dma_cb_t  ) ( int, int );

# define dev9_IMPORTS_start DECLARE_IMPORT_TABLE( dev9, 1, 1 )
# define dev9_IMPORTS_end   END_IMPORT_TABLE

void dev9RegisterIntrCb ( int, dev9_intr_cb_t );
# define I_dev9RegisterIntrCb DECLARE_IMPORT( 4, dev9RegisterIntrCb )

int dev9DmaTransfer ( int, void*, int, int );
# define I_dev9DmaTransfer DECLARE_IMPORT( 5, dev9DmaTransfer )

void dev9Shutdown ( void );
# define I_dev9Shutdown DECLARE_IMPORT( 6, dev9Shutdown )

void dev9IntrEnable ( int );
# define I_dev9IntrEnable DECLARE_IMPORT( 7, dev9IntrEnable )

void dev9IntrDisable ( int );
# define I_dev9IntrDisable DECLARE_IMPORT( 8, dev9IntrDisable )

int dev9GetEEPROM ( u16* );
# define I_dev9GetEEPROM DECLARE_IMPORT( 9, dev9GetEEPROM )

void dev9LEDCtl ( int );
# define I_dev9LEDCtl DECLARE_IMPORT( 10, dev9LEDCtl )

void dev9RegisterPreDmaCb ( int, dev9_dma_cb_t );
# define I_dev9RegisterPreDmaCb DECLARE_IMPORT( 11, dev9RegisterPreDmaCb )

void dev9RegisterPostDmaCb ( int, dev9_dma_cb_t );
# define I_dev9RegisterPostDmaCb DECLARE_IMPORT( 12, dev9RegisterPostDmaCb )

#endif /* IOP_PS2DEV9_H */
