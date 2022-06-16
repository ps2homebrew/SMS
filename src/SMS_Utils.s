/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# MUL64 is pulled from some binary library (I don't remember which one).
# mips_memcpy routine is pulled from 'sde' library from MIPS.
#
*/
.set noat
.set noreorder
.set nomacro

.globl MUL64
.globl PowerOf2
.globl PowF
.globl mips_memcpy
.globl mips_memset
.globl memcpy_swap16
.globl pcm_syn1
.globl pcm_syn2
.globl pcm_synN
.globl strlen
.globl strcat
.globl strcpy
.globl strncmp
.globl strncpy
.globl strchr
.globl strcmp
.globl strrchr
.globl strstr
.globl strpbrk
.globl memmove

.data
.align 4
s_MuxConst: .float 0.414214, 0.414214, 0.414214, 0.414214
            .float 0.292894, 0.292894, 0.292894, 0.292894

s_Const:    .word 0x0007FFFFF, 0x07F800000, 0x03FB504F3, 0x000800000
            .word 0x03F800000, 0x00000007F, 0x03E18EFE2, 0x04038AA3B
            .word 0x03E4CAF6F, 0x03EAAAABD, 0x0FF7FFFFF, 0x043BC00B5
            .word 0x041E77545, 0x0451E424B, 0x045E451C5, 0x040000000
            .word 0x0C2FC0000, 0x043000000, 0x07F7FFFFF

.text

MUL64:
    pmultuw	$v0, $a0, $a1
    dsra32	$a2, $a0, 0
    dsra32  $v1, $a1, 0
    mult    $v1, $a0, $v1
    mult1   $a2, $a2, $a1
    addu    $v1, $v1, $a2
    dsll32  $v1, $v1, 0
    jr      $ra
    daddu   $v0, $v0, $v1

memmove:
    subu    $t0, $a1, $a0
    slti    $at, $t0, 9
    beqz    $at, _memcpy
    nop
    bgtz    $t0, small_memcpy
    move    $v0, $a0
    addu    $t0, $a2, $t0
    bltz    $t0, _memcpy
    xor     $t0, $a0, $a1
    beqz    $t0, memmove_out
    move    $v0, $a0
small_rmemcpy:
    addu    $a0, $a0, $a2
    addu    $a1, $a1, $a2
    addiu   $t2, $a2, -8
    bltz    $t2, small_byte_rmemcpy
    nop
1:
    ldl     $t0, -1($a1)
    ldr     $t0, -8($a1)
    addiu   $t2, $t2, -8
    sdl     $t0, -1($a0)
    sdr     $t0, -8($a0)
    addiu   $a1, $a1, -8
    bgez    $t2, 1b
    addiu   $a0, $a0, -8
    addiu   $a2, $t2, 8
small_byte_rmemcpy:
    beqz    $a2, memmove_out
    nop
1:
    lbu     $t0, -1($a1)
    addiu   $a2, $a2, -1
    addiu   $a0, $a0, 1
    addiu   $a1, $a1, 1
    sb      $t0, -2($a0)
    bnez    $a2, 1b
    nop
memmove_out:
    jr      $ra
    move    $a2, $zero
mips_memcpy:
_memcpy:
    addu    $at, $a1, $a2
    slti    $v0, $a2, 32
    bnez    $v0, small_memcpy
    or      $v0, $zero, $a0
    xor     $t0, $a1, $a0
    andi    $t3, $t0, 0x7
    bnez    $t3, unaligned_memcpy
    andi    $t2, $t0, 0xF
    negu    $t1, $a0
    andi    $t1, $t1, 0x7
    beqz    $t1, try_double_word_cpy
    subu    $a2, $a2, $t1
    ldr     $t0, 0($a1)
    ldl     $t0, 7($a1)
    addu    $a1, $a1, $t1
    sdr     $t0, 0($a0)
    sdl     $t0, 7($a0)
    addu    $a0, $a0, $t1
try_double_word_cpy:
    bnez    $t2, memcpy_dword
    andi    $t1, $a0, 0x8
align_quad_word:
    beqz    $t1, memcpy_qword
    addiu   $t2, $a2, -32
    ld      $t0, 0($a1)
    addiu   $a1, $a1, 8
    sd      $t0, 0($a0)
    addiu   $a0, $a0, 8
    addiu   $a2, $a2,  -8
    addiu   $t2, $a2, -32
memcpy_qword:
    bltz    $t2, memcpy_dword
    nop
1:
    lq      $t0,  0($a1)
    addiu   $t2, $t2, -32
    lq      $t1, 16($a1)
    addiu   $a1, $a1, 32
    sq      $t0,  0($a0)
    sq      $t1, 16($a0)
    bgez    $t2, 1b
    addiu   $a0, $a0, 32
    beq     $zero, $zero, small_memcpy
    addiu   $a2, $t2, 32
memcpy_dword:
    addiu   $t2, $a2, -16
    nop
1:
    ld      $t0, 0($a1)
    addiu   $t2, $t2, -16
    ld      $t1, 8($a1)
    addiu   $a1, $a1, 16
    sd      $t0, 0($a0)
    sd      $t1, 8($a0)
    bgez    $t2, 1b
    addiu   $a0, $a0, 16
    beq     $zero, $zero, small_memcpy
    addiu   $a2, $t2, 16
unaligned_memcpy:
    negu    $t1, $a1
    andi    $t1, $t1, 0x7
    beqz    $t1, 1f
    subu    $a2, $a2, $t1
    ldr     $t0, 0($a1)
    ldl     $t0, 7($a1)
    addu    $a1, $a1, $t1
    sdr     $t0, 0($a0)
    sdl     $t0, 7($a0)
    addu    $a0, $a0, $t1
