#ifndef __SMSUTILS_H
# define __SMSUTILS_H

# include <irx.h>

# define smsutils_IMPORTS_start DECLARE_IMPORT_TABLE( smsutils, 1, 1 )

extern void mips_memcpy ( void*, const void*, unsigned );
#  define I_mips_memcpy DECLARE_IMPORT( 4, mips_memcpy )

extern void mips_memset ( void*, int, unsigned );
#  define I_mips_memset DECLARE_IMPORT( 5, mips_memset )

# define smsutils_IMPORTS_end END_IMPORT_TABLE

#endif  /* __SMSUTILS_H */
