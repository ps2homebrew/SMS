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
#include <dev9.h>
#include <ps2ip.h>

#include "smap.h"
#include "../SMSUTILS/smsutils.h"

#define	SMAP_TXMAXSIZE    ( 6 + 6 + 2 + 1500 )
#define	SMAP_TXMAXTAILPAD 4

#define	SMAP_BD_MAX_ENTRY 64
#define	SMAP_BD_NEXT( x ) ( x ) =(  ( x ) + 1  ) & ( SMAP_BD_MAX_ENTRY - 1  )

#define	SMAP_BD_TX_READY  ( 1 << 15 )
#define	SMAP_BD_TX_GENFCS ( 1 <<  9 )
#define	SMAP_BD_TX_GENPAD ( 1 <<  8 )
#define	SMAP_BD_RX_EMPTY  ( 1 << 15 )

#define	SMAP_BD_RX_OVERRUN    ( 1 << 9 )
#define	SMAP_BD_RX_PFRM       ( 1 << 8 )
#define	SMAP_BD_RX_BADFRM     ( 1 << 7 )
#define	SMAP_BD_RX_RUNTFRM    ( 1 << 6 )
#define	SMAP_BD_RX_SHORTEVNT  ( 1 << 5 )
#define	SMAP_BD_RX_ALIGNERR   ( 1 << 4 )
#define	SMAP_BD_RX_BADFCS     ( 1 << 3 )
#define	SMAP_BD_RX_FRMTOOLONG ( 1 << 2 )
#define	SMAP_BD_RX_OUTRANGE   ( 1 << 1 )
#define	SMAP_BD_RX_INRANGE    ( 1 << 0 )
#define	SMAP_BD_RX_ERRMASK    ( SMAP_BD_RX_OVERRUN   | SMAP_BD_RX_PFRM       |\
                                SMAP_BD_RX_BADFRM    | SMAP_BD_RX_RUNTFRM    |\
                                SMAP_BD_RX_SHORTEVNT | SMAP_BD_RX_ALIGNERR   |\
                                SMAP_BD_RX_BADFCS    | SMAP_BD_RX_FRMTOOLONG |\
                                SMAP_BD_RX_OUTRANGE  | SMAP_BD_RX_INRANGE     \
                              )

extern SMap         g_SMAP;
extern struct netif g_NIF;

extern void SMAP_PreDMACb  ( int, int );
extern void SMAP_PostDMACb ( int, int );

extern void _smap_phy_write ( int, int );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_smap_phy_write:\n\t"
 "addiu $sp, $sp, -12\n\t"
 "andi  $a0, $a0, 0x1F\n\t"
 "sw    $ra, 0($sp)\n\t"
 "sw    $s0, 4($sp)\n\t"
 "lui   $s0, 0xB000\n\t"
 "andi  $a1, $a1, 0xFFFF\n\t"
 "ori   $a0, $a0, 0x2020\n\t"
 "ori   $s0, $s0, 0x205C\n\t"
 "sll   $a0, $a0, 16\n\t"
 "sw    $s1, 8($sp)\n\t"
 "or    $a0, $a0, $a1\n\t"
 "addiu $s1, $zero, 10000\n\t"
 "sw    $a0, 0($s0)\n\t"
 "lw    $a0, 0($s0)\n\t"
 "lw    $a0, 0($s0)\n\t"
 "1:\n\t"
 "nop\n\t"
 "srl   $a0, $a0, 16\n\t"
 "andi  $a0, $a0, 0x8000\n\t"
 "bnez  $a0, 2f\n\t"
 "addiu $s1, $s1, -1\n\t"
 "jal   DelayThread\n\t"
 "addiu $a0, $zero, 10\n\t"
 "bgtz  $s1, 1b\n\t"
 "lw    $a0, 0($s0)\n\t"
 "2:\n\t"
 "lw    $ra, 0($sp)\n\t"
 "lw    $s0, 4($sp)\n\t"
 "lw    $s1, 8($sp)\n\t"
 "jr    $ra\n\t"
 "addiu $sp, $sp, 12\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

extern int _smap_phy_read ( int );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_smap_phy_read:\n\t"
 "addiu $sp, $sp, -12\n\t"
 "andi  $a0, $a0, 0x1F\n\t"
 "sw    $ra, 0($sp)\n\t"
 "sw    $s0, 4($sp)\n\t"
 "lui   $s0, 0xB000\n\t"
 "ori   $a0, $a0, 0x1020\n\t"
 "ori   $s0, $s0, 0x205C\n\t"
 "sll   $a0, $a0, 16\n\t"
 "sw    $s1, 8($sp)\n\t"
 "addiu $s1, $zero, 10000\n\t"
 "sw    $a0, 0($s0)\n\t"
 "lw    $a0, 0($s0)\n\t"
 "lw    $a0, 0($s0)\n\t"
 "1:\n\t"
 "nop\n\t"
 "sll   $v0, $a0, 16\n\t"
 "srl   $a0, $a0, 16\n\t"
 "andi  $a0, $a0, 0x8000\n\t"
 "bnez  $a0, 2f\n\t"
 "srl   $v0, $v0, 16\n\t"
 "addiu $s1, $s1, -1\n\t"
 "jal   DelayThread\n\t"
 "addiu $a0, $zero, 10\n\t"
 "bgtz  $s1, 1b\n\t"
 "lw    $a0, 0($s0)\n\t"
 "sll   $a0, $a0, 16\n\t"
 "srl   $v0, $a0, 16\n\t"
 "2:\n\t"
 "lw    $ra, 0($sp)\n\t"
 "lw    $s0, 4($sp)\n\t"
 "lw    $s1, 8($sp)\n\t"
 "jr    $ra\n\t"
 "addiu $sp, $sp, 12\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