1:
    addiu   $t2, $a2, -16
    nop
1:
    ld      $t0,  0($a1)
    addiu   $t2, $t2, -16
    ld      $t1,  8($a1)
    addiu   $a1, $a1, 16
    sdr     $t0,  0($a0)
    sdl     $t0,  7($a0)
    sdr     $t1,  8($a0)
    sdl     $t1, 15($a0)
    bgez    $t2, 1b
    addiu   $a0, $a0, 16
    addiu   $a2, $t2, 16
    nop
small_memcpy:
    addiu   $t2, $a2, -8
    bltz    $t2, small_byte_memcpy
    nop
    nop
1:
    ldr     $t0, 0($a1)
    ldl     $t0, 7($a1)
    addiu   $t2, $t2, -8
    sdr     $t0, 0($a0)
    sdl     $t0, 7($a0)
    addiu   $a0, $a0, 8
    bgez    $t2, 1b
    addiu   $a1, $a1, 8
    addiu   $a2, $t2, 8
small_byte_memcpy:
    beqz    $a2, memcpy_out
    nop
1:
    lbu     $t0, 0($a1)
    addiu   $a2, $a2, -1
    addiu   $a0, $a0, 1
    addiu   $a1, $a1, 1
    sb      $t0, -1($a0)
    bnez    $a2, 1b
    nop
memcpy_out:
    jr      $ra
    move    $a2, $zero

mips_memset:
    beqz    $a2, 1f
    sltiu   $at, $a2, 16
    bnez    $at, 2f
    andi    $a1, $a1, 0xFF
    dsll    $at, $a1, 0x8
    or      $a1, $a1, $at
    dsll    $at, $a1, 0x10
    or      $a1, $a1, $at
    dsll32  $at, $a1, 0x0
    or      $a1, $a1, $at
    andi    $v1, $a0, 0x7
    beqz    $v1, 3f
    addiu   $a3, $zero, 8
    subu    $a3, $a3, $v1
    subu    $a2, $a2, $a3
    sdr     $a1, 0($a0)
    addu    $a0, $a0, $a3
3:
    andi    $v1, $a2, 0x1f
    subu    $a3, $a2, $v1
    beqz    $a3, 4f
    addu    $a2, $zero, $v1
    addu    $a3, $a3, $a0
5:
    sd      $a1,  0($a0)
    sd      $a1,  8($a0)
    sd      $a1, 16($a0)
    addiu   $a0, $a0, 32
    sd      $a1, -8($a0)
    bne     $a0, $a3, 5b
4:
    andi    $v1, $a2, 0x7
    subu    $a3, $a2, $v1
    beqz    $a3, 2f
    addu    $a2, $zero, $v1
    addu    $a3, $a3, $a0
6:
    addiu   $a0, $a0, 8
    beq     $a0, $a3, 2f
    sd      $a1, -8($a0)
    addiu   $a0, $a0, 8
    beq     $a0, $a3, 2f
    sd      $a1, -8($a0)
    addiu   $a0, $a0, 8
    bne     $a0, $a3, 6b
    sd      $a1, -8($a0)
2:
    beqz    $a2, 1f
    addu    $a3, $a2, $a0
7:
    addiu   $a0, $a0, 1
    beq     $a0, $a3, 1f
    sb      $a1, -1($a0)
    addiu   $a0, $a0, 1
    beq     $a0, $a3, 1f
    sb      $a1, -1($a0)
    addiu   $a0, $a0, 1
    bne     $a0, $a3, 7b
    sb      $a1, -1($a0)
1:
    jr  $ra
    nop

PowerOf2:
    pnor    $v1, $zero, $zero
    dsll32  $a1, $a1, 0
    psrlw   $v1, $v1, 31
    or      $a0, $a0, $a1
    psllw   $v0, $v1, 5
    psubw   $a0, $a0, $v1
    psubw   $v0, $v0, $v1
    plzcw   $a0, $a0
    psubw   $v0, $v0, $a0
    sw      $v0, 0($a2)
    dsrl32  $v0, $v0, 0
    jr      $ra
    sw      $v0, 0($a3)

