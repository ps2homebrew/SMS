#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006-2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2007      Petr Otoupal (HDTV support)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
.set noreorder

.globl g_XShift

.sdata
.align 4
s_GSParams:
.space 32
s_PAR_NTSC: .word  0x3F6EEEEF
s_PAR_PAL : .word  0x3F888889
s_PAR_480P: .word  0x3F59999A
s_PAR_576P: .word  0x3F800000
s_PAR_VESA: .word  0x3F800000
s_PAR_720P: .word  0x3F800000
s_PAR_1080I:.word  0x3F800000
s_Half    : .word  0x3F000000
s_OffsetX:  .word 636
            .word 656
s_OffsetY:  .word  25
            .word  50
            .word  36
            .word  72
s_SinCos:   .word   0x00000000
            .word   0x3E9E377A
            .word   0x3F167914
            .word   0x3F4F1BBD
            .word   0x3F737879
            .word   0x3F800000
g_XShift:   .word   0x00000000

.text

.globl GS_Reset
.globl GS_Params
.globl GS_InitDC
.globl GS_SetDC
.globl GS_XYZ
.globl GS_XYZF
.globl GS_XYZv
.globl GS_InitGC
.globl GS_SetGC
.globl GS_InitClear
.globl GS_Clear
.globl GS_InitLoadImage
.globl GS_LoadImage
.globl GS_RRV
.globl GS_RenderRoundRect
.globl GS_VSync
.globl GS_L2P
.globl GS_VSync2
.globl GS_VMode2Index

.ent GS_Params
GS_Params:
    jr  $ra
    la  $v0, s_GSParams
.end GS_Params

.ent GS_Reset
GS_Reset:
    subu    $sp, $sp, 16
    sh      $a0, s_GSParams + 0
    sh      $a1, s_GSParams + 2
    sh      $a2, s_GSParams + 4
    sw      $ra, 0($sp)
    jal     GsPutIMR
    ori     $a0, $zero, 0xFF00
    lw      $v1, s_GSParams + 8
    beq     $v1, $zero, 1f
    nop
    jal     DisableIntc
    move    $a0, $zero
    lw      $a1, s_GSParams + 12
    jal     RemoveIntcHandler
    move    $a0, $zero
    sd      $zero, s_GSParams + 8
1:
    li      $v1, 0x12001000
    lhu     $a0, s_GSParams + 0
    ld      $v0, 0($v1)
    lhu     $a1, s_GSParams + 2
    dsrl    $v0, $v0, 16
    li      $a3, 0x0200
    andi    $v0, $v0, 0x00FF
    sh      $v0, s_GSParams + 6
    sd      $a3, 0($v1)
    lhu     $a2, s_GSParams + 4
    lw      $ra, 0($sp)
    slti    $v0, $a1, 3
    bnel    $v0, $zero, 2f
    lw      $v1, s_PAR_NTSC
    slti    $v0, $a1, 4
    bnel    $v0, $zero, 2f
    lw      $v1, s_PAR_PAL
    slti    $v0, $a1, 0x50
    bnel    $v0, $zero, 2f
    lw      $v1, s_PAR_VESA
    slti    $v0, $a1, 0x51
    bnel    $v0, $zero, 2f
    lw      $v1, s_PAR_480P
    slti    $v0, $a1, 0x52
    bnel    $v0, $zero, 2f
    lw      $v1, s_PAR_1080I
    slti    $v0, $a1, 0x53
    bnel    $v0, $zero, 2f
    lw      $v1, s_PAR_720P
    lw      $v1, s_PAR_576P
2:
    li      $t0, 1
    sll     $v0, $a2, 1
    or      $v0, $v0, $a0
    beql    $v0, $t0, 3f
    sw      $v1, s_GSParams + 16
    lwc1    $f1, s_Half
    mtc1    $v1, $f0
    mul.s   $f0, $f0, $f1
    swc1    $f0, s_GSParams + 16
3:
    la      $t0, s_GSParams + 16
    lw      $v1, s_GSParams + 16
    sw      $v1, s_GSParams + 20
    sw      $v1, s_GSParams + 24
    sw      $v1, s_GSParams + 28
    lqc2    $vf31, 0($t0)
    j       SetGsCrtEx
    addu    $sp, $sp, 16
.end GS_Reset

.ent SetGsCrtEx
SetGsCrtEx:
    addiu   $sp, $sp, -32
    sd      $ra, 0($sp)
    sd      $s0, 8($sp)
    addiu   $s0, $a1, -0x53
    beql    $s0, $zero, 1f
    ori     $a1, $zero, 0x50
1:
    jal     SetGsCrt
    nop
    bne     $s0, $zero, 3f
    nop
    jal     GS_VSync
    nop
.if 1
    lui	    $v0, 0x1200
    lui	    $v1, 0x00A9
    ori	    $v1, $v1, 0x0005
    dsll	$v1, $v1, 0x10
    ori	    $v1, $v1, 0x0210
    dsll	$v1, $v1, 0x10
    ori	    $v1, $v1, 0x1404
    ori	    $v0, $v0, 0x0060
    sd	    $v1, 0x0000($v0)
