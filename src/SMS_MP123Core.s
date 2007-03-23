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
.set noreorder
.set nomacro
.set noat

.globl MP123_CoreInit
.globl MP123_IMDCT36
.globl MP123_IMDCT12
.globl MP123_Synth

.sdata
.align 4
s_Cos64:    .word   0x3F002785, 0x3F01668B, 0x3F03F45B, 0x3F07F268
            .word   0x3F0D9838, 0x3F153B3A, 0x3F1F5C6E, 0x3F2CC03D
            .word   0x3F3E99EE, 0x3F56DF9E, 0x3F78FA3B, 0x3F95B035
            .word   0x3FBDF91B, 0x4003B2AF, 0x405A1642, 0x41230A46
            .word   0x3F009E8D, 0x3F05C278, 0x3F11233F, 0x3F25961D
            .word   0x3F49C480, 0x3F87C449, 0x3FDC7926, 0x40A33C9C
            .word   0x3F0281F7, 0x3F19F1BD, 0x3F6664D7, 0x402406CF
            .word   0x3F0A8BD4, 0x3FA73D75, 0x3F3504F3, 0x00000000
s_Cos36:    .float  0.866025,  0.939693, -0.173648,  -0.766044
            .float  0.500000,  0.984808, -0.342020,  -0.642788
            .float  0.501910,  0.517638,  0.551689,   0.610387
            .float  0.707107,  0.871723,  1.183100,   1.931850
            .float  5.736860,  0.000000,  0.000000,   0.000000
s_Cos12:    .float  0.866025,  0.866025,  0.866025,   0.000000
            .float  0.707107,  0.707107,  0.707107,   0.000000
            .float  0.500000,  0.500000,  0.500000,   0.000000
            .float  0.517638,  0.517638,  0.517638,   0.000000
            .float  1.931850,  1.931850,  1.931850,   0.000000