PowF:
    lui     $v0, %hi( s_Const )
    mfc1    $t0, $f12
    addiu   $v0, $v0, %lo( s_Const )
    cvt.w.s $f03, $f13
    lw      $t1,  0($v0)
    lw      $t2,  4($v0)
    lw      $v1,  8($v0)
    lw      $t7, 12($v0)
    lw      $t3, 16($v0)
    mfc1    $at, $f03
    and     $t4, $t0, $t1
    sll     $at, $at, 31
    and     $t5, $t0, $t2
    or      $t4, $t4, $t3
    mtc1    $t3, $f11
    srl     $t5, $t5, 23
    pcgtw   $v1, $t4, $v1
    pceqw   $t6, $zero, $t5
    sub     $t5, $t5, $v1
    and     $v1, $v1, $t7
    lw      $t7, 20($v0)
    sub     $t4, $t4, $v1
    sub     $t5, $t5, $t7
    mtc1    $t4, $f02
    sub.s   $f03, $f02, $f11
    add.s   $f02, $f02, $f11
    div.s   $f07, $f11, $f02
    mtc1    $zero, $f08
    lwc1    $f05, 24($v0)
    lwc1    $f06, 28($v0)
    lwc1    $f09, 32($v0)
    lwc1    $f10, 36($v0)
    lw      $t7,  40($v0)
    mtc1    $t5, $f04
    mul.s   $f03, $f03, $f07
    cvt.s.w $f04, $f04
    mul.s   $f02, $f03, $f03
    mul.s   $f03, $f03, $f06
    mul.s   $f05, $f05, $f02
    mul.s   $f02, $f02, $f03
    add.s   $f05, $f05, $f09
    mul.s   $f05, $f05, $f02
    add.s   $f05, $f05, $f10
    mul.s   $f02, $f02, $f05
    and     $t7, $t7, $t6
    add.s   $f03, $f03, $f02
    nor     $t6, $t6, $t6
    add.s   $f03, $f03, $f04
    mfc1    $t1, $f03
    and     $t6, $t6, $t1
    mtc1    $t6, $f04
    mul.s   $f04, $f04, $f13
    mfc1    $t6, $f04
    or      $t6, $t6, $t7
    mtc1    $t6, $f04
    cvt.w.s $f01, $f04
    lwc1    $f05, 44($v0)
    mfc1    $t7, $f01
    cvt.s.w $f01, $f01
    lwc1    $f02, 48($v0)
    sub.s   $f04, $f04, $f01
    lwc1    $f06, 52($v0)
    mul.s   $f03, $f04, $f04
    lwc1    $f07, 56($v0)
    add.s   $f05, $f05, $f03
    mul.s   $f02, $f02, $f03
    mul.s   $f05, $f05, $f03
    add.s   $f02, $f02, $f06
    lwc1    $f06, 60($v0)
    add.s   $f05, $f05, $f07
    mul.s   $f02, $f02, $f04
    sub.s   $f05, $f05, $f02
    mul.s   $f02, $f02, $f06
    div.s   $f07, $f11, $f05
    and     $at, $t0, $at
    lw      $t0, 64($v0)
    lw      $t1, 68($v0)
    sll     $t7, $t7, 23
    pcgtw   $t2, $t6, $t0
    sra     $t4, $t6, 31
    sra     $t5, $t0, 31
    and     $t4, $t4, $t5
    mul.s   $f02, $f02, $f07
    xor     $t2, $t2, $t4
    pcgtw   $t3, $t6, $t1
    pceqw   $t4, $t6, $t0
    add.s   $f02, $f02, $f11
    pceqw   $t5, $t6, $t1
    or      $t2, $t2, $t4
    or      $t3, $t3, $t5
    lw      $t1, 72($v0)
    mfc1    $t8, $f02
    add     $t7, $t7, $t8
    and     $t1, $t1, $t3
    and     $t2, $t2, $t7
    nor     $t3, $t3, $t3
    and     $t3, $t3, $t2
    or      $t3, $t3, $t1
    xor     $t3, $t3, $at
    jr      $ra
    mtc1    $t3, $f00

memcpy_swap16:
    srl     $at, $a2, 5
    beq     $at, $zero, 1f
    andi    $a2, $a2, 31
2:
    lq      $t0,  0($a1)
    lq      $t1, 16($a1)
    addiu   $a0, $a0, 32
    addiu   $a1, $a1, 32
    addiu   $at, $at, -1
    psllh   $t2, $t0, 8
    psrlh   $t0, $t0, 8
    psllh   $t3, $t1, 8
    psrlh   $t1, $t1, 8
    por     $t0, $t0, $t2
    por     $t1, $t1, $t3
    sq      $t0, -32($a0)
    bgtz    $at, 2b
    sq      $t1, -16($a0)
1:
    beq     $a2, $zero, 3f
    nop
4:
    lhu     $t0, 0($a1)
    addiu   $a0, $a0,  2
    addiu   $a1, $a1,  2
    addiu   $a2, $a2, -2
    sll     $t1, $t0, 8
    srl     $t0, $t0, 8
    or      $t0, $t0, $t1
    bgtz    $a2, 4b
    sh      $t0, -2($a0)
3:
    jr      $ra

pcm_syn1:
    pnor  $v1, $zero, $zero
    lw    $a1, 0($a1)
    psllw $v0, $v1, 31
    psrlw $v1, $v1, 17
    psraw $v0, $v0, 16
    srl   $at, $a2, 4
    beq   $at, $zero, 1f
    andi  $a2, $a2, 15
2:
    lqc2  $vf01,  0($a1)
    lqc2  $vf02, 16($a1)
    lqc2  $vf03, 32($a1)
    lqc2  $vf04, 48($a1)
    addiu $a1, $a1, 64
    addiu $at, $at, -1
    vftoi0.xyzw   $vf01, $vf01
    vftoi0.xyzw   $vf02, $vf02
    vftoi0.xyzw   $vf03, $vf03
    vftoi0.xyzw   $vf04, $vf04
    qmfc2 $t0, $vf01
    qmfc2 $t1, $vf02
    qmfc2 $t2, $vf03
    qmfc2 $t3, $vf04
    pmaxw $t0, $v0, $t0
    pmaxw $t1, $v0, $t1
    pmaxw $t2, $v0, $t2
    pmaxw $t3, $v0, $t3
    pminw $t0, $v1, $t0
    pminw $t1, $v1, $t1
    pminw $t2, $v1, $t2
    pminw $t3, $v1, $t3
    ppach $t0, $t1, $t0
    ppach $t2, $t3, $t2
    sq    $t0,  0($a0)
    sq    $t2, 16($a0)
    bgtz  $at, 2b
    addiu $a0, $a0, 32
1:
    beq   $a2, $zero, 3f
    nop
4:
    lwc1  $f01, 0($a1)
    cvt.w.s   $f01, $f01
    addiu $a1, $a1, 4
    addiu $a0, $a0, 2
    addiu $a2, $a2, -1
    mfc1  $t0, $f01
    pmaxw $t0, $v0, $t0
    pminw $t0, $v1, $t0
    bgtz  $a2, 4b
    sh    $t0, -2($a0)
3:
    jr    $ra