.else
    jal     GetOsdConfigParam
    addiu   $a0, $sp, 16
    lw		$a1, 16($sp)    
	lui		$a3, 0x1200
	addiu	$a0, $zero, 0x0017
	dsll	$a0, $a0, 16
	ori		$a0, $a0, 0x404B
	dsll	$a0, $a0, 16
	ori		$a0, $a0, 0x0504
	ori		$a3, $a3, 0x0010
	andi	$a1, $a1, 0x0008
	lui		$v1, 0x1200
	dsll	$a1, $a1, 22
	ori		$v1, $v1, 0x0040
	or		$a0, $a1, $a0
	lui		$a2, 0x0004
	ori		$a2, $a2, 0x02e0
	dsll	$a2, $a2, 16
	ori		$a2, $a2, 0x2003
	dsll	$a2, $a2, 16
	ori		$a2, $a2, 0xc827
	sd		$a0, 0x0000($a3)
	lui		$v0, 0x1200
	sd		$a2, 0x0000($v1)
	ori		$v0, $v0, 0x0050
	lui		$a0, 0x0019
	ori		$a0, $a0, 0xca67
	lui		$v1, 0x1200
	sd		$a0, 0x0000($v0)
	ori		$v1, $v1, 0x0060
	lui		$a0, 0x00a9
	dsll	$a0, $a0, 16
	ori		$a0, $a0, 0x0270
	dsll	$a0, $a0, 16
	ori		$a0, $a0, 0x0005
	lui		$v0, 0x1200
	sd		$a0, 0x0000($v1)
	ori		$v0, $v0, 0x0020
	sd		$zero, 0x0000($v0)
	lui		$v1, 0x1200
	ori		$v1, $v1, 0x0030
	addiu	$v0, $zero, 0x0004
	addiu	$a0, $zero, 0x0017
	dsll	$a0, $a0, 16
	ori		$a0, $a0, 0x4049
	dsll	$a0, $a0, 16
	ori		$a0, $a0, 0x0504
	sd		$v0, 0x0000($v1)
	or		$a1, $a1, $a0
    or		$a1, $zero, $a0
	sd		$a1, 0x0000($a3)
	li      $a2, 5000000
2:
	nop
	nop
	addiu   $a2, $a2, -1
	nop
	nop
	bne     $a2, $zero, 2b
	nop
.endif
3:
    ld      $ra, 0($sp)
    ld      $s0, 8($sp)
    jr      $ra
    addu    $sp, $sp, 32
.end SetGsCrtEx

# GS_DISPLAY1_DX_O    ( 0)
# GS_DISPLAY1_DY_O    (12)
# GS_DISPLAY1_MAGH_O  (23)
# GS_DISPLAY1_MAGV_O  (27)
# GS_DISPLAY1_DW_O    (32)
# GS_DISPLAY1_DH_O    (44)    

.ent GS_InitDC
GS_InitDC:
    lhu     $v0, s_GSParams + 2 # v0 = m_GSCRTMode
    slti    $v1, $v0, 4
    sw      $zero, g_XShift
    move    $t8, $zero  
    move    $t9, $zero          # switch ( m_GSCRTMode )
    bnel    $v1, $zero, 1f      #  case PAL/NTSC      : goto 1f
    lhu     $t8, s_GSParams + 0
    li      $v1, 0x50
    beql    $v0, $v1, 2f        #  case DTV_720x480P  : goto 2f
    li      $v1, 1440
    li      $v1, 0x53
    beql    $v0, $v1, 2f        #  case DTV_640x576p  : goto 2f
    li      $v1, 1440
    li      $v1, 0x51
    beql    $v0, $v1, 8f        #  case DTV_1920x1080I: goto 8f
    li      $v1, 1920
    li      $v1, 0x52
    beql    $v0, $v1, 9f        #  case DTV_1280x720P : goto 9f
    li      $v1, 1280
    li      $v1, 0x1A
    beql    $v0, $v1, 4f        #  case VESA640x480@60: goto 4f
    addiu   $t0, $t0, 276 
    li      $v1, 0x1C
    beql    $v0, $v1, 5f        #  case VESA640x480@75: goto 5f
    addiu   $t0, $t0, 356
    break   7                   #  default: exception
4:                              # VESA640x480@60
    beql    $zero, $zero, 6f
    addiu   $t1, $t1, 34
5:                              # VESA640x480@75
    addiu   $t1, $t1, 18
6:                              # VESA common
    lui     $v1, 0x0080 
    lui     $t6, 0x001D
    li      $t3, 0xF4FF
    dsll32  $t6, $t6, 0
    dsll32  $t3, $t3, 0  
    sll     $t1, $t1, 12  
    or      $t6, $t6, $t3
    or      $t6, $t6, $v1
    or      $t6, $t6, $t1
    or      $t6, $t6, $t0
    beq     $zero, $zero, 3f
    li      $v1, 640
2:                              # DTV_720x480P
    div     $v1, $a2
    mflo    $v0
    subu    $v0, 1
    sll     $v0, 23             # MAGH
    addiu   $t0, $t0, 232       # DX = OffsetX + DX
    or      $v0, $t0, $v0
    sll     $t2, $a2, 1         # DW * 2
    subu    $t2, $t2, 1         # DW * 2 - 1
    subu    $t6, $a3, 1         # DH - 1
    dsll32  $t2, $t2, 0     
    dsll32  $t6, $t6, 12  
    addiu   $t1, $t1, 35        # OffsetY = OffsetY + DY
    or      $t6, $t6, $t2       # DW_DH
    sll     $t1, $t1, 12        # DY = OffsetY + 35
    or      $t6, $t6, $t1       # DW_DH_DY_DX
    beq     $zero, $zero, 3f 
    or      $t6, $t6, $v0       # DW_DH_ MAGV_MAGH_DY_DX