s_DecWin:   .word   0x80000000, 0x41680000, 0xC2D50000, 0x43658000
            .word   0xC47EA000, 0x45210800, 0xC54D7000, 0x46927100
            .word   0xC7128F00, 0xC6927100, 0xC54D7000, 0xC5210800
            .word   0xC47EA000, 0xC3658000, 0xC2D50000, 0xC1680000
            .word   0x80000000, 0x41680000, 0xC2D50000, 0x43658000
            .word   0xC47EA000, 0x45210800, 0xC54D7000, 0x46927100
            .word   0xC7128F00, 0xC6927100, 0xC54D7000, 0xC5210800
            .word   0xC47EA000, 0xC3658000, 0xC2D50000, 0xC1680000
            .word   0x3F000000, 0x41780000, 0xC2DA0000, 0x4381C000
            .word   0xC47A0000, 0x452C6800, 0xC53A3800, 0x4699A800
            .word   0xC7127800, 0xC68B3800, 0xC55EF000, 0xC515A000
            .word   0xC480F000, 0xC3488000, 0xC2D00000, 0xC1500000
            .word   0x3F000000, 0x41780000, 0xC2DA0000, 0x4381C000
            .word   0xC47A0000, 0x452C6800, 0xC53A3800, 0x4699A800
            .word   0xC7127800, 0xC68B3800, 0xC55EF000, 0xC515A000
            .word   0xC480F000, 0xC3488000, 0xC2D00000, 0xC1500000
            .word   0x3F000000, 0x418C0000, 0xC2DE0000, 0x43914000
            .word   0xC4740000, 0x4537B800, 0xC5254000, 0x46A0D800
            .word   0xC7123400, 0xC683FF00, 0xC56EC000, 0xC50A4800
            .word   0xC4820000, 0xC32D8000, 0xC2CA0000, 0xC1400000
            .word   0x3F000000, 0x418C0000, 0xC2DE0000, 0x43914000
            .word   0xC4740000, 0x4537B800, 0xC5254000, 0x46A0D800
            .word   0xC7123400, 0xC683FF00, 0xC56EC000, 0xC50A4800
            .word   0xC4820000, 0xC32D8000, 0xC2CA0000, 0xC1400000
            .word   0x3F000000, 0x41980000, 0xC2E10000, 0x43A14000
            .word   0xC46CA000, 0x4542E800, 0xC50E8800, 0x46A7FE00
            .word   0xC711C300, 0xC6799600, 0xC57CE000, 0xC4FDF000
            .word   0xC4827000, 0xC3130000, 0xC2C40000, 0xC1280000
            .word   0x3F000000, 0x41980000, 0xC2E10000, 0x43A14000
            .word   0xC46CA000, 0x4542E800, 0xC50E8800, 0x46A7FE00
            .word   0xC711C300, 0xC6799600, 0xC57CE000, 0xC4FDF000
            .word   0xC4827000, 0xC3130000, 0xC2C40000, 0xC1280000
            .word   0x3F000000, 0x41A40000, 0xC2E30000, 0x43B1C000
            .word   0xC463C000, 0x454DE800, 0xC4EC0000, 0x46AF1500
            .word   0xC7112480, 0xC66B4000, 0xC584B000, 0xC4E79000
            .word   0xC4825000, 0xC2F40000, 0xC2BE0000, 0xC1180000
            .word   0x3F000000, 0x41A40000, 0xC2E30000, 0x43B1C000
            .word   0xC463C000, 0x454DE800, 0xC4EC0000, 0x46AF1500
            .word   0xC7112480, 0xC66B4000, 0xC584B000, 0xC4E79000
            .word   0xC4825000, 0xC2F40000, 0xC2BE0000, 0xC1180000
            .word   0x3F000000, 0x41B40000, 0xC2E40000, 0x43C2C000
            .word   0xC4596000, 0x4558B800, 0xC4B77000, 0x46B61900
            .word   0xC7105A00, 0xC65D0200, 0xC58A2000, 0xC4D17000
            .word   0xC481B000, 0xC2C50000, 0xC2B70000, 0xC1080000
            .word   0x3F000000, 0x41B40000, 0xC2E40000, 0x43C2C000
            .word   0xC4596000, 0x4558B800, 0xC4B77000, 0x46B61900
            .word   0xC7105A00, 0xC65D0200, 0xC58A2000, 0xC4D17000
            .word   0xC481B000, 0xC2C50000, 0xC2B70000, 0xC1080000
            .word   0x3F000000, 0x41C40000, 0xC2E40000, 0x43D40000
            .word   0xC44D8000, 0x45633800, 0xC47EA000, 0x46BD0600
            .word   0xC70F6380, 0xC64EE400, 0xC58ECC00, 0xC4BBC000
            .word   0xC4809000, 0xC2990000, 0xC2B00000, 0xC1000000
            .word   0x3F000000, 0x41C40000, 0xC2E40000, 0x43D40000
            .word   0xC44D8000, 0x45633800, 0xC47EA000, 0x46BD0600
            .word   0xC70F6380, 0xC64EE400, 0xC58ECC00, 0xC4BBC000
            .word   0xC4809000, 0xC2990000, 0xC2B00000, 0xC1000000
            .word   0x3F800000, 0x41D40000, 0xC2E30000, 0x43E5C000
            .word   0xC43FE000, 0x456D6800, 0xC4074000, 0x46C3D900
            .word   0xC70E4180, 0xC640EC00, 0xC592B400, 0xC4A67000
            .word   0xC47E0000, 0xC25E0000, 0xC2A90000, 0xC0E00000
            .word   0x3F800000, 0x41D40000, 0xC2E30000, 0x43E5C000
            .word   0xC43FE000, 0x456D6800, 0xC4074000, 0x46C3D900
            .word   0xC70E4180, 0xC640EC00, 0xC592B400, 0xC4A67000
            .word   0xC47E0000, 0xC25E0000, 0xC2A90000, 0xC0E00000
            .word   0x3F800000, 0x41E80000, 0xC2E00000, 0x43F7C000
            .word   0xC430C000, 0x45773000, 0xC20C0000, 0x46CA8D00
            .word   0xC70CF480, 0xC6332200, 0xC595E000, 0xC491A000
            .word   0xC47A2000, 0xC2100000, 0xC2A10000, 0xC0D00000
            .word   0x3F800000, 0x41E80000, 0xC2E00000, 0x43F7C000
            .word   0xC430C000, 0x45773000, 0xC20C0000, 0x46CA8D00
            .word   0xC70CF480, 0xC6332200, 0xC595E000, 0xC491A000
            .word   0xC47A2000, 0xC2100000, 0xC2A10000, 0xC0D00000
            .word   0x3F800000, 0x41FC0000, 0xC2DD0000, 0x44050000
            .word   0xC4200000, 0x45804400, 0x43F98000, 0x46D11E00
            .word   0xC70B7E00, 0xC6258A00, 0xC5985800, 0xC47AC000
            .word   0xC4754000, 0xC1900000, 0xC29A0000, 0xC0B00000
            .word   0x3F800000, 0x41FC0000, 0xC2DD0000, 0x44050000
            .word   0xC4200000, 0x45804400, 0x43F98000, 0x46D11E00
            .word   0xC70B7E00, 0xC6258A00, 0xC5985800, 0xC47AC000
            .word   0xC4754000, 0xC1900000, 0xC29A0000, 0xC0B00000
            .word   0x3F800000, 0x42080000, 0xC2D70000, 0x440E2000
            .word   0xC40D6000, 0x4584AC00, 0x4484A000, 0x46D78A00
            .word   0xC709DF00, 0xC6182C00, 0xC59A1C00, 0xC4538000
            .word   0xC46FE000, 0xBF800000, 0xC2930000, 0xC0A00000
            .word   0x3F800000, 0x42080000, 0xC2D70000, 0x440E2000
            .word   0xC40D6000, 0x4584AC00, 0x4484A000, 0x46D78A00
            .word   0xC709DF00, 0xC6182C00, 0xC59A1C00, 0xC4538000
            .word   0xC46FE000, 0xBF800000, 0xC2930000, 0xC0A00000
            .word   0x3FC00000, 0x42120000, 0xC2D00000, 0x44174000
            .word   0xC3F28000, 0x4588CC00, 0x44CE4000, 0x46DDCA00
            .word   0xC7081780, 0xC60B0E00, 0xC59B3C00, 0xC42D8000
            .word   0xC469C000, 0x41680000, 0xC28B0000, 0xC0900000
            .word   0x3FC00000, 0x42120000, 0xC2D00000, 0x44174000
            .word   0xC3F28000, 0x4588CC00, 0x44CE4000, 0x46DDCA00
            .word   0xC7081780, 0xC60B0E00, 0xC59B3C00, 0xC42D8000
            .word   0xC469C000, 0x41680000, 0xC28B0000, 0xC0900000
            .word   0x3FC00000, 0x421E0000, 0xC2C80000, 0x44206000
            .word   0xC3C68000, 0x458C9800, 0x450DA800, 0x46E3DD00
            .word   0xC7062A00, 0xC5FC6C00, 0xC59BB800, 0xC408E000
            .word   0xC4632000, 0x41E40000, 0xC2840000, 0xC0800000
            .word   0x3FC00000, 0x421E0000, 0xC2C80000, 0x44206000
            .word   0xC3C68000, 0x458C9800, 0x450DA800, 0x46E3DD00
            .word   0xC7062A00, 0xC5FC6C00, 0xC59BB800, 0xC408E000
            .word   0xC4632000, 0x41E40000, 0xC2840000, 0xC0800000
            .word   0x40000000, 0x422A0000, 0xC2BD0000, 0x44298000
            .word   0xC3974000, 0x45900C00, 0x4535D000, 0x46E9BE00
            .word   0xC7041680, 0xC5E35000, 0xC59B9C00, 0xC3CB8000
            .word   0xC45BE000, 0x42260000, 0xC27A0000, 0xC0600000
            .word   0x40000000, 0x422A0000, 0xC2BD0000, 0x44298000
            .word   0xC3974000, 0x45900C00, 0x4535D000, 0x46E9BE00
            .word   0xC7041680, 0xC5E35000, 0xC59B9C00, 0xC3CB8000
            .word   0xC45BE000, 0x42260000, 0xC27A0000, 0xC0600000
            .word   0x40000000, 0x42360000, 0xC2B10000, 0x44328000
            .word   0xC3490000, 0x45932000, 0x455F9000, 0x46EF6900
            .word   0xC701DF00, 0xC5CAD000, 0xC59AF000, 0xC3884000
            .word   0xC4544000, 0x42540000, 0xC26A0000, 0xC0600000
            .word   0x40000000, 0x42360000, 0xC2B10000, 0x44328000
            .word   0xC3490000, 0x45932000, 0x455F9000, 0x46EF6900
            .word   0xC701DF00, 0xC5CAD000, 0xC59AF000, 0xC3884000
            .word   0xC4544000, 0x42540000, 0xC26A0000, 0xC0600000
            .word   0x40200000, 0x42420000, 0xC2A30000, 0x443B4000
            .word   0xC2B90000, 0x4595C400, 0x45857000, 0x46F4DC00
            .word   0xC6FF0A00, 0xC5B2FC00, 0xC599B800, 0xC3100000
            .word   0xC44C4000, 0x427E0000, 0xC25E0000, 0xC0400000
            .word   0x40200000, 0x42420000, 0xC2A30000, 0x443B4000
            .word   0xC2B90000, 0x4595C400, 0x45857000, 0x46F4DC00
            .word   0xC6FF0A00, 0xC5B2FC00, 0xC599B800, 0xC3100000
            .word   0xC44C4000, 0x427E0000, 0xC25E0000, 0xC0400000
            .word   0x40200000, 0x42500000, 0xC2920000, 0x4443E000
            .word   0x41B40000, 0x4597FC00, 0x459BDC00, 0x46FA1300
            .word   0xC6FA1300, 0xC59BDC00, 0xC597FC00, 0xC1B40000
            .word   0xC443E000, 0x42920000, 0xC2500000, 0xC0200000
            .word   0x40200000, 0x42500000, 0xC2920000, 0x4443E000
            .word   0x41B40000, 0x4597FC00, 0x459BDC00, 0x46FA1300
            .word   0xC6FA1300, 0xC59BDC00, 0xC597FC00, 0xC1B40000
            .word   0xC443E000, 0x42920000, 0xC2500000, 0xC0200000

