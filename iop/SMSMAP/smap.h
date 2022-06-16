/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMAP_H
# define __SMAP_H

struct pbuf;

typedef enum SMapStatus {
 SMap_OK, SMap_Err, SMap_Con, SMap_TX
} SMapStatus;

# define ERR_OK    0
# define ERR_CONN -6
# define ERR_IF  -11

#define	SMAP_F_OPENED 0x00000001
#define	SMAP_F_LINK   0x00000002

# define SMAP_TXQUEUE_SIZE 16

# define INTR_EMAC3  0x40
# define INTR_RXEND  0x20
# define INTR_TXEND  0x10
# define INTR_RXDNV  0x08
# define INTR_TXDNV  0x04
# define INTR_BITMSK 0x7C

typedef struct smapbd {
 unsigned short m_Ctl;
 unsigned short m_Res;
 unsigned short m_Len;
 unsigned short m_Ptr;
} volatile smapbd;

typedef struct SMap {
 unsigned int   m_Flags;            /*  0 */
 unsigned short m_TXFree;           /*  4 */
 unsigned char  m_TXStartIdx;       /*  6 */
 unsigned char  m_TXEndIdx;         /*  7 */
 unsigned char  m_RXIdx;            /*  8 */
 unsigned char  m_HWAddr[ 6 ];      /*  9 */
 int            m_IIdx;             /* 16 */
 int            m_OIdx;             /* 20 */
 int            m_Event;            /* 24 */
 int            m_fWait;            /* 28 */
 int            m_Sema;             /* 32 */
 struct pbuf*   m_TXQueue[ SMAP_TXQUEUE_SIZE ];  /* 36 */
 int            m_fAuto;            /* 100 */
 int            m_Conf;             /* 104 */
} SMap;

static int inline _smap_emac3_ready ( void ) {
 int retVal;
 __asm__ __volatile__(
  ".set noat\n\t"
  "lui      $at, 0xB000\n\t"            // SMAP_EMAC3_TxMODE0
  "lw       %0, 0x2008($at)\n\t"
  "ori      $a0, $zero, 0x8000\n\t"     // E3_TX_GNP_0
  "xor      %0, %0, $a0\n\t"
  ".set at\n\t"
  : "=r"( retVal ) :: "at", "a0"
 );
 return retVal;
}  /* end _smap_emac3_ready */

static int inline _smap_check_link ( void ) {
 int retVal;
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "lui   $at, 0xB000\n\t"
  "lui   $a0, 0x1021\n\t"
  "sw    $a0, 0x205C($at)\n\t"
  "lw    $a0, 0x205C($at)\n\t"
  "lw    $a0, 0x205C($at)\n\t"
  "1:\n\t"
  "nop\n\t"
  "sll   %0, $a0, 16\n\t"
  "srl   $a0, $a0, 16\n\t"
  "andi  $a0, $a0, 0x8000\n\t"
  "bnez  $a0, 2f\n\t"
  "srl   %0, %0, 16\n\t"
  "beq   $zero, $zero, 1b\n\t"
  "lw    $a0, 0x205C($at)\n\t"
  "2:\n\t"
  "andi  %0, %0, 4\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  : "=r"( retVal ) :: "at", "a0"
 );
 return retVal;
}  /* end _smap_check_link */

void       SMap_Init        ( void                   );
void       SMap_Start       ( void                   );
SMapStatus SMap_Send        ( struct pbuf*           );
void       SMap_HandleTXInt ( void                   );
void       SMap_HandleRXInt ( void                   );
void       SMap_Reset       ( SMap*, unsigned short* );
#endif	/* __SMAP_H */