8:                              # DTV_1920x1080I
    div     $v1, $a2
    mflo    $v0
    subu    $v0, 1
    sw      $v0, g_XShift
    sllv    $t2, $a2, $v0
    sll     $v0, 23
    addiu   $t0, $t0, 238       # DX = OffsetX + DX
    or      $v0, $t0, $v0
    subu    $t2, $t2, 1
    subu    $t6, $a3, 2
    dsll32  $t2, $t2, 0    
    dsll32  $t6, $t6, 12 
    addiu   $t1, $t1, 40        # OffsetY = OffsetY + DY
    or      $t6, $t6, $t2       # DW_DH
    sll     $t1, $t1, 12        # DY = OffsetY + 35
    or      $t6, $t6, $t1       # DW_DH_DY_DX
    li      $t8, 1              # interlacing
    beq     $zero, $zero, 3f
    or      $t6, $t6, $v0       # DW_DH_ MAGV_MAGH_DY_DX
9:                              # DTV_1280x720p
    addiu   $t0, $t0, 302       # DX = OffsetX + DX
    subu    $t2, $a2, 1  
    subu    $t6, $a3, 1
    dsll32  $t2, $t2, 0
    dsll32  $t6, $t6, 12
    addiu   $t1, $t1, 24        # OffsetY = OffsetY + DY
    or      $t6, $t6, $t2       # DW_DH
    sll     $t1, $t1, 12        # DY = OffsetY + 35
    or      $t6, $t6, $t1       # DW_DH_DY_DX
    beq     $zero, $zero, 3f
    or      $t6, $t6, $t0
1:                              # PAL/NTSC
    addiu   $t2, $a2, 2559
    subu    $v0, $v0, 2
    div     $t2, $a2
    sll     $v0, $v0, 2
    la      $t4, s_OffsetX
    addu    $t4, $t4, $v0
    sll     $v0, $v0, 1
    sll     $t3, $t8, 2
    la      $t5, s_OffsetY
    addu    $t5, $t5, $t3
    lhu     $t9, s_GSParams + 4
    addiu   $a3, $a3, -1
    addu    $t5, $t5, $v0
    and     $v0, $t8, $t9
    sllv    $a3, $a3, $v0
    lw      $t6, 0($t5)
    subu    $a3, $a3,  1
    addu    $t6, $t6, $t1
    dsll32  $a3, $a3, 12
    dsll    $t6, $t6, 12
    or      $t6, $t6, $a3
    lw      $t7, 0($t4)
    mflo    $t5
    mult    $a3, $t5, $a2
    mult1   $v1, $t5, $t0
    subu    $t5, $t5, 1
    dsll    $t5, $t5, 23
    or      $t6, $t6, $t5
    subu    $a3, $a3, 1
    dsll32  $a3, $a3, 0
    addu    $v1, $v1, $t7
    or      $t6, $t6, $a3
    or      $t6, $t6, $v1
3:                              # Common for all
    addiu   $v0, $zero, 0x66
    addiu   $a2, $a2, 63
    sra     $a2, $a2, 6
    sll     $a1, $a1, 15
    sll     $a2, $a2, 9
    sll     $t9, $t9, 1
    li      $a3, 2
    or      $t8, $t8, $t9
    or      $a1, $a1, $a2
    movz    $t8, $a3, $t8
    sd      $v0,  0($a0)        # store m_PMODE en1-0,en2-1,crtmd-1,alpreg-1,blend-0,alpha-0
    sd      $t8,  8($a0)        # store m_SMODE2
    sd      $a1, 16($a0)        # store m_DISPFB
    jr      $ra 
    sd      $t6, 24($a0)        # store m_DISPLAY
.end GS_InitDC

.ent GS_SetDC
GS_SetDC:
    lui     $v0, 0x1200
    ld      $v1,  0($a0)
    ld      $a2,  8($a0)
    ld      $a3, 16($a0)
    ld      $t0, 24($a0)
    sd      $a2,  32($v0)
    sd      $a3, 144($v0)
    beq     $a1, $zero, 1f
    sd      $t0, 160($v0)
    li      $v1, 0x7027
    li      $t2, 1
    addiu   $t3, $zero, 0xFFFF
    dsll32  $t2, 11
    dsrl    $t3, $t3, 20
    dsrl32  $t4, $t0, 12
    or      $a3, $a3, $t2
    and     $t0, $t0, $t3
    sd      $a3, 112($v0)
    dsll32  $t4, $t4, 12
    or      $t0, $t0, $t4
    sd      $t0, 128($v0)
1:
    jr      $ra
    sd      $v1, 0($v0)
.end GS_SetDC