.section ".sbss"
.align 4
s_SynthBuf: .space  4352
s_Offset  : .space  4

.text

MP123_CoreInit:
    sw      $zero, s_Offset
    la      $a0, s_SynthBuf
    addu    $a1, $zero, $zero
    j       memset
    addiu   $a2, $zero, 4352

MP123_IMDCT36:
    addiu   $sp, $sp, -16
    sd      $s0, 0($sp)
    sd      $s1, 8($sp)
    pref    0, 0($a0)
    la      $at, s_Cos36
    pref    0, 0($at)
    ld      $t1,  0($a0)
    ld      $t2,  8($a0)
    ld      $t3, 16($a0)
    ld      $t4, 24($a0)
    pcpyld  $t1, $t2, $t1
    pcpyld  $t3, $t4, $t3
    ld      $t2, 32($a0)
    ld      $t4, 40($a0)
    ld      $t5, 48($a0)
    ld      $t6, 56($a0)
    pcpyld  $t2, $t4, $t2
    pcpyld  $t5, $t6, $t5
    qmtc2   $t1, $vf01
    ld      $t1, 64($a0)
    qmtc2   $t3, $vf02
    ld      $t3, 72($a0)
    qmtc2   $t2, $vf03
    qmtc2   $t5, $vf04
    pcpyld  $t1, $t3, $t1
    lqc2    $vf29,  0($at)
    lqc2    $vf30, 16($at)
    lqc2    $vf26, 32($at)
    lqc2    $vf27, 48($at)
    lqc2    $vf28, 64($at)
    qmtc2   $t1, $vf05
    .word   0x4A004938
    ldl     $t1,   7($a3)
    ldr     $t1,   0($a3)
    ldl     $t2,  15($a3)
    ldr     $t2,   8($a3)
    ldl     $t3,  23($a3)
    ldr     $t3,  16($a3)
    ldl     $t4,  31($a3)
    ldr     $t4,  24($a3)
    pcpyld  $t1, $t2, $t1
    pcpyld  $t3, $t4, $t3
    ldl     $t2,  39($a3)
    ldr     $t2,  32($a3)
    ldl     $t4,  47($a3)
    ldr     $t4,  40($a3)
    ldl     $t5,  55($a3)
    ldr     $t5,  48($a3)
    ldl     $t6,  63($a3)
    ldr     $t6,  56($a3)
    pcpyld  $t2, $t4, $t2
    pcpyld  $t5, $t6, $t5
    ldl     $t4,  71($a3)
    ldr     $t4,  64($a3)
    ldl     $t6,  79($a3)
    ldr     $t6,  72($a3)
    ldl     $t7,  87($a3)
    ldr     $t7,  80($a3)
    ldl     $t8,  95($a3)
    ldr     $t8,  88($a3)
    ldl     $t9, 103($a3)
    ldr     $t9,  96($a3)
    pcpyld  $t6, $t7, $t6
    pcpyld  $t8, $t9, $t8
    ldl     $t7, 111($a3)
    ldr     $t7, 104($a3)
    ldl     $t9, 119($a3)
    ldr     $t9, 112($a3)
    ldl     $v0, 127($a3)
    ldr     $v0, 120($a3)
    ldl     $v1, 135($a3)
    ldr     $v1, 128($a3)
    pcpyld  $t7, $t9, $t7
    pcpyld  $v0, $v1, $v0
    ldl     $t9, 143($a3)
    ldr     $t9, 136($a3)
    ldl     $v1,  7($a1)
    ldr     $v1,  0($a1)
    ldl     $at, 15($a1)
    ldr     $at,  8($a1)
    ldl     $a3, 23($a1)
    ldr     $a3, 16($a1)
    ldl     $a0, 31($a1)
    ldr     $a0, 24($a1)
    pcpyld  $v1, $at, $v1
    pcpyld  $a3, $a0, $a3
    ldl     $at, 39($a1)
    ldr     $at, 32($a1)
    ldl     $a0, 47($a1)
    ldr     $a0, 40($a1)
    ldl     $s0, 55($a1)
    ldr     $s0, 48($a1)
    ldl     $s1, 63($a1)
    ldr     $s1, 56($a1)
    pcpyld  $at, $a0, $at
    pcpyld  $s0, $s1, $s0
    ldl     $a0, 71($a1)
    ldr     $a0, 64($a1)
    qmtc2.i $t6, $vf15
    qmtc2   $t8, $vf16
    qmtc2   $t7, $vf17
    qmtc2   $v0, $vf18
    qmtc2   $t1, $vf01
    qmtc2   $t3, $vf02
    qmtc2   $t2, $vf03
    qmtc2   $t5, $vf04
    qmtc2   $t4, $vf05
    qmtc2   $t9, $vf19
    qmtc2   $v1, $vf26
    qmtc2   $a3, $vf27
    qmtc2   $at, $vf28
    qmtc2   $s0, $vf29
    qmtc2   $a0, $vf30
    vmul.xyzw   $vf10, $vf10, $vf15
    vmul.xyzw   $vf11, $vf11, $vf16
    vmul.xyzw   $vf12, $vf12, $vf17
    vmul.xyzw   $vf13, $vf13, $vf18
    vmul.xyzw   $vf14, $vf14, $vf19
    vmul.xyzw   $vf21, $vf21, $vf01
    vmul.xyzw   $vf22, $vf22, $vf02
    vmul.xyzw   $vf23, $vf23, $vf03
    vmul.xyzw   $vf24, $vf24, $vf04
    vmul.xyzw   $vf25, $vf25, $vf05
    vadd.xyzw   $vf21, $vf21, $vf26
    vadd.xyzw   $vf22, $vf22, $vf27
    vadd.xyzw   $vf23, $vf23, $vf28
    vadd.xyzw   $vf24, $vf24, $vf29
    vadd.xyzw   $vf25, $vf25, $vf30
    qmfc2   $t1, $vf10
    qmfc2   $t3, $vf11
    pcpyud  $t2, $t1, $zero
    pcpyud  $t4, $t3, $zero
    qmfc2   $t5, $vf12
    qmfc2   $t7, $vf13
    pcpyud  $t6, $t5, $zero
    pcpyud  $t8, $t7, $zero
    sdl     $t1,  7($a2)
    sdr     $t1,  0($a2)
    sdl     $t2, 15($a2)
    sdr     $t2,  8($a2)
    qmfc2   $t1, $vf14
    sdl     $t3, 23($a2)
    sdr     $t3, 16($a2)
    sdl     $t4, 31($a2)
    sdr     $t4, 24($a2)
    sdl     $t5, 39($a2)
    sdr     $t5, 32($a2)
    sdl     $t6, 47($a2)
    sdr     $t6, 40($a2)
    sdl     $t7, 55($a2)
    sdr     $t7, 48($a2)
    sdl     $t8, 63($a2)
    sdr     $t8, 56($a2)
    sdl     $t1, 71($a2)
    sdr     $t1, 64($a2)
    qmfc2   $t1, $vf21
    qmfc2   $t3, $vf22
    pcpyud  $t2, $t1, $zero
    pcpyud  $t4, $t3, $zero
    qmfc2   $t5, $vf23
    qmfc2   $t7, $vf24
    pcpyud  $t6, $t5, $zero
    pcpyud  $t8, $t7, $zero
    sw      $t1,    0($t0)
    dsrl32  $t1, $t1, 0
    sw      $t2,  256($t0)
    dsrl32  $t2, $t2, 0
    sw      $t1,  128($t0)
    sw      $t2,  384($t0)
    qmfc2   $t1, $vf25
    sw      $t3,  512($t0)
    dsrl32  $t3, $t3, 0
    dsrl32  $t2, $t1, 0
    sw      $t4,  768($t0)
    dsrl32  $t4, $t4, 0
    sw      $t3,  640($t0)
    sw      $t4,  896($t0)
    sw      $t5, 1024($t0)
    dsrl32  $t5, $t5, 0
    sw      $t6, 1280($t0)
    dsrl32  $t6, $t6, 0
    sw      $t5, 1152($t0)
    sw      $t6, 1408($t0)
    sw      $t7, 1536($t0)
    dsrl32  $t7, $t7, 0
    sw      $t8, 1792($t0)
    dsrl32  $t8, $t8, 0
    sw      $t7, 1664($t0)
    sw      $t8, 1920($t0)
    sw      $t1, 2048($t0)
    sw      $t2, 2176($t0)
    ld      $s0, 0($sp)
    ld      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 16