extern void _smap_bd_init ( void );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_smap_bd_init:\n\t"
 "lui   $v0, 0xB000\n\t"
 "addiu $at, $zero, 64\n\t"
 "ori   $v1, $v0, 0x3200\n\t"
 "ori   $v0, $v0, 0x3000\n\t"
 "ori   $a0, $zero, 0x8000\n\t"
 "1:\n\t"
 "sh    $zero, 0($v0)\n\t"
 "sh    $zero, 2($v0)\n\t"
 "sh    $zero, 4($v0)\n\t"
 "sh    $zero, 6($v0)\n\t"
 "addiu $v0, $v0, 8\n\t"
 "addiu $at, $at, -1\n\t"
 "sh    $a0,   0($v1)\n\t"
 "sh    $zero, 2($v1)\n\t"
 "sh    $zero, 4($v1)\n\t"
 "sh    $zero, 6($v1)\n\t"
 "bgtz  $at, 1b\n\t"
 "addiu $v1, $v1, 8\n\t"
 "jr    $ra\n\t"
 "nop\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void _smap_reset_fifo_emac3 ( unsigned short* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_smap_reset_fifo_emac3:\n\t"
 "addiu $sp, $sp, -20\n\t"
 "sw    $ra, 0($sp)\n\t"
 "sw    $s0, 4($sp)\n\t"
 "lui   $s0, 0xB000\n\t"
 "sw    $a0, 8($sp)\n\t"
 "ori   $s0, 0x0100\n\t"
 "jal   dev9IntrDisable\n\t"
 "addiu $a0, $zero, 0x7C\n\t"
 "addiu $at, $zero, 1\n\t"
 "sb    $at, 0x0F00($s0)\n\t"
 "lbu   $at, 0x0F00($s0)\n\t"
 "1:\n\t"
 "nop\n\t"
 "andi  $at, $at, 1\n\t"
 "beq   $at, $zero, 1f\n\t"
 "addiu $at, $zero, 1\n\t"
 "jal   DelayThread\n\t"
 "addiu $a0, $zero, 1000\n\t"
 "beq   $zero, $zero, 1b\n\t"
 "lbu   $at, 0x0F00($s0)\n\t"
 "1:\n\t"
 "sb    $at, 0x0F30($s0)\n\t"
 "lbu   $at, 0x0F30($s0)\n\t"
 "nop\n\t"
 "1:\n\t"
 "nop\n\t"
 "andi  $at, $at, 1\n\t"
 "beq   $at, $zero, 1f\n\t"
 "addiu $at, $zero, 0x7C\n\t"
 "jal   DelayThread\n\t"
 "addiu $a0, $zero, 1000\n\t"
 "beq   $zero, $zero, 1b\n\t"
 "lbu   $at, 0x0F30($s0)\n\t"
 "1:\n\t"
 "sb    $zero, 2($s0)\n\t"      // SMAP_BD_MODE
 "sw    $at,  40($s0)\n\t"      // SMAP_INTR_CLR
 "lui   $s0, 0xB000\n\t"
 "addiu $at, $zero, 0x2000\n\t"
 "ori   $s0, 0x2000\n\t"
 "sw    $at, 0($s0)\n\t"
 "lw    $at, 0($s0)\n\t"
 "lw    $at, 0($s0)\n\t"
 "1:\n\t"
 "nop\n\t"
 "andi  $at, $at, 0x2000\n\t"
 "beq   $at, $zero, 1f\n\t"
 "nop\n\t"
 "jal   DelayThread\n\t"
 "addiu $a0, $zero, 1000\n\t"
 "beq   $zero, $zero, 1b\n\t"
 "lw    $at, 0($s0)\n\t"
 "1:\n\t"
 "jal   dev9GetEEPROM\n\t"
 "addiu $a0, $sp, 12\n\t"
 "lui   $at, 0x8000\n\t"
 "ori   $at, $zero, 0x8164\n\t"
 "sw    $at, 4($s0)\n\t"    // SMAP_EMAC3_MODE1
 "lw    $at, 4($s0)\n\t"
 "addiu $at, $zero, 0x380F\n\t"
 "sw    $at, 12($s0)\n\t"   // SMAP_EMAC3_TxMODE1
 "lw    $at, 12($s0)\n\t"
 "ori   $at, $zero, 0xC058\n\t"
 "sw    $at, 16($s0)\n\t"   // SMAP_EMAC3_RxMODE
 "lw    $at, 16($s0)\n\t"
 "lui   $at, 0x01C0\n\t"
 "sw    $at, 20($s0)\n\t"   // SMAP_EMAC3_INTR_STAT
 "lw    $v0, 20($s0)\n\t"
 "sw    $at, 24($s0)\n\t"   // SMAP_EMAC3_INTR_ENABLE
 "lhu   $v0, 14($sp)\n\t"
 "lw    $at, 24($s0)\n\t"
 "lhu   $v1, 12($sp)\n\t"
 "srl   $a1, $v0, 8\n\t"
 "sll   $v0, $v0, 8\n\t"
 "or    $a1, $a1, $v0\n\t"
 "lhu   $v0, 16($sp)\n\t"
 "sll   $a1, $a1, 16\n\t"
 "srl   $a0, $v0, 8\n\t"
 "sll   $v0, $v0, 8\n\t"
 "or    $a0, $a0, $v0\n\t"
 "andi  $a0, $a0, 0xFFFF\n\t"
 "or    $a1, $a1, $a0\n\t"
 "srl   $v0, $v1, 8\n\t"
 "sll   $v1, $v1, 8\n\t"
 "or    $v0, $v0, $v1\n\t"
 "andi  $v0, $v0, 0xFFFF\n\t"
 "sll   $v1, $v0, 16\n\t"
 "srl   $v0, $v0, 16\n\t"
 "or    $v1, $v1, $v0\n\t"
 "sw    $v1, 28($s0)\n\t"
 "lw    $v0, 28($s0)\n\t"   // SMAP_EMAC3_ADDR_HI
 "sll   $v0, $a1, 16\n\t"
 "srl   $a1, $a1, 16\n\t"
 "or    $v0, $v0, $a1\n\t"
 "sw    $v0, 32($s0)\n\t"   // SMAP_EMAC3_ADDR_LO
 "lw    $v0, 32($s0)\n\t"
 "lui   $at, 0xFFFF\n\t"
 "sw    $at, 44($s0)\n\t"   // SMAP_EMAC3_PAUSE_TIMER
 "lw    $at, 44($s0)\n\t"
 "sw    $zero, 64($s0)\n\t" // SMAP_EMAC3_GROUP_HASH1
 "lw    $at, 64($s0)\n\t"
 "sw    $zero, 68($s0)\n\t" // SMAP_EMAC3_GROUP_HASH2
 "lw    $at, 68($s0)\n\t"
 "sw    $zero, 72($s0)\n\t"
 "lw    $at, 72($s0)\n\t"   // SMAP_EMAC3_GROUP_HASH3
 "sw    $zero, 76($s0)\n\t"
 "lw    $at, 76($s0)\n\t"   // SMAP_EMAC3_GROUP_HASH4
 "lui   $at, 0x0004\n\t"
 "sw    $at, 88($s0)\n\t"   // SMAP_EMAC3_INTER_FRAME_GAP
 "lw    $at, 88($s0)\n\t"
 "addiu $at, $zero, 0x6000\n\t"
 "sw    $at, 96($s0)\n\t"   // SMAP_EMAC3_TX_THRESHOLD
 "lw    $at, 96($s0)\n\t"
 "lui   $at, 0x4000\n\t"
 "ori   $at, $at, 0x0800\n\t"
 "sw    $at, 100($s0)\n\t"  // SMAP_EMAC3_RX_WATERMARK
 "lw    $at, 100($s0)\n\t"
 "lw    $a0, 8($sp)\n\t"
 "lw    $ra, 0($sp)\n\t"
 "lw    $s0, 4($sp)\n\t"
 "lw    $at, 12($sp)\n\t"
 "lhu   $v0, 16($sp)\n\t"
 "swl   $at, 3($a0)\n\t"
 "swr   $at, 0($a0)\n\t"
 "sb    $v0, 4($a0)\n\t"
 "srl   $v0, $v0, 8\n\t"
 "sb    $v0, 5($a0)\n\t"
 "beq   $zero, $zero, _smap_bd_init\n\t"
 "addiu $sp, $sp, 20\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