pcm_syn2:
    pnor  $v1, $zero, $zero
    lw    $a3, 0($a1)
    lw    $a1, 4($a1)
    psllw $v0, $v1, 31
    psrlw $v1, $v1, 17
    psraw $v0, $v0, 16
    srl   $at, $a2, 3
    beq   $at, $zero, 1f
    andi  $a2, $a2, 7
2:
    lqc2  $vf01,  0($a3)
    lqc2  $vf02, 16($a3)
    lqc2  $vf03,  0($a1)
    lqc2  $vf04, 16($a1)
    addiu $a3, $a3, 32
    addiu $a1, $a1, 32
    vftoi0.xyzw   $vf01, $vf01
    vftoi0.xyzw   $vf02, $vf02
    vftoi0.xyzw   $vf03, $vf03
    vftoi0.xyzw   $vf04, $vf04
    addiu $at, $at, -1
    qmfc2 $t0, $vf01
    qmfc2 $t1, $vf02
    qmfc2 $t2, $vf03
    qmfc2 $t3, $vf04
    pmaxw $t0, $v0, $t0
    pmaxw $t1, $v0, $t1
    pmaxw $t2, $v0, $t2
    pmaxw $t3, $v0, $t3
    pminw $t0, $v1, $t0
    pminw $t1, $v1, $t1
    pminw $t2, $v1, $t2
    pminw $t3, $v1, $t3
    pinteh    $t0, $t2, $t0
    pinteh    $t1, $t3, $t1
    sq    $t0,  0($a0)
    sq    $t1, 16($a0)
    bgtz  $at, 2b
    addiu $a0, $a0, 32
1:
    beq   $a2, $zero, 3f
    nop
4:
    lwc1  $f01, 0($a3)
    lwc1  $f02, 0($a1)
    cvt.w.s   $f01, $f01
    cvt.w.s   $f02, $f02
    addiu $a3, $a3, 4
    addiu $a1, $a1, 4
    addiu $a0, $a0, 4
    addiu $a2, $a2, -1
    mfc1  $t0, $f01
    mfc1  $t1, $f02
    pinteh    $t0, $t1, $t0
    pmaxw $t0, $v0, $t0
    pminw $t0, $v1, $t0
    bgtz  $a2, 4b
    sw    $t0, -4($a0)
3:
    jr    $ra

pcm_synN:
    sll     $a2, $a2, 2
    lui     $t1, %hi( s_MuxConst )
    addiu   $t1, $t1, %lo( s_MuxConst )
    lw      $t5,  4($a1)                # t5 = ch1
    lw      $a3,  8($a1)                # a3 = ch2
    lw      $t0, 12($a1)                # t0 = ch3
    lqc2    $vf01,  0($t1)              # vf01 = DM_MUL
    lqc2    $vf02, 16($t1)              # vf02 = RSQRT2 * DM_MUL
    lw      $t1, 16($a1)                # t1 = ch4
    lw      $a1,  0($a1)                # a1 = ch0
    addu    $t4, $t5, $a2
    pnor    $v1, $zero, $zero
    psllw   $v0, $v1, 31
    psrlw   $v1, $v1, 17
    psraw   $v0, $v0, 16
1:
    lqc2    $vf03, 0($t5)               # vf03 = ch1
    lqc2    $vf04, 0($a1)               # vf04 = ch0
    lqc2    $vf05, 0($t0)               # vf05 = ch3
    lqc2    $vf06, 0($a3)               # vf06 = ch2
    lqc2    $vf07, 0($t1)               # vf07 = ch4
    vmula.xyzw  ACC, $vf03, $vf01
    vmadda.xyzw ACC, $vf04, $vf02
    vmadd.xyzw  $vf03, $vf05, $vf02
    vmula.xyzw  ACC, $vf06, $vf01
    vmadda.xyzw ACC, $vf04, $vf02
    vmadd.xyzw  $vf04, $vf07, $vf02
    vftoi0.xyzw $vf03, $vf03
    addiu   $t5, $t5, 16
    addiu   $a1, $a1, 16
    vftoi0.xyzw $vf04, $vf04
    addiu   $t0, $t0, 16
    addiu   $a3, $a3, 16
    addiu   $t1, $t1, 16
    qmfc2   $t2, $vf03
    qmfc2   $t3, $vf04
    pmaxw   $t2, $v0, $t2
    pmaxw   $t3, $v0, $t3
    pminw   $t2, $v1, $t2
    pminw   $t3, $v1, $t3
    pinteh  $t2, $t3, $t2
    sq      $t2, 0($a0)
    bne     $t5, $t4, 1b
    addiu   $a0, $a0, 16
    jr      $ra

strlen:
    andi    $v0, $a0, 7
    bnez    $v0, 4f
    addu    $a3, $zero, $a0
    andi    $v1, $a0, 15
    ori     $v0, $zero, 0x0101
    addu    $a1, $zero, $a0
    bnez    $v1, 2f
    pcpyh   $v0, $v0
    lq      $v1, 0($a1)
    pcpyld  $t0, $v0, $v0
    ori     $a0, $zero, 0x8080
    psubb   $v0, $v1, $t0
    pcpyh   $a0, $a0
    pnor    $v1, $zero, $v1
    pcpyld  $t1, $a0, $a0
    pand    $v0, $v0, $v1
    pand    $v0, $v0, $t1
    pcpyud  $v1, $v0, $t0
    or      $a2, $v1, $v0
    bnezl   $a2, 4f
    addu    $a0, $zero, $a1
    addiu   $a1, $a1, 16
1:
    lq      $v0, 0($a1)
    pnor    $v1, $zero, $v0
    psubb   $v0, $v0, $t0
    pand    $v0, $v0, $v1
    pand    $a0, $v0, $t1
    pcpyud  $v1, $a0, $a2
    or      $v1, $v1, $a0
    beqzl   $v1, 1b
    addiu   $a1, $a1, 16
    beq     $zero, $zero, 4f
    addu    $a0, $zero, $a1
