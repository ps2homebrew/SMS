/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 by Eugene Plotnikov <e-plotnikov@operamail.com>
#
# Licensed under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/
#ifndef __SMSUTILS_H
# define __SMSUTILS_H

# include <irx.h>

# define smsutils_IMPORTS_start DECLARE_IMPORT_TABLE( smsutils, 1, 1 )

extern void mips_memcpy ( void*, const void*, unsigned );
#  define I_mips_memcpy DECLARE_IMPORT( 4, mips_memcpy )

extern void mips_memset ( void*, int, unsigned );
#  define I_mips_memset DECLARE_IMPORT( 5, mips_memset )

extern int mips_vsprintf ( char*, const char*, ... );
#  define I_mips_vsprintf DECLARE_IMPORT( 6, mips_vsprintf )

extern int mips_scan ( const char*, int* );
#  define I_mips_scan DECLARE_IMPORT( 7, mips_scan )

extern void mips_ee_printf ( const char*, ... );
#  define I_mips_ee_printf DECLARE_IMPORT( 8, mips_ee_printf );

extern void mips_set_sbus_int_handler (  int, void ( * ) ( void* ), void*  );
#  define I_mips_set_sbus_int_handler DECLARE_IMPORT( 9, mips_set_sbus_int_handler );

# define smsutils_IMPORTS_end END_IMPORT_TABLE

#endif  /* __SMSUTILS_H */