int _smap_phy_reset ( void );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_smap_phy_reset:\n\t"
 "addiu   $sp, $sp, -80\n\t"
 "sw      $s4, 64($sp)\n\t"
 "lui     $s4, %hi( g_SMAP )\n\t"
 "addiu   $s4, %lo( g_SMAP )\n\t"
 "sw      $s5, 68($sp)\n\t"
 "sw      $ra, 76($sp)\n\t"
 "sw      $s6, 72($sp)\n\t"
 "sw      $s3, 60($sp)\n\t"
 "sw      $s2, 56($sp)\n\t"
 "sw      $s1, 52($sp)\n\t"
 "sw      $s0, 48($sp)\n\t"
 "lui     $s3, 0xB000\n\t"
 "xor     $a0, $a0, $a0\n\t"
 "ori     $s3, $s3, 0x2000\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"  // DsPHYTER_BMCR, PHY_BMCR_RST
 "ori     $a1, $zero, 0x8000\n\t"
 "addu    $s2, $zero, 9\n\t"
 "5:\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"   // DsPHYTER_BMCR
 "xor     $a0, $a0, $a0\n\t"
 "andi    $v0, $v0, 0x8000\n\t"
 "beqz    $v0, 2f\n\t"
 "nop\n\t"
 "blez    $s2, 3f\n\t"                 // if ( i <= 0 ) goto 3f
 "nop\n\t"
 "jal     DelayThread\n\t"             // while ( PHY_BMCR_RST ) {
 "addiu   $a0, $zero, 1000\n\t"        //  DelayThread ( 1000 );
 "beq     $zero, $zero, 5b\n\t"        //  goto 5b;
 "addiu   $s2, $s2, -1\n\t"            // }
 "2:\n\t"
 "lw      $v0, 100($s4)\n\t"    // v0 = g_SMSMap.m_fAuto
 "nop\n\t"
 "bnez    $v0, 6f\n\t"          // if ( g_SMSMap.m_fAuto ) goto 6f
 "nop\n\t"
 "lw      $v0, 104($s4)\n\t"    // v0 = g_SMSMap.m_Conf
 "nop\n\t"
 "andi    $s5, $v0, 0x180\n\t"  // if (conf & 0x180) 100Mbps; otherwise, 10Mbps (100FD|100HD)
 "sltu    $s5, $zero, $s5\n\t"  // s5 = (100FD||100HD)?1:0
 "andi    $s0, $v0, 0x140\n\t"  // if (conf & 0x140) Full-Duplex; otherwise, Half-Duplex (100FD|10FD)
 "beqz    $s0, 8f\n\t"          // s0 = (100FD|10FD)?1:0
 "sll     $s1, $s5, 13\n\t"     // s1 = speed selection
 "ori     $s1, $s1, 0x100\n\t"  // PHY_BMCR_DUPM (Full duplex)
 "8:\n\t"
 "xor     $a0, $a0, $a0\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"
 "addu    $a1, $zero, $s1\n\t"
 "21:\n\t"
 "xor     $s2, $s2, $s2\n\t"
 "12:\n\t"
 "lui     $a0, 0x0003\n\t"
 "jal     DelayThread\n\t"
 "ori     $a0, $a0, 0xD40\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"    // DsPHYTER_BMSR
 "addiu   $a0, $zero, 1\n\t"
 "andi    $v0, $v0, 0x4\n\t"            // PHY_BMSR_LINK
 "bnez    $v0, 11f\n\t"
 "addiu   $s2, $s2, 1\n\t"
 "slti    $v0, $s2, 5\n\t"
 "bnez    $v0, 12b\n\t"
 "nop\n\t"
 "beq     $zero, $zero, 12b\n\t"
 "6:\n\t"                       // autonegotiation code
 "xor     $a0, $a0, $a0\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"   // DsPHYTER_BMCR, 0
 "xor     $a1, $a1, $a1\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"    // DsPHYTER_BMSR
 "addiu   $a0, $zero, 1\n\t"
 "andi    $s1, $v0, 0xFFFF\n\t"
 "lw      $a1, 104($s4)\n\t"            // a1 = g_SMSMap.m_Conf
 "andi    $v0, $s1, 0x4000\n\t"         // PHY_BMSR_100TX_FD
 "bnez    $v0, 14f\n\t"
 "andi    $v0, $s1, 0x2000\n\t"         // PHY_BMSR_100TX_HD
 "ori     $v0, $zero, 0xFEFF\n\t"
 "and     $v0, $a1, $v0\n\t"
 "sw      $v0, 104($s4)\n\t"
 "andi    $v0, $s1, 0x2000\n\t"         // PHY_BMSR_100TX_HD
 "14:\n\t"
 "bnez    $v0, 15f\n\t"
 "andi    $v0, $s1, 0x1000\n\t"         // PHY_BMSR_10T_FD
 "lw      $v0, 104($s4)\n\t"
 "ori     $v1, $zero, 0xFF7F\n\t"
 "and     $v0, $v0, $v1\n\t"
 "sw      $v0, 104($s4)\n\t"
 "andi    $v0, $s1, 0x1000\n\t"         // PHY_BMSR_10T_FD
 "15:\n\t"
 "bnez    $v0, 16f\n\t"
 "andi    $v0, $s1, 0x800\n\t"          // PHY_BMSR_10T_HD
 "lw      $v0, 104($s4)\n\t"
 "ori     $v1, $zero, 0xFFBF\n\t"
 "and     $v0, $v0, $v1\n\t"
 "sw      $v0, 104($s4)\n\t"
 "andi    $v0, $s1, 0x800\n\t"          // PHY_BMSR_10T_HD
 "16:\n\t"
 "bnez    $v0, 17f\n\t"
 "ori     $v1, $zero, 0xFFDF\n\t"
 "lw      $v0, 104($s4)\n\t"
 "nop\n\t"
 "and     $v0, $v0, $v1\n\t"
 "sw      $v0, 104($s4)\n\t"
 "17:\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"   // DsPHYTER_ANAR
 "addiu   $a0, $zero, 4\n\t"
 "andi    $a1, $v0, 0xFFFF\n\t"
 "andi    $s1, $a1, 0x1F\n\t"
 "lw      $v0, 104($s4)\n\t"
 "nop\n\t"
 "andi    $v0, $v0, 0x5E0\n\t"
 "or      $s1, $s1, $v0\n\t"
 "addiu   $a0, $zero, 4\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"   // DsPHYTER_ANAR, lVal
 "addu    $a1, $zero, $s1\n\t"
 "xor     $a0, $a0, $a0\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"   // DsPHYTER_BMCR, PHY_BMCR_ANEN | PHY_BMCR_RSAN
 "addiu   $a1, $zero, 4608\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"    // DsPHYTER_BMCR
 "xor     $a0, $a0, $a0\n\t"
 "addiu   $a0, $a0, 4\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"    // DsPHYTER_ANAR
 "andi    $s0, $v0, 0xFFFF\n\t"         // s0 = DsPHYTER_BMCR
 "28:\n\t"
 "xor     $s2, $s2, $s2\n\t"            // s2 = 0
 "32:\n\t"
 "addiu   $s6, $zero, 0x20\n\t"         // s6 = 32
 "27:\n\t"
 "xor     $s0, $s0, $s0\n\t"            // s0 = 0
 "lui     $a0, 0x000F\n\t"
 "22:\n\t"
 "jal     DelayThread\n\t"              // DelayThread ( 1000000 )
 "ori     $a0, $a0, 0x4240\n\t"
 "addiu   $s0, $s0, 1\n\t"              // s0 += 1
 "slti    $v0, $s0, 3\n\t"
 "bnez    $v0, 22b\n\t"                 // if ( s0 < 3 ) goto 22b
 "lui     $a0, 0x000F\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"    // DsPHYTER_BMSR
 "addiu   $a0, $zero, 1\n\t"
 "andi    $s1, $v0, 0xFFFF\n\t"
 "andi    $v0, $s1, 0x30\n\t"           // PHY_BMSR_RM_FAULT | PHY_BMSR_ANCP
 "bne     $v0, $s6, 23f\n\t"            // if ( !PHY_BMSR_ANCP ) goto 23f
 "lui     $a0, 0x0003\n\t"
 "xor     $s0, $s0, $s0\n\t"
 "25:\n\t"
 "jal     DelayThread\n\t"
 "ori     $a0, $a0, 0x0D40\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"   // DsPHYTER_BMSR
 "addiu   $a0, $zero, 1\n\t"
 "andi    $v0, $v0, 0x4\n\t"           // PHY_BMSR_LINK
 "bnez    $v0, 24f\n\t"
 "addiu   $v0, $zero, 1\n\t"
 "addiu   $s0, $s0, 1\n\t"
 "slti    $v0, $s0, 20\n\t"
 "bnez    $v0, 25b\n\t"
 "lui     $a0, 0x0003\n\t"
 "23:\n\t"
 "xor     $a0, $a0, $a0\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"  // DsPHYTER_BMCR, PHY_BMCR_ANEN | PHY_BMCR_RSAN
 "addiu   $a1, $zero, 4608\n\t"
 "addiu   $s2, $s2, 1\n\t"
 "slti    $v0, $s2, 3\n\t"
 "bnez    $v0, 27b\n\t"
 "nop\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"   // DsPHYTER_BMSR
 "addiu   $a0, $zero, 1\n\t"
 "andi    $v0, $v0, 0x4\n\t"           // PHY_BMSR_LINK
 "beqz    $v0, 28b\n\t"
 "nop\n\t"
 "xor     $a0, $a0, $a0\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"  // DsPHYTER_BMCR, PHY_BMCR_SPD100
 "ori     $a1, $zero, 0x2000\n\t"
 "lui     $a0, 0x000F\n\t"
 "jal     DelayThread\n\t"
 "ori     $a0, $a0, 0x4240\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"
 "addiu   $a0, $zero, 1\n\t"
 "andi    $v0, $v0, 0x4\n\t"
 "bnez    $v0, 30f\n\t"
 "nop\n\t"
 "xor     $a0, $a0, $a0\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"  // DsPHYTER_BMCR, 0
 "xor     $a1, $a1, $a1\n\t"
 "lui     $a0, 0x000F\n\t"
 "jal     DelayThread\n\t"
 "ori     $a0, $a0, 0x4240\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"
 "addiu   $a0, $zero, 1\n\t"
 "andi    $v0, $v0, 0x4\n\t"
 "beqz    $v0, 32b\n\t"
 "xor     $s2, $s2, $s2\n\t"
 "30:\n\t"
 "sw      $zero, 100($s4)\n\t"
 "11:\n\t"
 "addiu   $v0, $zero, 1\n\t"
 "24:\n\t"                      // link established
 "lw      $at, 0($s4)\n\t"
 "sll     $v0, $v0, 1\n\t"
 "or      $at, $at, $v0\n\t"
 "sw      $at, 0($s4)\n\t"
 "xor     $s2, $s2, $s2\n\t"
 "addiu   $s0, $sp, 32\n\t"
 "33:\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"
 "addu    $a0, $zero, $s2\n\t"
 "sh      $v0, 0($s0)\n\t"
 "addiu   $s2, $s2, 1\n\t"
 "slti    $v0, $s2, 6\n\t"
 "bnez    $v0, 33b\n\t"
 "addiu   $s0, $s0, 2\n\t"
 "lhu     $v1, 36($sp)\n\t"     // DsPHYTER_PHYIDR1
 "ori     $v0, $zero, 0x2000\n\t"
 "bne     $v1, $v0, 35f\n\t"
 "ori     $v1, $zero, 23584\n\t"
 "lhu     $v0, 38($sp)\n\t"
 "nop\n\t"
 "andi    $v0, $v0, 0xFFF0\n\t"
 "bne     $v0, $v1, 35f\n\t"
 "nop\n\t"
 "lw      $v0, 100($s4)\n\t"
 "nop\n\t"
 "beqz    $v0, 35f\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"   // DsPHYTER_FCSCR
 "addiu   $a0, $zero, 20\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"   // DsPHYTER_RECR
 "addiu   $a0, $zero, 21\n\t"
 "lui     $a0, 0x0007\n\t"
 "jal     DelayThread\n\t"
 "ori     $a0, $a0, 0xA120\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"
 "addiu   $a0, $zero, 20\n\t"
 "addiu   $a0, $zero, 21\n\t"
 "bgezal  $zero, _smap_phy_read\n\t"
 "addu    $s0, $zero, $v0\n\t"
 "andi    $a2, $v0, 0xFFFF\n\t"
 "bnez    $a2, 36f\n\t"
 "andi    $v0, $s0, 0xFFFF\n\t"
 "sltiu   $v0, $v0, 17\n\t"
 "bnez    $v0, 35f\n\t"
 "nop\n\t"
 "36:\n\t"
 "xor     $a0, $a0, $a0\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"
 "xor     $a1, $a1, $a1\n\t"
 "sw      $zero, 100($s4)\n\t"
 "beq     $zero, $zero, 21b\n\t"
 "nop\n\t"
 "35:\n\t"
 "lw      $v0, 100($s4)\n\t"
 "nop\n\t"
 "beqz    $v0, 38f\n\t"
 "addiu   $v1, $zero, 32\n\t"
 "lhu     $v0, 40($sp)\n\t"
 "nop\n\t"
 "andi    $v0, $v0, 0x1E0\n\t"
 "bne     $v0, $v1, 39f\n\t"
 "nop\n\t"
 "beq     $zero, $zero, 50f\n\t"
 "addiu   $a0, $zero, 26\n\t"
 "38:\n\t"
 "lhu     $v0, 32($sp)\n\t"
 "nop\n\t"
 "andi    $v0, $v0, 0x2100\n\t"
 "bnez    $v0, 39f\n\t"
 "nop\n\t"
 "addiu   $a0, $zero, 26\n\t"
 "50:\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"
 "addiu   $a1, $zero, 260\n\t"
 "39:\n\t"
 "lhu     $v0, 38($sp)\n\t"
 "nop\n\t"
 "andi    $v0, $v0, 0xF\n\t"
 "bnez    $v0, 35f\n\t"
 "nop\n\t"
 "addiu   $a0, $zero, 19\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"
 "addiu   $a1, $zero, 1\n\t"
 "addiu   $a0, $zero, 25\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"
 "addiu   $a1, $zero, 6296\n\t"
 "addiu   $a0, $zero, 31\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"
 "xor     $a1, $a1, $a1\n\t"
 "addiu   $a0, $zero, 29\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"
 "addiu   $a1, $zero, 20544\n\t"
 "addiu   $a0, $zero, 30\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"
 "addiu   $a1, $zero, 140\n\t"
 "addiu   $a0, $zero, 19\n\t"
 "bgezal  $zero, _smap_phy_write\n\t"
 "xor     $a1, $a1, $a1\n\t"
 "35:\n\t"
 "lhu     $v1, 32($sp)\n\t"
 "nop\n\t"
 "andi    $v0, $v1, 0x1000\n\t"
 "beqz    $v0, 40f\n\t"
 "xor     $s2, $s2, $s2\n\t"
 "lhu     $v1, 40($sp)\n\t"
 "lhu     $v0, 42($sp)\n\t"
 "nop\n\t"
 "and     $v1, $v1, $v0\n\t"
 "andi    $v0, $v1, 0x180\n\t"
 "sltu    $s5, $s2, $v0\n\t"
 "andi    $v0, $v1, 0x140\n\t"
 "sltu    $s0, $s2, $v0\n\t"
 "beqz    $s0, 41f\n\t"
 "andi    $v0, $v1, 0x400\n\t"
 "beq     $zero, $zero, 41f\n\t"
 "sltu    $s2, $zero, $v0\n\t"
 "40:\n\t"
 "andi    $v0, $v1, 0xFFFF\n\t"
 "srl     $v1, $v0, 13\n\t"
 "andi    $s5, $v1, 0x0001\n\t"
 "srl     $v0, $v0, 8\n\t"
 "lw      $v1, 104($s4)\n\t"
 "andi    $s0, $v0, 0x0001\n\t"
 "srl     $s2, $v1, 10\n\t"
 "andi    $s2, $s2, 0x0001\n\t"
 "41:\n\t"
 "beqz    $s5, 42f\n\t"
 "nop\n\t"
 "bnez    $s0, 43f\n\t"
 "addiu   $v0, $zero, 8\n\t"
 "beq     $zero, $zero, 43f\n\t"
 "addiu   $v0, $zero, 4\n\t"
 "42:\n\t"
 "beqz    $s0, 51f\n\t"
 "addiu   $v0, $zero, 2\n\t"
 "beq     $zero, $zero, 43f\n\t"
 "nop\n\t"
 "3:\n\t"
 "beq     $zero, $zero, 4f\n\t"
 "addiu   $v0, $zero, -1\n\t"
 "10:\n\t"
 "beq     $zero, $zero, 4f\n\t"
 "xor     $v0, $v0, $v0\n\t"
 "51:\n\t"
 "addiu   $v0, $zero, 1\n\t"
 "43:\n\t"
 "lui     $a0, 0x67FF\n\t"
 "lw      $v1, 4($s3)\n\t"
 "ori     $a0, $a0, 0xFFFF\n\t"
 "sll     $v0, $v1, 16\n\t"
 "srl     $v1, $v1, 16\n\t"
 "or      $v0, $v0, $v1\n\t"
 "beqz    $s0, 48f\n\t"
 "and     $s1, $v0, $a0\n\t"
 "lui     $v0, 0x8000\n\t"
 "or      $s1, $s1, $v0\n\t"
 "48:\n\t"
 "beqz    $s2, 49f\n\t"
 "lui     $v0, 0x1800\n\t"
 "or      $s1, $s1, $v0\n\t"
 "49:\n\t"
 "move    $v0, $zero\n\t"
 "sll     $v1, $s1, 16\n\t"
 "srl     $a0, $s1, 16\n\t"
 "or      $v1, $v1, $a0\n\t"
 "sw      $v1, 4($s3)\n\t"
 "lw      $v1, 4($s3)\n\t"
 "4:\n\t"
 "lw      $ra, 76($sp)\n\t"
 "lw      $s6, 72($sp)\n\t"
 "lw      $s5, 68($sp)\n\t"
 "lw      $s4, 64($sp)\n\t"
 "lw      $s3, 60($sp)\n\t"
 "lw      $s2, 56($sp)\n\t"
 "lw      $s1, 52($sp)\n\t"
 "lw      $s0, 48($sp)\n\t"
 "jr      $ra\n\t"
 "addiu   $sp, $sp, 80\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void SMap_Reset ( SMap* apSMap, unsigned short* apAddr ) {

 _smap_reset_fifo_emac3 ( apAddr );
 _smap_phy_reset ();

}  /* end SMap_Reset */