2:
    ld      $v1, 0($a1)
    ori     $a0, $zero, 0x8080
    dsubu   $v0, $v1, $v0
    pcpyh   $a0, $a0
    nor     $v1, $zero, $v1
    and     $v0, $v0, $v1
    and     $v0, $v0, $a0
    bnezl   $v0, 4f
    addu    $a0, $zero, $a1
    ori     $a2, $zero, 0x0101
    addiu   $a1, $a1, 8
    pcpyh   $a2, $a2
3:
    ld      $v0, 0($a1)
    nor     $v1, $zero, $v0
    dsubu   $v0, $v0, $a2
    and     $v0, $v0, $v1
    and     $v0, $v0, $a0
    beqzl   $v0, 3b
    addiu   $a1, $a1, 8
    addu    $a0, $zero, $a1
4:
    lb      $v0, 0($a0)
    nop
    nop
    nop
    nop
    bnezl   $v0, 4b
    addiu   $a0, $a0, 1
    jr      $ra
    subu    $v0, $a0, $a3

strcat:
    addiu   $sp, $sp, -32
    sq      $s0,  0($sp)
    addu    $s0, $zero, $a0
    andi    $v0, $s0,0x7
    bnez    $v0, 4f
    sw      $ra, 16($sp)
    andi    $v0, $s0, 15
    ori     $v1, $zero, 0x0101
    ori     $a0, $zero, 0x8080
    pcpyh   $v1, $v1
    pcpyh   $a0, $a0
    bnez    $v0, 2f
    ld      $a2, 0($s0)
    lq      $v0, 0($s0)
    pcpyld  $a3, $v1, $v1
    pcpyld  $t0, $a0, $a0
    psubb   $v1, $v0, $a3
    pnor    $v0, $zero, $v0
    pand    $v1, $v1, $v0
    pand    $v1, $v1, $t0
    pcpyud  $v0, $v1, $v1
    or      $v1, $v0, $v1
    bnez    $v1, 4f
    addu    $a0, $zero, $s0
    addiu   $a2, $a0, 16
1:
    lq      $v0, 0($a2)
    pnor    $v1, $zero, $v0
    psubb   $v0, $v0, $a3
    pand    $v0, $v0, $v1
    pand    $v0, $v0, $t0
    pcpyud  $v1, $v0, $v0
    or      $v0, $v0, $v1
    beqzl   $v0, 1b
    addiu   $a2, $a2, 16
    beq     $zero, $zero, 4f
    addu    $a0, $zero, $a2

2:
    addu    $a3, $zero, $v1
    addu    $t0, $zero, $a0
    dsubu   $v1, $a2, $v1
    nor     $v0, $zero, $a2
    and     $v1, $v1, $v0
    and     $v1, $v1, $a0
    bnez    $v1, 4f
    addu    $a0, $zero, $s0
    addiu   $a2, $s0, 8
3:
    ld      $v0, 0($a2)
    nor     $v1, $zero, $v0
    dsubu   $v0, $v0, $a3
    and     $v0, $v0, $v1
    and     $v0, $v0, $t0
    beqzl   $v0, 3b
    addiu   $a2, $a2, 8
    addu    $a0, $zero, $a2
4:
    lb      $v0, 0($a0)
    nop
    nop
    nop
    nop
    bnezl   $v0, 4b
    addiu   $a0, $a0, 1
    bgezal  $zero, _strcpy
    nop
    addu    $v0, $zero, $s0
    lw      $ra, 16($sp)
    lq      $s0,  0($sp)
    jr      $ra
    addiu   $sp, $sp, 32

strcpy :
_strcpy:
    addu    $a3, $zero, $a0
    or      $t0, $a1, $a3
    andi    $v0, $t0, 7
    bnez    $v0, 5f
    addu    $v1, $zero, $a3
    andi    $v0, $t0, 15
    ori     $t1, $zero, 0x0101
    ori     $a0, $zero, 0x8080
    pcpyh   $t1, $t1
    pcpyh   $a0, $a0
    bnezl   $v0, 2f
    ld      $t2, 0($a1)
    pcpyld  $t2, $t1, $t1
    lq      $t1, 0($a1)
    pcpyld  $t0, $a0, $a0
    psubb   $v0, $t1, $t2
    pnor    $v1, $zero, $t1
    pand    $v0, $v0, $v1
    pand    $v0, $v0, $t0
    pcpyud  $a0, $v0, $t1
    or      $v1, $v0, $a0
    bnez    $v1, 4f
    addu    $a2, $zero, $a3
1:
    sq      $t1, 0($a2)
    addiu   $a1, $a1, 16
    lq      $t1, 0($a1)
    psubb   $v0, $t1, $t2
    pnor    $v1, $zero, $t1
    pand    $v0, $v0, $v1
    pand    $v0, $v0, $t0
    pcpyud  $a0, $v0, $t1
    or      $v1, $v0, $a0
    beqz    $v1, 1b
    addiu   $a2, $a2, 16
    beq     $zero, $zero, 5f
    addu    $v1, $zero, $a2
2:
    dsubu   $v0, $t2, $t1
    nor     $v1, $zero, $t2
    and     $v0, $v0, $v1
    and     $v0, $v0, $a0
    bnez    $v0, 4f
    addu    $a2, $zero, $a3
3:
    sd      $t2, 0($a2)
    addiu   $a1, $a1, 8
    ld      $t2, 0($a1)
    nor     $v0, $zero, $t2
    dsubu   $v1, $t2, $t1
    and     $v1, $v1, $v0
    and     $v1, $v1, $a0
    beqz    $v1, 3b
    addiu   $a2, $a2, 8