.ent GS_XYZ
GS_XYZ :
_gs_xyz:
    .set noat
    lw          $at, g_XShift
    srav        $a0, $a0, $at
    qmfc2       $at, $vf01
    qmtc2       $a1, $vf01
    vitof0.xyzw $vf01, $vf01
    sll         $v0, $a0, 4
    mtsah       $zero, -1
    ori         $v1, $zero, 0xFFFF
    vmul.xyzw   $vf01, $vf01, $vf31
    dsll32      $a2, $a2, 0
    dsll32      $v1, $v1, 16
    or          $v0, $v0, $a2
    dsrl32      $v1, $v1, 0
    vftoi4.xyzw $vf01, $vf01
    qmfc2       $a1, $vf01
    qmtc2       $at, $vf01
    qfsrv       $a1, $a1
    move        $a2, $a1
    and         $a2, $a2, $v1
    jr          $ra
    or          $v0, $v0, $a2
    .set at
.end GS_XYZ

.ent GS_XYZF
GS_XYZF:
    move    $t0, $ra
    bgezal  $zero, _gs_xyz
    dsll32  $a3, $a3, 24
    move    $ra, $t0
    jr      $ra
    or      $v0, $v0, $a3
.end GS_XYZF

.ent GS_XYZv
GS_XYZv:
    .set noat
    lw      $t1, 0($a3)
    lw      $t2, 4($a3)
    lw      $t3, 8($a3)
    sll     $v1, $a2, 2
    pextlw  $t1, $t1, $t1
    pextlw  $t2, $t2, $t2
    pextlw  $t3, $t3, $t3
    pextlw  $t1, $t1, $t1
    pextlw  $t2, $t2, $t2
    pextlw  $t3, $t3, $t3
    lw      $at, g_XShift
    qmtc2   $t1, $vf01
    qmtc2   $t2, $vf02
    qmtc2   $t3, $vf03
    dsll32  $t0, $t0, 0
    addu    $a3, $a1, $v1
    pcpyld  $t0, $t0
1:
    lqc2    $vf04, 0($a1)
    lqc2    $vf05, 0($a3)
    vmul.xyzw   $vf04, $vf04, $vf01
    vmul.xyzw   $vf05, $vf05, $vf01
    vadd.xyzw   $vf04, $vf04, $vf02
    vadd.xyzw   $vf05, $vf05, $vf03
    vftoi4.xyzw $vf04, $vf04
    vmul.xyzw   $vf05, $vf05, $vf31
    qmfc2   $t1, $vf04
    vftoi4.xyzw $vf05, $vf05
    addu    $a2, $a2, -4
    addu    $a1, $a1, 16
    qmfc2   $t2, $vf05
    addu    $a3, $a3, 16
    pinteh  $t2, $t2, $zero
    bgtzl   $at, 2f
    psrah   $t1, $t1, 1
2:
    por     $t1, $t1, $t2
    pextlw  $t3, $zero, $t1
    pextuw  $t1, $zero, $t1
    por     $t3, $t3, $t0
    por     $t1, $t1, $t0
    sq      $t3,  0($a0)
    sq      $t1, 16($a0)
    bgtz    $a2, 1b
    addu    $a0, $a0, 32
    jr      $ra
    .set at
.end GS_XYZv

.ent GS_InitGC
GS_InitGC:
    addiu   $t3, $a3, 63
    addiu   $v0, $a3, 126
    slti    $v1, $t3, 0
    movz    $v0, $t3, $v1
    sra     $t3, $v0, 6
    andi    $v0, $a2, 2
    srl     $t6, $v0, 1
    sll     $t7, $v0, 26
    nor     $t7, $t7, $t7
    beqz    $v0, 1f
    move    $t4, $a0
    addiu   $t2, $t0, 63
    addiu   $v0, $t0, 126
    slti    $v1, $t2, 0
    movz    $v0, $t2, $v1
    b       2f
    sra     $t2, $v0, 6
1:
    addiu   $t2, $t0, 31
    addiu   $v0, $t0, 62
    slti    $v1, $t2, 0
    movz    $v0, $t2, $v1
    sra     $t2, $v0, 5
2:
    addiu   $a0, $t0, 63
    addiu   $v0, $t0, 126
    slti    $v1, $a0, 0
    movz    $v0, $a0, $v1
    sra     $v0, $v0, 6
    mult    $t5, $t3, $v0
    lui     $v0, 0x1000
    dsll32  $v0, $v0, 0
    ori     $v0, $v0, 0x800C
    sd      $v0, 0($a1)
    li      $v0, 0x000E
    sd      $v0, 8($a1)
    move    $v1, $t3
    dsll    $v1, $v1, 16
    dsll32  $v0, $a2, 0
    dsrl    $v0, $v0, 8
    or      $v1, $v1, $v0
    sd      $v1, 16($a1)
    addiu   $v0, $t4, 0x004C
    sd      $v0, 24($a1)
    mult    $v0, $t3, $t2
    lui     $v1, 0x0A00
    or      $v0, $v0, $v1
    bnez    $t1, 3f
    move    $v1, $v0
    li      $v0, 0x8000
    dsll    $v0, $v0, 17
    or      $v1, $v1, $v0