void SMap_Init ( void ) {

 SMap* lpSMap;

 __asm__ __volatile__(
  "la   %0, g_SMAP\n\t"
  : "=r"( lpSMap )
 );

 lpSMap -> m_TXFree = 4096;

 dev9RegisterPreDmaCb  ( 1, SMAP_PreDMACb  );
 dev9RegisterPostDmaCb ( 1, SMAP_PostDMACb );

 SMap_Reset (  lpSMap, ( u16* )lpSMap -> m_HWAddr  );

}  /* end SMap_Init */

void SMap_Start ( void );
__asm__(
 ".set noreorder\n\t"
 ".globl SMap_Start\n\t"
 ".text\n\t"
 "SMap_Start:\n\t"
 "la    $a0, g_SMAP\n\t"
 "lw    $v0, 0($a0)\n\t"
 "addiu $a1, $zero, 0x1800\n\t"
 "andi  $v1, $v0, 1\n\t"
 "bne   $v1, $zero, 1f\n\t"
 "lui   $v1, 0xB000\n\t"
 "sw    $a1, 0x2000($v1)\n\t"
 "lw    $a1, 0x2000($v1)\n\t"
 "ori   $v0, $v0, 1\n\t"
 "sw    $v0, 0($a0)\n\t"
 "j     dev9IntrEnable\n\t"
 "addiu $a0, $zero, 0x0078\n\t"
 "1:\n\t"
 "jr    $ra\n\t"
 "nop\n\t"
 ".set reorder\n\t"
);