4:
    addu    $v1, $zero, $a2
5:
    lbu     $v0, 0($a1)
    addiu   $a1, $a1, 1
    sb      $v0, 0($v1)
    sll     $v0, $v0, 24
    addiu   $v1, $v1, 1
    bnez    $v0, 5b
    nop
    jr      $ra
    addu    $v0, $zero, $a3

strncmp:
    bnez    $a2, 2f
    or      $v1, $a0, $a1
1:
    jr      $ra
    xor     $v0, $v0, $v0
2:
    andi    $v0, $v1, 7
    bnez    $v0, 8f
    addu    $v0, $zero, $a2
    andi    $v0, $v1, 15
    sltiu   $a3, $a2, 16
    ori     $t1, $zero, 0x0101
    or      $v0, $v0, $a3
    pcpyh   $t1, $t1
    bnez    $v0, 4f
    addu    $a3, $zero, $a0
    lq      $v1, 0($a0)
    pcpyld  $t2, $t1, $t1
    lq      $v0, 0($a1)
    ori     $t0, $zero, 0x8080
    pcpyh   $t0, $t0
    psubw   $v1, $v1, $v0
    pcpyld  $t1, $t0, $t0
    pcpyud  $v0, $v1, $a0
    addu    $t0, $zero, $a1
    or      $v1, $v0, $v1
    bnez    $v1, 8f
    addu    $v0, $zero, $a2
    addiu   $a2, $a2, -16
3:
    beqz    $a2, 1b
    nop
    lq      $v0, 0($a3)
    pnor    $v1, $zero, $v0
    psubb   $v0, $v0, $t2
    pand    $v0, $v0, $v1
    pand    $v1, $v0, $t1
    pcpyud  $v0, $v1, $a0
    or      $v0, $v0, $v1
    bnez    $v0, 1b
    addiu   $a3, $a3, 16
    sltiu   $v0, $a2, 16
    lq      $v1, 0($a3)
    bnez    $v0, 6f
    addiu   $t0, $t0, 16
    lq      $v0, 0($t0)
    psubw   $v1, $v1, $v0
    pcpyud  $v0, $v1, $a0
    or      $v0, $v0, $v1
    beqzl   $v0, 3b
    addiu   $a2, $a2, -16
    beq     $zero, $zero, 7f
    addu    $a0, $zero, $a3
4:
    sltiu   $v0, $a2, 8
    bnez    $v0, 6f
    addu    $t0, $zero, $a1
    ld      $v1, 0($a0)
    ld      $v0, 0($a1)
    bne     $v1, $v0, 8f
    addu    $v0, $zero, $a2
    addiu   $a2, $a2, -8
    ori     $t2, $zero, 0x8080
    pcpyh   $t2, $t2
5:
    beqz    $a2, 1b
    nop
    ld      $v0, 0($a3)
    nor     $v1, $zero, $v0
    dsubu   $v0, $v0, $t1
    and     $v0, $v0, $v1
    and     $v0, $v0, $t2
    bnez    $v0, 1b
    addiu   $a3, $a3, 8
    sltiu   $v0, $a2, 8
    bnez    $v0, 6f
    addiu   $t0, $t0, 8
    ld      $v1, 0($a3)
    ld      $v0, 0($t0)
    beql    $v1, $v0, 5b
    addiu   $a2, $a2, -8
6:
    addu    $a0, $zero, $a3
7:
    addu    $a1, $zero, $t0
    addu    $v0, $zero, $a2
8:
    beqz    $v0, 11f
    addiu   $a2, $a2, -1
    beq     $zero, $zero, 10f
    lb      $v1, 0($a0)
    nop
9:
    beqz    $a2, 1b
    nop
    beqz    $a3, 1b
    addiu   $a0, $a0, 1
    addiu   $a1, $a1, 1
    lb      $v1, 0($a0)
    addiu   $a2, $a2, -1
10:
    lb      $v0, 0($a1)
    beq     $v1, $v0, 9b
    lbu     $a3, 0($a0)
11:
    lbu     $v0, 0($a1)
    lbu     $v1, 0($a0)
    jr      $ra
    subu    $v0, $v1, $v0

strncpy:
    addu    $t0, $zero, $a0
    or      $a3, $a1, $a0
    addiu   $t2, $zero, 16
    andi    $v0, $a3, 7
    addiu   $t1, $zero, 8
    bnez    $v0, 4f
    andi    $v0, $a3, 15
    movz    $t1, $t2, $v0
    bnez    $v0, 1f
    sltu    $v0, $a2, $t1
    bnez    $v0, 4f
    nop
    ori     $a3, $zero, 0x0101
    lq      $v1, 0($a1)
    pcpyh   $a3, $a3
    pcpyld  $t1, $a3, $a3
    pnor    $v1, $zero, $v1
    ori     $a3, $zero, 0x8080
    pcpyh   $a3, $a3
    psubb   $v0, $v1, $t1
    pcpyld  $t2, $a3, $a3
    pand    $v0, $v0, $v1
    pand    $v0, $v0, $t2
    pcpyud  $v1, $v0, $a0
    or      $v1, $v0, $v1
    bnez    $v1, 3f
    addu    $a3, $zero, $t0
    lq      $v1, 0($a1)
    addiu   $a2, $a2, -16
    addiu   $a1, $a1, 16
    sltiu   $v0, $a2, 16
    sq      $v1, 0($a3)
    bnez    $v0, 3f
    addiu   $a3, $a3, 16
    lq      $v0, 0($a1)
    pnor    $v1, $zero, $v0
    psubb   $v0, $v0, $t1
    pand    $v0, $v0, $v1
    pand    $v0, $v0, $t2
    pcpyud  $v1, $v0, $a0
    or	    $v0, $v0, $v1
    beqzl   $v0, 2f
    lq      $v1, 0($a1)
    beq     $zero, $zero, 4f
    addu    $a0, $zero, $a3