3:
    and     $v1, $v1, $t7
    sd      $v1, 32($a1)
    addiu   $v0, $t4, 0x004E
    sd      $v0, 40($a1)
    sd      $zero, 48($a1)
    addiu   $v0, $t4, 0x0018
    sd      $v0, 56($a1)
    subu    $v1, $a3, 1
    dsll    $v1, $v1, 16
    subu    $v0, $t0, 1
    dsll32  $v0, $v0, 16
    or      $v1, $v1, $v0
    addiu   $v0, $t4, 0x0040
    sd      $v1, 64($a1)
    li      $a0, 1
    sd      $v0, 72($a1)
    li      $v0, 0x001A
    sd      $a0, 80($a1)
    sd      $v0, 88($a1)
    li      $v0, 0x0046
    sd      $a0, 96($a1)
    sd      $v0, 104($a1)
    li      $v0, 0x0045
    sd      $t6, 112($a1)
    sd      $v0, 120($a1)
    dsll32  $v0, $t1, 0
    dsrl    $v0, $v0, 16
    lui     $v1, 4
    ori     $v1, $v1, 0x0802
    or      $v0, $v0, $v1
    addiu   $v1, $t4, 0x0047
    sd      $v0, 128($a1)
    li      $v0, 0x0044
    sd      $v1, 136($a1)
    addiu   $v1, $t4, 0x0042
    sd      $v0, 144($a1)
    sd      $v1, 152($a1)
    li      $v0, 0x0049
    sd      $zero, 160($a1)
    sd      $v0, 168($a1)
    li      $v1, 0x0060
    addiu   $v0, $t4, 0x0014
    sd      $v1, 176($a1)
    sd      $v0, 184($a1)
    lui     $v0, 0x4071
    ori     $v0, 0x2635
    lui     $v1, 0x7160
    ori     $v1, 0x3524
    pextlw  $v1, $v0, $v1
    li      $v0, 0x0044
    sd      $v1, 192($a1)
    sd      $v0, 200($a1)
    jr      $ra
    move    $v0, $t5
.end GS_InitGC

.ent GS_SetGC
GS_SetGC:
    li  $v1, 0xA000
    b   _gs_send
    li  $a1, 13
.end GS_SetGC

_gs_send:
    lui     $v0, 0x1000
    or      $v0, $v0, $v1
1:
    lw      $v1, 0($v0)
    nop
    nop
    andi    $v1, $v1, 0x0100
    nop
    nop
    bne     $v1, $zero, 1b
    li      $a2, 0x0101
    sw      $a0, 16($v0)
    sw      $a1, 32($v0)
    jr      $ra
    sw      $a2,  0($v0)

_gs_send_chain:
    lui     $v0, 0x1000
    or      $v0, $v0, $v1
1:
    lw      $v1, 0($v0)
    nop
    nop
    andi    $v1, $v1, 0x0100
    nop
    nop
    bne     $v1, $zero, 1b
    li      $a2, 0x0105
    sw      $zero, 32($v0)
    sw      $a0, 48($v0)
    jr      $ra
    sw      $a2, 0($v0)

.ent GS_InitClear
GS_InitClear:
    dsll32  $t2, $t2, 0
    move    $t5, $a0
    move    $t7, $ra
    dsrl    $t2, $t2, 16
    sd      $zero, 0($t5)
    lui     $v0, 0x5000
    ori     $v0, $v0, 7
    dsll32  $v0, $v0, 0
    sd      $v0, 8($t5)
    lui     $v0, 0x1000
    dsll32  $v0, $v0, 0
    ori     $v0, $v0, 0x8006
    sd      $v0, 16($t5)
    li      $v0, 0x000E
    sd      $v0, 24($t5)
    lui     $t3, 0x0002
    ori     $t3, $t3, 0x0802
    or      $t3, $t3, $t2
    sd      $t3, 32($t5)
    li      $t4, 0x0047
    sd      $t4, 40($t5)
    li      $v0, 0x0006
    sd      $v0, 48($t5)
    sd      $zero, 56($t5)
    sd      $t1, 64($t5)
    li      $v0, 0x0001
    move    $a0, $a1
    sd      $v0, 72($t5)
    move    $a1, $a2
    bgezal  $zero, _gs_xyz  
    move    $a2, $zero
    sd      $v0, 80($t5)
    li      $t6, 0x0005
    move    $a0, $a3
    sd      $t6, 88($t5)
    bgezal  $zero, _gs_xyz
    move    $a1, $t0
    sd      $v0, 96($t5)
    sd      $t6, 104($t5)
    lui     $t3, 0x0004
    ori     $t3, $t3, 0x0802
    or      $t2, $t2, $t3
    move    $ra, $t7
    sd      $t2, 112($t5)
    jr      $ra
    sd      $t4, 120($t5)
.end GS_InitClear

.ent GS_Clear
GS_Clear:
    li  $v1, 0x9000
    b   _gs_send
    li  $a1, 8
.end GS_Clear

.ent GS_InitLoadImage
GS_InitLoadImage:
    beqz    $a3, 1f
    mult    $t4, $t2, $t3
    li      $v0, 0x0030
    beq     $a3, $v0, 1f
    li      $v0, 0x0001
    beq     $a3, $v0, 2f
    li      $v0, 0x0031
    bne     $a3, $v0, 3f
    li      $v0, 0x0002
2:
    sll     $v0, $t4, 1
    addu    $t4, $t4, $v0
    beq     $zero, $zero, 4f
    srl     $t4, $t4, 4
1:
    b       4f
    srl     $t4, $t4, 2
3:
    beq     $a3, $v0, 5f
    li      $v0, 0x000A
    beq     $a3, $v0, 5f
    li      $v0, 0x0032
    beq     $a3, $v0, 5f
    li      $v0, 0x003A
    bne     $a3, $v0, 6f
    li      $v0, 0x0013
5:
    b       4f
    srl     $t4, $t4, 3
