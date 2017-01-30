/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_EE_H
# define __SMS_EE_H

# define SMS_TLB_VALID( v ) (  ( v ) & 2  )

typedef struct SMS_EETlbEntry {
 unsigned int m_PageMask;
 unsigned int m_EntryHi;
 unsigned int m_EntryLo0;
 unsigned int m_EntryLo1;
} SMS_EETlbEntry;

struct SMS_List;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

static unsigned int inline SMS_EECp0Wired ( void ) {
 unsigned int retVal;
 __asm__ __volatile__(
  "mfc0 %0, $6\n\t"
  : "=r"( retVal )
 );
 return retVal;
}  /* end SMS_EECp0Wired */

static void inline SMS_EECp0SetWired ( unsigned int aVal ) {
 __asm__ __volatile__(
  "mtc0 %0, $6\n\t"
  "sync.p\n\t"
  :: "r"( aVal )
 );
}  /* end SMS_EECp0SetWired */

static void inline SMS_EEIntr ( int aState ) {
 __asm__(
  ".set noreorder\n\t"
  "bnel %0, $zero, 1f\n\t"
  "ei\n\t"
  "1:\n\t"
  ".set reorder\n\t"
  :: "r"( aState )
 );
}  /* end SMS_EEIntr */

void SMS_EEInit      ( void                                       );
void SMS_EExec       ( char*                                      );
void SMS_EEPort2Init ( void                                       );
void SMS_EEScanDir   ( const char*, const char*, struct SMS_List* );
void SMS_EETlbInit   ( void                                       );
void SMS_EETlbSet    ( unsigned int, SMS_EETlbEntry*              );
void SMS_EETlbGet    ( unsigned int, SMS_EETlbEntry*              );
int  SMS_EETlbAlloc  ( void                                       );
void SMS_EETlbFree   ( int                                        );
int  SMS_EEDIntr     ( void                                       );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_EE_H */