1:
    bnez    $v0, 4f
    nop
    ld      $v1, 0($a1)
    ori     $t1, $zero, 0x0101
    ori     $t2, $zero, 0x8080
    pcpyh   $t1, $t1
    pcpyh   $t2, $t2
    dsubu   $v0, $v1, $t1
    nor     $v1, $zero, $v1
    and     $v0, $v0, $v1
    and     $v0, $v0, $t2
    bnez    $v0, 3f
    addu    $a3, $zero, $t0
    ld      $v1, 0($a1)
2:
    addiu   $a2, $a2, -8
    addiu   $a1, $a1, 8
    sltiu   $v0, $a2, 8
    sd      $v1, 0($a3)
    bnez    $v0, 3f
    addiu   $a3, $a3, 8
    ld      $v0, 0($a1)
    nor     $v1, $zero, $v0
    dsubu   $v0, $v0, $t1
    and     $v0, $v0, $v1
    and     $v0, $v0, $t2
    beqzl   $v0, 2b
    ld      $v1, 0($a1)
3:
    addu    $a0, $zero, $a3
4:
    beqz    $a2, 6f
    addu    $v0, $zero, $a2
    lbu     $v0, 0($a1)
    addiu   $a2, $a2, -1
    addiu   $a1, $a1, 1
    sb      $v0, 0($a0)
    sll     $v0, $v0, 24
    bnez    $v0, 4b
    addiu   $a0, $a0, 1
    addu    $v0, $zero, $a2
    beqz    $v0, 6f
    addiu   $a2, $a2, -1
5:
    sb      $zero, 0($a0)
    addu    $v0, $zero, $a2
    addiu   $a0, $a0, 1
    addiu   $a2, $a2, -1
    nop
    bnez    $v0, 5b
    nop
6:
    jr      $ra
    addu    $v0, $zero, $t0

strchr:
_strchr:
    andi    $v0, $a0, 7
    bnez    $v0, 5f
    andi    $a1, $a1, 0xFF
    dsll    $v1, $a1, 8
    lui     $a2, 0x0101
    ori     $a2, $zero, 0x0101
    daddu   $t2, $v1, $a1
    pcpyh   $a2, $a2
    andi    $v1, $a0, 15
    dsll    $v0, $t2, 16
    ori     $t0, $zero, 0x8080
    daddu   $v0, $v0, $t2
    pcpyh   $t0, $t0
    dsll32  $t2, $v0, 0
    bnez    $v1, 2f
    daddu   $a3, $v0, $t2
1:
    lq      $t1, 0($a0)
    pcpyld  $t2, $a2, $a2
    pnor    $v1, $zero, $t1
    psubb   $v0, $t1, $t2
    pcpyld  $a2, $t0, $t0
    pand    $v0, $v0, $v1
    pcpyld  $t0, $a3, $a3
    pand    $v0, $v0, $a2
    pcpyud  $v1, $v0, $a3
    or      $v1, $v0, $v1
    bnezl   $v1, 6f
    lbu     $v0, 0($a0)
    pxor    $v0, $t1, $t0
    psubb   $v1, $v0, $t2
    pnor    $v0, $zero, $v0
    ori     $t0, $zero, 0x8080
    pand    $v1, $v1, $v0
    pcpyh   $t0, $t0
    pand    $v1, $v1, $a2
    ori     $a2, $zero, 0x0101
    pcpyud  $v0, $v1, $a1
    pcpyh   $a2, $a2
    or      $v1, $v0, $v1
    beqzl   $v1, 1b
    addiu   $a0, $a0, 16
    beq     $zero, $zero, 6f
    lbu     $v0, 0($a0)
2:
    ld      $t1, 0($a0)
    nor     $v1, $zero, $t1
    dsubu   $v0, $t1, $a2
    and     $v0, $v0, $v1
    and     $v0, $v0, $t0
    bnezl   $v0, 6f
    lbu     $v0, 0($a0)
    xor     $v0, $t1, $a3
    dsubu   $v1, $v0, $a2
    nor     $v0, $zero, $v0
    and     $v1, $v1, $v0
    and     $v1, $v1, $t0
    bnezl   $v1, 6f
    lbu     $v0, 0($a0)
    addu    $t1, $zero, $a2
    addiu   $a0, $a0, 8
3:
    ld      $a2, 0($a0)
    dsubu   $v0, $a2, $t1
    nor     $v1, $zero, $a2
    and     $v0, $v0, $v1
    and     $v0, $v0, $t0
    bnez    $v0, 5f
    xor     $v0, $a2, $a3
    nor     $v1, $zero, $v0
    dsubu   $v0, $v0, $t1
    and     $v0, $v0, $v1
    and     $v0, $v0, $t0
    beqzl   $v0, 3b
    addiu   $a0, $a0, 8
    beq     $zero, $zero, 6f
    lbu     $v0, 0($a0)

4:
    beql    $v0, $a1, 7f
    lbu     $v1, 0($a0)
    addiu   $a0, $a0, 1
5:
    lbu     $v0, 0($a0)
6:
    bnez    $v0, 4b
    nop
    lbu     $v1, 0($a0)
7:
    xor     $v0, $v0, $v0
    xor     $v1, $v1, $a1
    jr      $ra
    movz    $v0, $a0, $v1