MP123_IMDCT12:
    la      $at, s_Cos12
    pref    0, 0($at)
    ldl     $t1,  7($a0)
    ldr     $t1,  0($a0)
    lw      $t2,  8($a0)
    ldl     $t3, 19($a0)
    ldr     $t3, 12($a0)
    lw      $t4, 20($a0)
    ldl     $t5, 31($a0)
    ldr     $t5, 24($a0)
    lw      $t6, 32($a0)
    pcpyld  $t1, $t2, $t1
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    qmtc2   $t1, $vf06
    qmtc2   $t3, $vf07
    qmtc2   $t5, $vf08
    ldl     $t1, 43($a0)
    ldr     $t1, 36($a0)
    lw      $t2, 44($a0)
    ldl     $t3, 55($a0)
    ldr     $t3, 48($a0)
    lw      $t4, 56($a0)
    ldl     $t5, 67($a0)
    ldr     $t5, 60($a0)
    lw      $t6, 68($a0)
    pcpyld  $t1, $t2, $t1
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    qmtc2   $t1, $vf09
    qmtc2   $t3, $vf10
    qmtc2   $t5, $vf11
    lqc2    $vf01,  0($at)
    lqc2    $vf02, 16($at)
    lqc2    $vf03, 32($at)
    lqc2    $vf04, 48($at)
    lqc2    $vf05, 64($at)
    ldl     $t1,  7($a3)
    ldr     $t1,  0($a3)
    lw      $t2,  8($a3)
    ldl     $t3, 19($a3)
    ldr     $t3, 12($a3)
    lw      $t4, 20($a3)
    ldl     $t5, 31($a3)
    ldr     $t5, 24($a3)
    lw      $t6, 32($a3)
    ldl     $t7, 43($a3)
    ldr     $t7, 36($a3)
    lw      $t8, 44($a3)
    pcpyld  $t1, $t2, $t1
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    pcpyld  $t7, $t8, $t7
    qmtc2   $t1, $vf12
    qmtc2   $t3, $vf13
    qmtc2   $t5, $vf14
    qmtc2   $t7, $vf15
    ldl     $t1, 31($a1)
    ldr     $t1, 24($a1)
    lw      $t2, 32($a1)
    ldl     $t3, 43($a1)
    ldr     $t3, 36($a1)
    lw      $t4, 44($a1)
    ldl     $t5, 55($a1)
    ldr     $t5, 48($a1)
    lw      $t6, 56($a1)
    ldl     $t7, 67($a1)
    ldr     $t7, 60($a1)
    lw      $t8, 68($a1)
    pcpyld  $t1, $t2, $t1
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    pcpyld  $t7, $t8, $t7
    qmtc2   $t1, $vf16
    qmtc2   $t3, $vf17
    qmtc2   $t5, $vf18
    qmtc2   $t7, $vf19
    .word   0x4A003438
    lw      $t1,  0($a1)
    lw      $t2,  4($a1)
    lw      $t3,  8($a1)
    lw      $t4, 12($a1)
    lw      $t5, 16($a1)
    lw      $t6, 20($a1)
    sw      $t1,   0($t0)
    sw      $t2, 128($t0)
    sw      $t3, 256($t0)
    sw      $t4, 384($t0)
    sw      $t5, 512($t0)
    sw      $t6, 640($t0)
    sw      $zero, 48($a2)
    sw      $zero, 52($a2)
    sw      $zero, 56($a2)
    sw      $zero, 60($a2)
    sw      $zero, 64($a2)
    sw      $zero, 68($a2)
    qmfc2.i $t1, $vf16
    qmfc2   $t3, $vf17
    qmfc2   $t5, $vf18
    qmfc2   $t7, $vf19
    pcpyud  $t2, $t1, $zero
    pcpyud  $t4, $t3, $zero
    pcpyud  $t6, $t5, $zero
    pcpyud  $t8, $t7, $zero
    sw      $t1,  768($t0)
    dsrl32  $t1, $t1, 0
    sw      $t2, 1024($t0)
    sw      $t1,  896($t0)
    sw      $t3, 1152($t0)
    dsrl32  $t3, $t3, 0
    sw      $t4, 1408($t0)
    sw      $t3, 1280($t0)
    sw      $t5, 1536($t0)
    dsrl32  $t5, $t5, 0
    sw      $t6, 1792($t0)
    sw      $t5, 1664($t0)
    sw      $t7, 1920($t0)
    dsrl32  $t7, $t7, 0
    sw      $t8, 2176($t0)
    sw      $t7, 2048($t0)
    qmfc2   $t1, $vf23
    qmfc2   $t3, $vf24
    qmfc2   $t5, $vf25
    qmfc2   $t7, $vf26
    pcpyud  $t2, $t1, $zero
    pcpyud  $t4, $t3, $zero
    pcpyud  $t6, $t5, $zero
    pcpyud  $t8, $t7, $zero
    sdl     $t1,  7($a2)
    sdr     $t1,  0($a2)
    sw      $t2,  8($a2)
    sdl     $t3, 19($a2)
    sdr     $t3, 12($a2)
    sw      $t4, 20($a2)
    sdl     $t5, 31($a2)
    sdr     $t5, 24($a2)
    sw      $t6, 32($a2)
    sdl     $t7, 43($a2)
    sdr     $t7, 36($a2)
    jr      $ra
    sw      $t8, 44($a2)