6:
    beq     $a3, $v0, 7f
    li      $v0, 0x001B
    bnel    $a3, $v0, 4f
    srl     $t4, $t4, 5
7:
    srl     $t4, $t4, 4
4:
    lui     $v0, 0x1000
    ori     $v0, $v0, 0x0007
    sd      $v0, 0($a0)         # m_DMATag1[ 0 ] = DMA_TAG( 7, 0, DMATAG_ID_CNT, 0, 0, 0 );
    lui     $v0, 0x5000
    ori     $v0, $v0, 0x0007
    dsll32  $v0, $v0, 0
    sd      $v0, 8($a0)         # m_FMATag1[ 1 ] = VIF_DIRECT( 7 )
    lui     $v0, 0x1000
    dsll32  $v0, $v0, 0
    ori     $v0, $v0, 0x0005
    sd      $v0, 16($a0)
    li      $v0, 0x000E
    sd      $v0, 24($a0)
    sd      $zero, 32($a0)
    li      $v0, 0x003F
    sd      $v0, 40($a0)
    dsll32  $v0, $a1, 0
    dsll32  $v1, $a2, 16
    or      $v0, $v0, $v1
    dsll32  $v1, $a3, 24
    or      $v0, $v0, $v1
    sd      $v0, 48($a0)
    li      $v0, 0x0050
    sd      $v0, 56($a0)
    dsll32  $v0, $t0, 0
    dsll32  $v1, $t1, 16
    or      $v0, $v0, $v1
    sd      $v0, 64($a0)
    li      $v0, 0x0051
    sd      $v0, 72($a0)
    dsll32  $v0, $t3, 0
    or      $v0, $t2, $v0
    sd      $v0, 80($a0)
    li      $v0, 0x0052
    sd      $v0, 88($a0)
    sd      $zero, 96($a0)
    li      $v0, 0x0053
    sd      $v0, 104($a0)
    ori     $v0, $t4, 0x8000
    dsll32  $v0, $v0, 0
    dsrl32  $v0, $v0, 0
    li      $v1, 0xC000
    dsll32  $v1, $v1, 13
    or      $v0, $v0, $v1
    sd      $v0, 112($a0)
    sd      $zero, 120($a0)
    lui     $v0, 0x3000
    or      $v0, $v0, $t4
    sd      $v0, 128($a0)       # m_DMATag2[ 0 ] = DMA_TAG( lQWC, 0, DMATAG_ID_REF, 0, 0, 0 )
    lui     $v0, 0x5000
    or      $v0, $v0, $t4
    dsll32  $v0, $v0, 0
    sd      $v0, 136($a0)       # m_DMATag2[ 1 ] = VIF_DIRECT( lQWC )
    lui     $v0, 0x6000
    sd      $v0, 144($a0)       # m_DMATag3[ 0 ] = DMA_TAG( 0, 0, DMATAG_ID_RET, 0, 0, 0 )
    jr      $ra
    sd      $zero, 152($a0)     # m_DMATag3[ 1 ] = VIF_NOP()
.end GS_InitLoadImage

.ent GS_LoadImage
GS_LoadImage:
    lui     $v0, 0x2000
    or      $v0, $v0, $a0
    sw      $a1, 132($v0)
    sync
    b       _gs_send_chain
    or      $v1, $zero, 0xA000
.end GS_LoadImage

.ent GS_RRV
GS_RRV :
_gs_rrv:
    sub     $sp, $sp, 24
    mtc1    $t1, $f0
    mtc1    $a1, $f1
    mtc1    $a2, $f2
    mtc1    $a3, $f3
    mtc1    $t0, $f4
    cvt.s.w $f0, $f0
    cvt.s.w $f1, $f1
    cvt.s.w $f2, $f2
    cvt.s.w $f3, $f3
    cvt.s.w $f4, $f4
    move    $t2, $a0
    move    $t3, $a1
    move    $a0, $sp
    move    $t9, $ra
    la      $a1, s_SinCos
    li      $v0, 6
    move    $t4, $a2
1:
    lwc1    $f5, 0($a1)
    addu    $a1, $a1, 4
    mul.s   $f5, $f5, $f0
    subu    $v0, $v0, 1
    swc1    $f5, 0($a0)
    bgtz    $v0, 1b
    addu    $a0, $a0, 4
    add.s   $f5, $f1, $f0
    add.s   $f6, $f2, $f0
    sra     $a0, $a3, 1
    sra     $a1, $t0, 1
    addu    $a0, $a0, $t3
    addu    $a1, $a1, $t4
    bgezal  $zero, _gs_xyz
    move    $a2, $zero
    sd      $v0, 0($t2)
    move    $a0, $t3
    addu    $a1, $t4, $t1
    bgezal  $zero, _gs_xyz
    move    $a2, $zero
    sd      $v0, 8($t2)
    addu    $t2, $t2, 16
    lui     $t6, 0x3F00
    li      $t8, 6
    mtc1    $t6, $f7
    move    $t5, $sp
    addu    $t6, $sp, 20