strcmp:
    or      $t0, $a0, $a1
    andi    $v0, $t0, 7
    bnezl   $v0, 7f
    lb      $v0, 0($a0)
    andi    $t1, $t0, 15
    ori     $a3, $zero, 0x0101
    ori     $a2, $zero, 0x8080
    pcpyh   $a3, $a3
    pcpyh   $a2, $a2
    bnez    $t1, 3f
    ld      $v0, 0($a1)
    lq      $v1, 0($a0)
    pcpyld  $t0, $a3, $a3
    lq      $v0, 0($a1)
    pcpyld  $t2, $a2, $a2
    psubw   $a3, $v0, $v1
    pcpyud  $a2, $a3, $a0
    or      $v1, $a2, $a3
    bnezl   $v1, 7f
    lb      $v0, 0($a0)
    lq      $v0, 0($a0)
    pnor    $v1, $zero, $v0
1:
    psubb   $v0, $v0, $t0
    pand    $v0, $v0, $v1
    pand    $v0, $v0, $t2
    pcpyud  $v1, $v0, $a0
    or      $a2, $v1, $v0
    beqz    $a2, 2f
    addiu   $a0, $a0, 16
    jr      $ra
    xor     $v0, $v0, $v0
2:
    addiu   $a1, $a1, 16
    lq      $v0, 0($a0)
    lq      $v1, 0($a1)
    psubw   $a3, $v0, $v1
    pcpyud  $a2, $a3, $a0
    or      $t1, $a2, $a3
    beqzl   $t1, 1b
    pnor    $v1, $zero, $v0
    beq     $zero, $zero, 7f
    lb      $v0, 0($a0)
3:
    ld      $v1, 0($a0)
    bnel    $v1, $v0, 7f
    lb      $v0, 0($a0)
    ld      $v0, 0($a0)
    nor     $t0, $zero, $v0
4:
    dsubu   $v0, $v0, $a3
    and     $v0, $v0, $t0
    and     $v0, $v0, $a2
    beqz    $v0, 5f
    addiu   $a0, $a0, 8
    jr      $ra
    xor     $v0, $v0, $v0
5:
    addiu   $a1, $a1, 8
    ld      $v0, 0($a0)
    ld      $v1, 0($a1)
    beql    $v1, $v0, 4b
    nor     $t0, $zero, $v0
    beq     $zero, $zero, 7f
    lb      $v0, 0($a0)
6:
    sll     $v0, $v1, 24
    lb      $v1, 0($a1)
    sra     $v0, $v0, 24
    bnel    $v0, $v1, 8f
    lbu     $v1, 0($a0)
    addiu   $a0, $a0, 1
    addiu   $a1, $a1, 1
    lb      $v0, 0($a0)
7:
    bnez    $v0, 6b
    lbu     $v1, 0($a0)
8:
    lbu     $v0, 0($a1)
    jr      $ra
    subu    $v0, $v1, $v0

strrchr:
    addu    $v0, $zero, $a0
    pcpyld  $ra, $ra, $ra
    pcpyld  $s0, $s0, $s0
    pcpyld  $s1, $s1, $s1
    addu    $s0, $zero, $a1
    beq     $a1, $zero, 1f
    xor     $s1, $s1, $s1
3:
    addu    $a0, $zero, $v0
    bgezal  $zero, _strchr
    addu    $a1, $zero, $s0
    bnezl   $v0, 2f
    addu    $s1, $zero, $v0
4:
    addu    $v0, $zero, $s1
    pcpyud  $ra, $ra, $ra
    pcpyud  $s0, $s0, $s0
    jr      $ra
    pcpyud  $s1, $s1, $s1
2:
    beq     $zero, $zero, 3b
    addiu   $v0, $v0, 1
1:
    bgezal  $zero, _strchr
    nop
    beq     $zero, $zero, 4b
    addu    $s1, $zero, $v0

strstr:
    lb      $t7, 0($a0)
    bnez    $t7, 1f
    addu    $v0, $zero, $a0
    lb      $t7, 0($a1)
    jr      $ra
    movn    $v0, $zero, $t7
1:
    xor     $t4, $t4, $t4
3:
    addu    $t7, $a1, $t4
    lb      $t5, 0($t7)
    beqz    $t5, 2f
    addu    $t6, $v0, $t4
    lb      $t7, 0($t6)
    beq     $t5, $t7, 3b
    addiu   $t4, $t4, 1
    addiu   $v0, $v0, 1
    lb      $t7, 0($v0)
    bnezl   $t7, 3b
    xor     $t4, $t4, $t4
    xor     $v0, $v0, $v0
2:
    jr      $ra
    nop

strpbrk:
    lbu     $t5, 0($a0)
    addu    $t4, $zero, $a1
    beqz    $t5, 1f
    xor     $v0, $v0, $v0
6:
    lbu     $t6, 0($t4)
    addu    $a1, $zero, $t4
    beqz    $t6, 2f
    addu    $t7, $zero, $t6
    sll     $t5, $t5, 24
    sra     $t5, $t5, 24
4:
    sll     $t7, $t7, 24
    sra     $t7, $t7, 24
    beql    $t5, $t7, 3f
    sll     $v0, $t6, 24
    addiu   $a1, $a1, 1
    lbu     $t6, 0($a1)
    bnez    $t6, 4b
    addu    $t7, $zero, $t6
2:
    sll     $v0, $t6, 24
3:
    sra     $v0, $v0, 24
    bnezl   $v0, 5f
    movz    $a0, $zero, $v0
    addiu   $a0, $a0, 1
    lbu     $t7, 0($a0)
    bnez    $t7, 6b
    addu    $t5, $zero, $t7
    movz    $a0, $zero, $v0
5:
    addu    $v0, $zero, $a0
1:
    jr      $ra
    nop