MP123_Synth:
    addu    $t8, $zero, $a2
    addu    $a2, $zero, $a0
    lw      $v1, s_Offset
    bnel    $a1, $zero, 1f
    addu    $t8, $t8, 2
    addu    $v1, $v1, -1
    la      $t4, s_SynthBuf
    andi    $v1, $v1, 0x000F
    beq     $zero, $zero, 2f
    sw      $v1, s_Offset
1:
    la      $t4, s_SynthBuf + 2176
2:
    andi    $t5, $v1, 0x0001
    beql    $t5, $zero, 1f
    addiu   $t9, $t4, 1088
    addu    $t9, $zero, $t4
    addu    $t6, $zero, $v1
    addiu   $t5, $v1, 1
    addiu   $a0, $t4, 1088
    andi    $t5, $t5, 0x000F
    addu    $a1, $zero, $t4
    sll     $v1, $v1, 2
    sll     $t5, $t5, 2
    addu    $a1, $a1, $v1
    addu    $a0, $a0, $t5
    beq     $zero, $zero, 2f
    sll     $t6, $t6, 2
1:
    addiu   $t6, $v1, 1
    sll     $v1, $v1, 2
    sll     $t6, $t6, 2
    addu    $a0, $t4, $v1
    addu    $a1, $t9, $t6