2:
    lwc1    $f8, 0($t6)
    subu    $t6, $t6, 4
    lwc1    $f9, 0($t5)
    addu    $t5, $t5, 4
    neg.s   $f8, $f8
    neg.s   $f9, $f9
    subu    $t8, $t8, 1
    add.s   $f8, $f8, $f5
    add.s   $f9, $f9, $f6
    add.s   $f8, $f8, $f7
    add.s   $f9, $f9, $f7
    cvt.w.s $f8, $f8
    cvt.w.s $f9, $f9
    mfc1    $a0, $f8
    mfc1    $a1, $f9
    bgezal  $zero, _gs_xyz
    move    $a2, $zero
    sd      $v0, 0($t2)
    bgtz    $t8, 2b
    addu    $t2, $t2, 8
    add.s   $f5, $f1, $f3
    addu    $a0, $t3, $a3
    move    $a1, $t4
    move    $a2, $zero
    subu    $a0, $a0, $t1
    bgezal  $zero, _gs_xyz
    sub.s   $f5, $f5, $f0
    sd      $v0, 0($t2)
    addu    $t2, $t2, 8
    addu    $t5, $sp, 4
    addu    $t6, $sp, 16
    li      $t8, 5
3:
    lwc1    $f9, 0($t6)
    subu    $t6, $t6, 4
    lwc1    $f8, 0($t5)
    addu    $t5, $t5, 4
    neg.s   $f9, $f9
    subu    $t8, $t8, 1
    add.s   $f8, $f8, $f5
    add.s   $f9, $f9, $f6
    add.s   $f8, $f8, $f7
    add.s   $f9, $f9, $f7
    cvt.w.s $f8, $f8
    cvt.w.s $f9, $f9
    mfc1    $a0, $f8
    mfc1    $a1, $f9
    bgezal  $zero, _gs_xyz
    move    $a2, $zero
    sd      $v0, 0($t2)
    bgtz    $t8, 3b
    addu    $t2, $t2, 8
    add.s   $f6, $f2, $f4
    addu    $a0, $t3, $a3
    addu    $a1, $t4, $t0
    sub.s   $f6, $f6, $f0
    move    $a2, $zero
    bgezal  $zero, _gs_xyz
    subu    $a1, $a1, $t1
    sd      $v0, 0($t2)
    addu    $t2, $t2, 8
    addu    $t5, $sp, 20
    move    $t6, $sp
    li      $t8, 6
4:
    lwc1    $f8, 0($t5)
    subu    $t5, $t5, 4
    lwc1    $f9, 0($t6)
    addu    $t6, $t6, 4
    subu    $t8, $t8, 1
    add.s   $f8, $f8, $f5
    add.s   $f9, $f9, $f6
    add.s   $f8, $f8, $f7
    add.s   $f9, $f9, $f7
    cvt.w.s $f8, $f8
    cvt.w.s $f9, $f9
    mfc1    $a0, $f8
    mfc1    $a1, $f9
    bgezal  $zero, _gs_xyz
    move    $a2, $zero
    sd      $v0, 0($t2)
    bgtz    $t8, 4b
    addu    $t2, $t2, 8
    add.s   $f5, $f1, $f0
    addu    $a0, $t3, $t1
    addu    $a1, $t4, $t0
    bgezal  $zero, _gs_xyz
    move    $a2, $zero
    sd      $v0, 0($t2)
    addu    $t2, $t2, 8
    addu    $t5, $sp, 4
    addu    $t6, $sp, 16
    li      $t8, 5
5:
    lwc1    $f8, 0($t5)
    addu    $t5, $t5, 4
    lwc1    $f9, 0($t6)
    subu    $t6, $t6, 4
    neg.s   $f8, $f8
    subu    $t8, $t8, 1
    add.s   $f9, $f9, $f6
    add.s   $f8, $f8, $f5
    add.s   $f9, $f9, $f7
    add.s   $f8, $f8, $f7
    cvt.w.s $f9, $f9
    cvt.w.s $f8, $f8
    mfc1    $a1, $f9
    mfc1    $a0, $f8
    bgezal  $zero, _gs_xyz
    move    $a2, $zero
    sd      $v0, 0($t2)
    bgtz    $t8, 5b
    addu    $t2, $t2, 8
    addu    $a1, $t4, $t1
    move    $a0, $t3
    bgezal  $zero, _gs_xyz
    move    $a2, $zero
    move    $ra, $t9
    sd      $v0, 0($t2)
    jr      $ra
    add     $sp, $sp, 24
.end GS_RRV

.ent GS_RenderRoundRect
GS_RenderRoundRect:
    subu    $sp, $sp, 160
    sd      $fp, 144($sp)
    sd      $s7, 128($sp)
    move    $s7, $t2
    sd      $a1, 112($sp)
    sd      $a2, 96($sp)
    sd      $a3, 80($sp)
    sd      $t0, 64($sp)
    sd      $s2, 48($sp)
    move    $s2, $t1
    sd      $s1, 32($sp)
    move    $s1, $a0
    bgez    $t1, 1f
    sd      $s0, 16($sp)
    negu    $s2, $t1
    li      $v0, 2
    sw      $v0, 0($sp)
    addiu   $fp, $zero, 1
    addiu   $s0, $a0, 56
    b       2f
    li      $v0, 27
1:
    li      $v0, 0x45
    sw      $v0, 0($sp)
    xor     $fp, $fp, $fp
    addiu   $s0, $a0, 64
    li      $v0, 28