u16 SMAP_CopyToFIFO ( u32*, int );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "SMAP_CopyToFIFO:\n\t"
 "srl     $at, $a1, 4\n\t"
 "lui     $v1, 0xB000\n\t"
 "lhu     $v0, 0x1004($v1)\n\t"
 "beqz    $at, 3f\n\t"
 "andi    $a1, $a1, 0xF\n\t"
 "4:\n\t"
 "lwr     $t0,  0($a0)\n\t"
 "lwl     $t0,  3($a0)\n\t"
 "lwr     $t1,  4($a0)\n\t"
 "lwl     $t1,  7($a0)\n\t"
 "lwr     $t2,  8($a0)\n\t"
 "lwl     $t2, 11($a0)\n\t"
 "lwr     $t3, 12($a0)\n\t"
 "lwl     $t3, 15($a0)\n\t"
 "addiu   $at, $at, -1\n\t"
 "sw      $t0, 4352($v1)\n\t"
 "sw      $t1, 4352($v1)\n\t"
 "sw      $t2, 4352($v1)\n\t"
 "addiu   $a0, $a0, 16\n\t"
 "bgtz    $at, 4b\n\t"
 "sw      $t3, 4352($v1)\n\t"
 "3:\n\t"
 "beqz    $a1, 1f\n\t"
 "nop\n\t"
 "2:\n\t"
 "lwr     $at, 0($a0)\n\t"
 "lwl     $at, 3($a0)\n\t"
 "addiu   $a1, $a1, -4\n\t"
 "sw      $at, 4352($v1)\n\t"
 "bnez    $a1, 2b\n\t"
 "addiu   $a0, $a0, 4\n\t"
 "1:\n\t"
 "jr      $ra\n\t"
 "addiu   $v0, $v0, 4096\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "SMAP_PreDMACb:\n\t"
 "lui   $at, 0xB000\n\t"
 "srl   $a0, $a0, 16\n\t"
 "addiu $v1, $zero, 2\n\t"
 "sh    $a0, 0x1038($at)\n\t"
 "jr    $ra\n\t"
 "sb    $v1, 0x1030($at)\n\t"
 "SMAP_PostDMACb:\n\t"
 "lui   $at, 0xB000\n\t"
 "lw    $at, 0x1030($at)\n\t"
 "nop\n\t"
 "andi  $at, $at, 2\n\t"
 "bne   $at, $zero, SMAP_PostDMACb\n\t"
 "nop\n\t"
 "jr    $ra\n\t"
 "nop\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void SMAP_CopyFromFIFO ( struct pbuf*, u16 );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "SMAP_CopyFromFIFO:\n\t"
 "addiu $sp, $sp, -16\n\t"
 "sw    $ra,  0($sp)\n\t"
 "sw    $s0,  4($sp)\n\t"
 "lw    $s0,  4($a0)\n\t"       // s0 = lpData
 "sw    $s1,  8($sp)\n\t"
 "lhu   $s1,  8($a0)\n\t"       // s1 = lLen
 "sw    $s2, 12($sp)\n\t"
 "srl   $s2, $s1, 7\n\t"        // s2 = lDMASize
 "beq   $s2, $zero, 1f\n\t"
 "lui   $at, 0xB000\n\t"
 "sh    $a1, 0x1034($at)\n\t"   // SMAP_RXFIFO_RD_PTR = aPtr
 "addiu $a0, $zero, 1\n\t"      // a0 = 1
 "sll   $a2, $s2, 16\n\t"       // a2 = ( lDmaSize << 16 )
 "addu  $a1, $zero, $s0\n\t"    // a1 = lpData
 "ori   $a2, $a2, 0x20\n\t"     // a2 = ( lDmaSize << 16 ) | 0x20
 "xor   $a3, $a3, $a3\n\t"      // a3 = 0
 "jal   dev9DmaTransfer\n\t"
 "sll   $s2, $s2, 7\n\t"        // lDMASize *= 128
 "addu  $s0, $s0, $s2\n\t"      // lpData += lDMASize
 "subu  $s1, $s1, $s2\n\t"      // lLen   -= lDMASize
 "lui   $at, 0xB000\n\t"
 "1:\n\t"
 "srl   $v0, $s1, 5\n\t"
 "beq   $v0, $zero, 3f\n\t"
 "andi  $s1, $s1, 0x1F\n\t"
 "4:\n\t"
 "lw    $t0, 0x1200($at)\n\t"
 "lw    $t1, 0x1200($at)\n\t"
 "lw    $t2, 0x1200($at)\n\t"
 "lw    $t3, 0x1200($at)\n\t"
 "lw    $t4, 0x1200($at)\n\t"
 "lw    $t5, 0x1200($at)\n\t"
 "lw    $t6, 0x1200($at)\n\t"
 "lw    $t7, 0x1200($at)\n\t"
 "addiu $v0, $v0, -1\n\t"
 "sw    $t0,  0($s0)\n\t"
 "sw    $t1,  4($s0)\n\t"
 "sw    $t2,  8($s0)\n\t"
 "sw    $t3, 12($s0)\n\t"
 "sw    $t4, 16($s0)\n\t"
 "sw    $t5, 20($s0)\n\t"
 "sw    $t6, 24($s0)\n\t"
 "sw    $t7, 28($s0)\n\t"
 "bgtz  $v0, 4b\n\t"
 "addiu $s0, $s0, 32\n\t"
 "3:\n\t"
 "beqz  $s1, 1f\n\t"
 "nop\n\t"
 "2:\n\t"
 "lw    $t0, 0x1200($at)\n\t"
 "addiu $s1, $s1, -4\n\t"
 "sw    $t0, 0($s0)\n\t"
 "bne   $s1, $zero, 2b\n\t"
 "addiu $s0, $s0, 4\n\t"
 "1:\n\t"
 "lw    $ra,  0($sp)\n\t"
 "lw    $s0,  4($sp)\n\t"
 "lw    $s1,  8($sp)\n\t"
 "lw    $s2, 12($sp)\n\t"
 "jr    $ra\n\t"
 "addiu $sp, $sp, 16\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