2:
    mtsah   $zero, 6
    lqc2    $vf01,  0($a2)
    lqc2    $vf02, 16($a2)
    lqc2    $vf03, 32($a2)
    lqc2    $vf04, 48($a2)
    lq      $t3,   64($a2)
    lq      $t2,   80($a2)
    lq      $t1,   96($a2)
    lq      $t0,  112($a2)
    pexew   $t3, $t3
    pexew   $t2, $t2
    pexew   $t1, $t1
    pexew   $t0, $t0
    qfsrv   $t3, $t3
    qfsrv   $t2, $t2
    qfsrv   $t1, $t1
    qfsrv   $t0, $t0
    qmtc2   $t3, $vf08
    qmtc2   $t2, $vf07
    qmtc2   $t1, $vf06
    qmtc2   $t0, $vf05
    la      $at, s_Cos64
    vadd.xyzw   $vf12, $vf04, $vf08
    vadd.xyzw   $vf11, $vf03, $vf07
    vadd.xyzw   $vf10, $vf02, $vf06
    vadd.xyzw   $vf09, $vf01, $vf05
    pref    0, 64($at)
    vsub.xyzw   $vf01, $vf01, $vf05
    vsub.xyzw   $vf02, $vf02, $vf06
    vsub.xyzw   $vf03, $vf03, $vf07
    vsub.xyzw   $vf04, $vf04, $vf08
    lqc2    $vf13,  0($at)
    lqc2    $vf14, 16($at)
    lqc2    $vf15, 32($at)
    lqc2    $vf16, 48($at)
    vmul.xyzw   $vf01, $vf01, $vf13
    vmul.xyzw   $vf02, $vf02, $vf14
    vmul.xyzw   $vf03, $vf03, $vf15
    vmul.xyzw   $vf04, $vf04, $vf16
    qmfc2   $t0, $vf12
    qmfc2   $t1, $vf11
    pexew   $t0, $t0
    pexew   $t1, $t1
    qfsrv   $t0, $t0
    qfsrv   $t1, $t1
    qmtc2   $t0, $vf05
    qmtc2   $t1, $vf06
    vadd.xyzw   $vf13, $vf09, $vf05
    vadd.xyzw   $vf14, $vf10, $vf06
    vsub.xyzw   $vf15, $vf09, $vf05
    vsub.xyzw   $vf16, $vf10, $vf06
    lqc2    $vf05, 64($at)
    lqc2    $vf06, 80($at)
    qmfc2   $t0, $vf04
    qmfc2   $t1, $vf03
    pexew   $t0, $t0
    pexew   $t1, $t1
    vmul.xyzw   $vf15, $vf15, $vf05
    vmul.xyzw   $vf16, $vf16, $vf06
    qfsrv   $t0, $t0
    qfsrv   $t1, $t1
    qmtc2   $t0, $vf04
    qmtc2   $t1, $vf03
    vsub.xyzw   $vf19, $vf01, $vf04
    vsub.xyzw   $vf20, $vf02, $vf03
    vadd.xyzw   $vf17, $vf01, $vf04
    vadd.xyzw   $vf18, $vf02, $vf03
    vmul.xyzw   $vf19, $vf19, $vf05
    vmul.xyzw   $vf20, $vf20, $vf06
    qmfc2   $t0, $vf14
    qmfc2   $t1, $vf16
    qmfc2   $t2, $vf18
    qmfc2   $t3, $vf20
    pexew   $t0, $t0
    pexew   $t1, $t1
    pexew   $t2, $t2
    pexew   $t3, $t3
    qfsrv   $t0, $t0
    qfsrv   $t1, $t1
    qfsrv   $t2, $t2
    qfsrv   $t3, $t3
    qmtc2   $t0, $vf14
    qmtc2   $t1, $vf16
    qmtc2   $t2, $vf18
    qmtc2   $t3, $vf20
    vsub.xyzw   $vf10, $vf13, $vf14
    vsub.xyzw   $vf12, $vf15, $vf16
    vsub.xyzw   $vf03, $vf17, $vf18
    vsub.xyzw   $vf01, $vf19, $vf20
    lqc2    $vf05, 96($at)
    vadd.xyzw   $vf09, $vf13, $vf14
    vadd.xyzw   $vf11, $vf15, $vf16
    vadd.xyzw   $vf04, $vf17, $vf18
    vadd.xyzw   $vf02, $vf19, $vf20
    vmul.xyzw   $vf10, $vf10, $vf05
    vmul.xyzw   $vf12, $vf12, $vf05
    vmul.xyzw   $vf03, $vf03, $vf05
    vmul.xyzw   $vf01, $vf01, $vf05
    mtsah   $zero, 2
    qmfc2   $t0, $vf09
    qmfc2   $t1, $vf10
    ld      $v0, 112($at)
    pcpyld  $t2, $t0, $t1
    pcpyud  $t0, $t1, $t0
    pexew   $t0, $t0
    pcpyld  $v0, $v0, $v0
    qfsrv   $t0, $t0
    qmtc2   $t2, $vf09
    qmtc2   $t0, $vf10
    qmtc2   $v0, $vf05
    vsub.xyzw   $vf14, $vf09, $vf10
    vadd.xyzw   $vf13, $vf09, $vf10
    vmul.xyzw   $vf14, $vf14, $vf05
    qmfc2   $t0, $vf11
    qmfc2   $t1, $vf12
    pcpyld  $t2, $t0, $t1
    pcpyud  $t0, $t1, $t0
    pexew   $t0, $t0
    qfsrv   $t0, $t0
    qmtc2   $t2, $vf11
    qmtc2   $t0, $vf12
    vsub.xyzw   $vf15, $vf11, $vf12
    vadd.xyzw   $vf16, $vf11, $vf12
    vmul.xyzw   $vf15, $vf15, $vf05
    qmfc2   $t0, $vf04
    qmfc2   $t1, $vf03
    pcpyld  $t2, $t0, $t1
    pcpyud  $t0, $t1, $t0
    pexew   $t0, $t0
    qfsrv   $t0, $t0
    qmtc2   $t2, $vf04
    qmtc2   $t0, $vf03
    vsub.xyzw   $vf18, $vf04, $vf03
    vadd.xyzw   $vf17, $vf04, $vf03
    vmul.xyzw   $vf18, $vf18, $vf05
    qmfc2   $t0, $vf02
    qmfc2   $t1, $vf01
    pcpyld  $t2, $t0, $t1
    pcpyud  $t0, $t1, $t0
    pexew   $t0, $t0
    qfsrv   $t0, $t0
    qmtc2   $t2, $vf02
    qmtc2   $t0, $vf01
    vsub.xyzw   $vf19, $vf02, $vf01
    vadd.xyzw   $vf20, $vf02, $vf01
    vmul.xyzw   $vf19, $vf19, $vf05
    qmfc2   $t0, $vf13
    qmfc2   $t1, $vf14
    lw      $at, 120($at)
    pextlw  $t2, $t0, $t1
    pextuw  $t0, $t0, $t1
    pextlw  $at, $at, $at
    pcpyld  $t1, $t0, $t2
    pcpyud  $t0, $t2, $t0
    pcpyld  $at, $at, $at
    qmtc2   $t0, $vf13
    qmtc2   $t1, $vf14
    qmtc2   $at, $vf05
    vsub.xyzw   $vf10, $vf14, $vf13
    vadd.xyzw   $vf09, $vf14, $vf13
    vmul.xyzw   $vf10, $vf10, $vf05
    qmfc2   $t0, $vf16
    qmfc2   $t1, $vf15
    pextlw  $t2, $t0, $t1
    pextuw  $t0, $t0, $t1
    pcpyld  $t1, $t0, $t2
    pcpyud  $t0, $t2, $t0
    qmtc2   $t0, $vf16
    qmtc2   $t1, $vf15
    vsub.xyzw   $vf12, $vf15, $vf16
    vadd.xyzw   $vf11, $vf15, $vf16
    vmul.xyzw   $vf12, $vf12, $vf05
    qmfc2   $t0, $vf17
    qmfc2   $t1, $vf18
    pextlw  $t2, $t0, $t1
    pextuw  $t0, $t0, $t1
    pcpyld  $t1, $t0, $t2
    pcpyud  $t0, $t2, $t0
    qmtc2   $t0, $vf17
    qmtc2   $t1, $vf18
    vsub.xyzw   $vf03, $vf18, $vf17
    vadd.xyzw   $vf04, $vf18, $vf17
    vmul.xyzw   $vf03, $vf03, $vf05
    qmfc2   $t0, $vf20
    qmfc2   $t1, $vf19
    pextlw  $t2, $t0, $t1
    pextuw  $t0, $t0, $t1
    pcpyld  $t1, $t0, $t2
    pcpyud  $t0, $t2, $t0
    qmtc2   $t0, $vf20
    qmtc2   $t1, $vf19
    .word   0x4A002938
    la      $v1, s_DecWin
    subu    $v1, $v1, $t6
    lui     $t7, 0x46FF
    lui     $at, 0xC700
    addiu   $v1, $v1, 64
    ori     $t7, $t7, 0xFE00
    mtc1    $at, $f19
    mtc1    $t7, $f18
    mtc1    $zero, $f00
    addiu   $t7, $zero, 16
    sll     $a3, $a3, 1
    qmfc2.i $t0, $vf05
    qmfc2   $t1, $vf09
    qmfc2   $t2, $vf07
    qmfc2   $t3, $vf08
    qmfc2   $at, $vf13
    sw      $t0,    0($a0)
    sw      $t1,  256($a0)
    sw      $t2,  512($a0)
    sw      $t3,  768($a0)
    sw      $at, 1024($a0)
    qmfc2   $t0, $vf14
    qmfc2   $t1, $vf15
    qmfc2   $t2, $vf16
    qmfc2   $t3, $vf10
    sw      $t0,   0($a1)
    sw      $t1, 256($a1)
    sw      $t2, 512($a1)
    sw      $t3, 768($a1)
    vaddw.x     $vf05, $vf02, $vf03
    vaddz.x     $vf06, $vf04, $vf02
    vaddz.y     $vf07, $vf02, $vf04
    vaddw.y     $vf08, $vf04, $vf02
    vaddw.w     $vf13, $vf03, $vf01
    vaddy.y     $vf14, $vf01, $vf03
    vaddz.z     $vf15, $vf01, $vf03
    vaddx.x     $vf16, $vf03, $vf01
    qmfc2   $t0, $vf05
    qmfc2   $t1, $vf06
    qmfc2   $t2, $vf07
    qmfc2   $t3, $vf08
    dsrl32  $t2, $t2, 0
    dsrl32  $t3, $t3, 0
    sw      $t0,  64($a0)
    sw      $t1, 320($a0)
    sw      $t2, 576($a0)
    sw      $t3, 832($a0)
    qmfc2   $t0, $vf13
    qmfc2   $t1, $vf14
    qmfc2   $t2, $vf15
    qmfc2   $t3, $vf16
    pcpyud  $t0, $t0, $t0
    dsrl32  $t1, $t1, 0
    dsrl32  $t0, $t0, 0
    pcpyud  $t2, $t2, $t2
    sw      $t0,  64($a1)
    sw      $t1, 320($a1)
    sw      $t2, 576($a1)
    sw      $t3, 832($a1)
    vaddz.x     $vf06, $vf00, $vf11
    vaddy.x     $vf07, $vf00, $vf11
    vaddw.x     $vf08, $vf00, $vf11
    vaddw.x     $vf13, $vf00, $vf12
    vaddy.x     $vf14, $vf00, $vf12
    vaddz.x     $vf15, $vf00, $vf12
    qmfc2   $t0, $vf11
    qmfc2   $t1, $vf06
    qmfc2   $t2, $vf07
    qmfc2   $t3, $vf08
    sw      $t0, 128($a0)
    sw      $t1, 384($a0)
    sw      $t2, 640($a0)
    sw      $t3, 896($a0)
    qmfc2   $t0, $vf13
    qmfc2   $t1, $vf14
    qmfc2   $t2, $vf15
    qmfc2   $t3, $vf12
    sw      $t0, 128($a1)
    sw      $t1, 384($a1)
    sw      $t2, 640($a1)
    sw      $t3, 896($a1)
    vaddx.x     $vf05, $vf04, $vf02
    vaddz.z     $vf06, $vf04, $vf02
    vaddy.y     $vf07, $vf04, $vf02
    vaddw.w     $vf08, $vf04, $vf02
    vaddw.y     $vf13, $vf03, $vf01
    vaddz.y     $vf14, $vf01, $vf03
    vaddz.x     $vf15, $vf03, $vf01
    qmfc2   $t0, $vf05
    qmfc2   $t1, $vf06
    qmfc2   $t2, $vf07
    qmfc2   $t3, $vf08
    pcpyud  $t1, $t1, $t1
    dsrl32  $t2, $t2, 0
    pcpyud  $t3, $t3, $t3
    sw      $t0, 192($a0)
    dsrl32  $t3, $t3, 0
    sw      $t1, 448($a0)
    sw      $t2, 704($a0)
    sw      $t3, 960($a0)
    qmfc2   $t0, $vf13
    qmfc2   $t1, $vf14
    qmfc2   $t2, $vf15
    qmfc2   $t3, $vf01
    dsrl32  $t0, $t0, 0
    dsrl32  $t1, $t1, 0
    sw      $t0, 192($a1)
    sw      $t1, 448($a1)
    sw      $t2, 704($a1)
    sw      $t3, 960($a1)