2:
    sw      $v0, 4($sp)
    move    $t7, $ra
    move    $a0, $s0
    lw      $t0,  64($sp)
    lw      $a3,  80($sp)
    lw      $a2,  96($sp)
    lw      $a1, 112($sp)
    bgezal  $zero, _gs_rrv
    move    $t1, $s2
    move    $ra, $t7
    sd      $zero, 0($s1)
    lui     $v0, 0x5000
    ori     $v0, $v0, 17
    dsll32  $v0, $v0, 0
    sd      $v0, 8($s1)
    lui     $v0, 0x2400
    dsll32  $v0, $v0, 0
    ori     $v0, $v0, 1
    sd      $v0, 16($s1)
    li      $v0, 16
    sd      $v0, 24($s1)
    lw      $v0, 0($sp)
    or      $v1, $zero, $v0
    move    $v0, $fp
    ld      $fp, 144($sp)
    dsll    $v0, $v0, 7
    or      $v0, $v1, $v0
    li      $v1, 256
    or      $v0, $v0, $v1
    sd      $v0, 32($s1)
    sd      $s7, 40($s1)
    lw      $v0, 4($sp)
    ori     $v1, $v0, 0x8000
    li      $v0, 0xA000
    dsll32  $v0, $v0, 13
    or      $v0, $v1, $v0
    sd      $v0, 48($s1)
    li      $v0, 5
    sd      $v0, 56($s1)
    ld      $s7, 128($sp)
    ld      $s2,  48($sp)
    ld      $s1,  32($sp)
    ld      $s0,  16($sp)
    jr      $ra
    addiu   $sp, $sp, 160
.end GS_RenderRoundRect

.ent GS_VSync
GS_VSync:
    lui     $v0, 0x1200
    lw      $v1, 0x1000($v0)
    ori     $v1, $v1, 8
    sw      $v1, 0x1000($v0)
1:
    lw      $v1, 0x1000($v0)
    nop
    andi    $v1, $v1, 8
    nop
    beq     $v1, $zero, 1b
    nop
    jr      $ra
    nop
.end GS_VSync

.ent GS_L2P
GS_L2P:
    .set noat
    lw          $v0, g_XShift
    pcpyld      $a1, $zero, $a1
    qmfc2       $at, $vf01
    pextlw      $a1, $a3, $a1
    qmtc2       $a1, $vf01
    vitof0.xyzw $vf01, $vf01
    srav        $a0, $a0, $v0
    dsll32      $v0, $a2, 0
    mtsah       $zero, -1
    vmul.xyzw   $vf01, $vf01, $vf31
    or          $v0, $v0, $a0
    vftoi0      $vf01, $vf01
    qmfc2       $a1, $vf01
    qmtc2       $at, $vf01
    qfsrv       $a1, $a1, $a1
    jr          $ra
    or          $v0, $v0, $a1
    .set at
.end GS_L2P

.ent GS_VSync2
GS_VSync2:
    .set noat
    lui     $v0, 0x1001
    addiu   $v1, $zero, 8
    sw      $v1, -4096($v0)
1:
    lw      $v1, -4096($v0)
    andi    $v1, $v1, 8
    beq     $v1, $zero, 1b
    nop
    lui     $v0, 0x1200
    addiu   $v1, $zero, 4
    nor     $at, $v1, $zero
1:
    ld      $a2, 4096($v0)
    or      $a2, $a2, $v1
    sd      $a2, 4096($v0)
2:
    ld      $a2, 4096($v0)
    andi    $a3, $a2, 4
    beq     $a3, $zero, 2b
    nop
    and     $a2, $a2, $at
    addiu   $a0, $a0, -1
    bgtz    $a0, 1b
    sd      $a2, 4096($v0)
    jr      $ra
    .set at
.end GS_VSync2

.ent GS_VMode2Index
GS_VMode2Index:
    .set noat
    ori     $t0, $zero, 0x0002  # GSVideoMode_NTSC           - 0
    ori     $t1, $zero, 0x0003  # GSVideoMode_PAL            - 1
    ori     $t2, $zero, 0x0050  # GSVideoMode_DTV_720x480P   - 2
    ori     $t3, $zero, 0x0051  # GSVideoMode_DTV_1920x1080I - 5
    ori     $t4, $zero, 0x0052  # GSVideoMode_DTV_1280x720P  - 4
    ori     $t5, $zero, 0x001A  # GSVideoMode_VESA_60Hz      - 6
    ori     $t6, $zero, 0x001C  # GSVideoMode_VESA_75Hz      - 7
    ori     $v0, $zero, 0x0008  # GSVideoMode_Default        - 8
    ori     $at, $zero, 0x0053  # GSVideoMode_DTV_640x576P   - 3
    beql    $a0, $t0, 1f
    ori     $v0, $zero, 0       # NTSC
    beql    $a0, $t1, 1f
    ori     $v0, $zero, 1       # PAL
    beql    $a0, $t2, 1f
    ori     $v0, $zero, 2       # 480p
    beql    $a0, $t3, 1f
    ori     $v0, $zero, 5       # 1080i
    beql    $a0, $t4, 1f
    ori     $v0, $zero, 4       # 720p
    beql    $a0, $t5, 1f
    ori     $v0, $zero, 6       # VESA60Hz
    beql    $a0, $t6, 1f
    ori     $v0, $zero, 7       # VESA75Hz
    beql    $a0, $at, 1f
    ori     $v0, $zero, 3       # 576p
1:
    jr      $ra
    nop
    .set at
.end GS_VMode2Index