SMapStatus SMap_Send ( struct pbuf* apPacket ) {

 static u32 sl_TXBuf[ ( SMAP_TXMAXSIZE + SMAP_TXMAXTAILPAD + 3 ) / 4 ]; 

 SMap*   lpSMap;
 smapbd* lpTXBD = ( smapbd* )0xB0003000;
 int     lPackLen;
 int     lTXLen;
 u32*    lpSrc;

 __asm__ __volatile__(
  "la   %0, g_SMAP\n\t"
  : "=r"( lpSMap )
 );

 lpTXBD   += lpSMap -> m_TXEndIdx;
 lPackLen  = apPacket -> tot_len;

 if	(  !( lpSMap -> m_Flags & SMAP_F_LINK )  ) return SMap_Con;

 lTXLen = ( lPackLen + 3 ) & ~3;

 if ( lTXLen > lpSMap -> m_TXFree ) return SMap_TX;

 if ( !apPacket -> next )
  lpSrc = apPacket -> payload;
 else {
  u8* lpDst = ( u8* )sl_TXBuf;
  while ( apPacket ) {
   int lLen = apPacket -> len;
   mips_memcpy ( lpDst, apPacket -> payload, lLen );
   lpDst   += apPacket -> len;
   apPacket = apPacket -> next;
  }  /* end while */
  lpSrc = sl_TXBuf;
 }  /* end else */

 lpTXBD -> m_Len = lPackLen;
 lpTXBD -> m_Ptr = SMAP_CopyToFIFO ( lpSrc, lTXLen );
 __asm__ __volatile__(
  ".set noat\n\t"
  "lui      $at, 0xB000\n\t"
  "sb       $zero, 0x1010($at)\n\t"  // SMAP_TXFIFO_FRAME_INC
  ".set at\n\t"
  ::: "at"
 );
 lpTXBD -> m_Ctl = SMAP_BD_TX_READY | SMAP_BD_TX_GENFCS | SMAP_BD_TX_GENPAD;

 __asm__ __volatile__(
  ".set noat\n\t"
  "lui      $at, 0xB000\n\t"            // SMAP_EMAC3_TxMODE0
  "ori      $a0, $zero, 0x8000\n\t"     // E3_TX_GNP_0
  "sw       $a0, 0x2008($at)\n\t"
  "lw       $at, 0x2008($at)\n\t"
  ".set at\n\t"
  ::: "at", "a0"
 );

 lpSMap -> m_TXFree -= lTXLen;
 SMAP_BD_NEXT( lpSMap -> m_TXEndIdx );

 return SMap_OK;

}  /* end SMap_Send */