1:
    lwc1    $f01,  0($v1)
    lwc1    $f02,  0($t9)
    lwc1    $f03,  4($v1)
    lwc1    $f04,  4($t9)
    mul.s   $f01, $f01, $f02
    lwc1    $f05,  8($v1)
    lwc1    $f06,  8($t9)
    lwc1    $f07, 12($v1)
    adda.s  $f01, $f00
    lwc1    $f08, 12($t9)
    msuba.s $f03, $f04
    lwc1    $f01, 16($v1)
    madda.s $f05, $f06
    lwc1    $f02, 16($t9)
    msuba.s $f07, $f08
    lwc1    $f03, 20($v1)
    lwc1    $f04, 20($t9)
    lwc1    $f05, 24($v1)
    lwc1    $f06, 24($t9)
    madda.s $f01, $f02
    lwc1    $f07, 28($v1)
    msuba.s $f03, $f04
    lwc1    $f08, 28($t9)
    madda.s $f05, $f06
    lwc1    $f01, 32($v1)
    lwc1    $f02, 32($t9)
    msuba.s $f07, $f08
    lwc1    $f03, 36($v1)
    lwc1    $f04, 36($t9)
    madda.s $f01, $f02
    lwc1    $f05, 40($v1)
    lwc1    $f06, 40($t9)
    msuba.s $f03, $f04
    lwc1    $f07, 44($v1)
    lwc1    $f08, 44($t9)
    madda.s $f05, $f06
    lwc1    $f01, 48($v1)
    lwc1    $f02, 48($t9)
    msuba.s $f07, $f08
    lwc1    $f03, 52($v1)
    lwc1    $f04, 52($t9)
    madda.s $f01, $f02
    lwc1    $f05, 56($v1)
    lwc1    $f06, 56($t9)
    msuba.s $f03, $f04
    lwc1    $f07, 60($v1)
    lwc1    $f08, 60($t9)
    madda.s $f05, $f06
    msuba.s $f07, $f08
    madd.s  $f01, $f00, $f00
    addiu   $v1, $v1, 128
    min.s   $f01, $f01, $f18
    addiu   $t9, $t9, 64
    max.s   $f01, $f01, $f19
    pref    0, 0($v1)
    pref    0, 0($t9)
    addiu   $t7, $t7, -1
    cvt.w.s $f01, $f01
    mfc1    $t4, $f01
    sh      $t4, 0($t8)
    bgtz    $t7, 1b
    addu    $t8, $t8, $a3
    lwc1    $f01,  0($v1)
    lwc1    $f02,  0($t9)
    lwc1    $f03,  8($v1)
    lwc1    $f04,  8($t9)
    mul.s   $f01, $f01, $f02
    lwc1    $f05, 16($v1)
    lwc1    $f06, 16($t9)
    lwc1    $f07, 24($v1)
    adda.s  $f01, $f00
    lwc1    $f08, 24($t9)
    madda.s $f03, $f04
    lwc1    $f01, 32($v1)
    madda.s $f05, $f06
    lwc1    $f02, 32($t9)
    madda.s $f07, $f08
    lwc1    $f03, 40($v1)
    lwc1    $f04, 40($t9)
    lwc1    $f05, 48($v1)
    lwc1    $f06, 48($t9)
    madda.s $f01, $f02
    lwc1    $f07, 56($v1)
    madda.s $f03, $f04
    lwc1    $f08, 56($t9)
    madda.s $f05, $f06
    madda.s $f07, $f08
    sll     $t6, $t6, 1
    madd.s  $f01, $f00, $f00
    min.s   $f01, $f01, $f18
    addiu   $t9, $t9, -64
    addiu   $t7, $zero, 15
    max.s   $f01, $f01, $f19
    addiu   $v1, $v1, -128
    cvt.w.s $f01, $f01
    addu    $v1, $v1, $t6
    mfc1    $t4, $f01
    sh      $t4, 0($t8)
    addu    $t8, $t8, $a3
1:
    lwc1    $f01,  -4($v1)
    lwc1    $f02,   0($t9)
    lwc1    $f03,  -8($v1)
    lwc1    $f04,   4($t9)
    mul.s   $f01, $f01, $f02
    lwc1    $f05, -12($v1)
    lwc1    $f06,   8($t9)
    lwc1    $f07, -16($v1)
    suba.s  $f00, $f01
    lwc1    $f08,  12($t9)
    msuba.s $f03, $f04
    lwc1    $f01, -20($v1)
    msuba.s $f05, $f06
    lwc1    $f02,  16($t9)
    msuba.s $f07, $f08
    lwc1    $f03, -24($v1)
    lwc1    $f04,  20($t9)
    lwc1    $f05, -28($v1)
    lwc1    $f06,  24($t9)
    msuba.s $f01, $f02
    lwc1    $f07, -32($v1)
    msuba.s $f03, $f04
    lwc1    $f08,  28($t9)
    msuba.s $f05, $f06
    lwc1    $f01, -36($v1)
    lwc1    $f02,  32($t9)
    msuba.s $f07, $f08
    lwc1    $f03, -40($v1)
    lwc1    $f04,  36($t9)
    msuba.s $f01, $f02
    lwc1    $f05, -44($v1)
    lwc1    $f06,  40($t9)
    msuba.s $f03, $f04
    lwc1    $f07, -48($v1)
    lwc1    $f08,  44($t9)
    msuba.s $f05, $f06
    lwc1    $f01, -52($v1)
    lwc1    $f02,  48($t9)
    msuba.s $f07, $f08
    lwc1    $f03, -56($v1)
    lwc1    $f04,  52($t9)
    msuba.s $f01, $f02
    lwc1    $f05, -60($v1)
    lwc1    $f06,  56($t9)
    msuba.s $f03, $f04
    lwc1    $f07, -64($v1)
    lwc1    $f08,  60($t9)
    msuba.s $f05, $f06
    msuba.s $f07, $f08
    madd.s  $f01, $f00, $f00
    addiu   $v1, $v1, -128
    min.s   $f01, $f01, $f18
    addiu   $t9, $t9, -64
    max.s   $f01, $f01, $f19
    addiu   $t7, $t7, -1
    cvt.w.s $f01, $f01
    mfc1    $t4, $f01
    sh      $t4, 0($t8)
    bgtz    $t7, 1b
    addu    $t8, $t8, $a3
    addiu   $v0, $zero, 16
    srl     $a3, $a3, 1
    jr      $ra
    sllv    $v0, $v0, $a3