void SMap_HandleTXInt ( void ) {

 SMap*   lpSMap;
 smapbd* lpTXBDBase = ( smapbd* )0xB0003000;

 __asm__ __volatile__(
  ".set noat\n\t"
  "la       %0, g_SMAP\n\t"
  "lui      $at, 0xB000\n\t"
  "addiu    $a0, $zero, 16\n\t"
  "sh       $a0, 0x0128($at)\n\t"
  ".set at\n\t"
  : "=r"( lpSMap ) :: "at", "a0"
 );

 while ( lpSMap -> m_TXStartIdx != lpSMap -> m_TXEndIdx ) {

  smapbd* lpBD = lpTXBDBase + lpSMap -> m_TXStartIdx;
  int     lSts = lpBD -> m_Ctl;

  if ( lSts & SMAP_BD_TX_READY ) return;

  lpSMap -> m_TXFree += (  ( lpBD -> m_Len + 3 ) & ~3  );

  SMAP_BD_NEXT( lpSMap -> m_TXStartIdx );

  lpBD -> m_Len = 0;
  lpBD -> m_Ptr = 0;
  lpBD -> m_Ctl = 0;

 }  /* end while */

}  /* end SMap_HandleTXInt */

void SMap_HandleRXInt ( void ) {

 SMap*   lpSMap;
 smapbd* lpRXBDBase = ( smapbd* )0xB0003200;

 __asm__ __volatile__(
  ".set noat\n\t"
  "lui      $at, 0xB000\n\t"
  "la       %0, g_SMAP\n\t"
  "addiu    $a0, $zero, 0x20\n\t"
  "sh       $a0, 0x0128($at)\n\t"
  ".set at\n\t"
  : "=r"( lpSMap ) :: "at", "a0"
 );

 while ( 1 ) {

  smapbd* lpRXBD = lpRXBDBase + lpSMap -> m_RXIdx;
  int     lSts   = lpRXBD -> m_Ctl;
  u16     lPtr;
  u16     lLen;

  if ( lSts & SMAP_BD_RX_EMPTY ) break;

  lPtr = lpRXBD -> m_Ptr;
  lLen = ( lpRXBD -> m_Len + 3 ) & ~3;

  if (  !( lSts & SMAP_BD_RX_ERRMASK )  ) {

   struct pbuf* pBuf = ( struct pbuf* )pbuf_alloc ( PBUF_RAW, lLen, PBUF_POOL );

   if ( pBuf ) {

    SMAP_CopyFromFIFO ( pBuf, lPtr );

    ps2ip_input ( pBuf, &g_NIF );

   } else goto error;

  } else {
error:
   __asm__ __volatile__(
    ".set noat\n\t"
    "lui    $at, 0xB000\n\t"
    "addu   $v1, %0, %1\n\t"
    "sh     $v1, 0x1034($at)\n\t"
    ".set at\n\t"
    :: "r"( lPtr ), "r"( lLen ) : "at", "v1"
   );

  }  /* end else */

  __asm__ __volatile__(
   ".set noat\n\t"
   "lui    $at, 0xB000\n\t"
   "sb     $zero, 0x1040($at)\n\t"  // SMAP_RXFIFO_FRAME_DEC
   ".set at\n\t"
   ::: "at"
  );

  lpRXBD -> m_Ctl = SMAP_BD_RX_EMPTY;
  SMAP_BD_NEXT( lpSMap -> m_RXIdx );

 }  /* end while */

}  /* end SMap_HandleRXInt */
