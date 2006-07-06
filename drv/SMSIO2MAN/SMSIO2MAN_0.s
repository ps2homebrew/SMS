#
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
#
.set noreorder
.set noat
.set nomacro
.set volatile

.globl SIO2_GetCtrl
.globl SIO2_SetCtrl
.globl SIO2_GetStat6C
.globl SIO2_SetPortNCtrl1
.globl SIO2_GetPortNCtrl1
.globl SIO2_SetPortNCtrl2
.globl SIO2_GetPortNCtrl2
.globl SIO2_GetStat70
.globl SIO2_SetReg
.globl SIO2_GetReg
.globl SIO2_GetStat74
.globl SIO2_SetUnk78
.globl SIO2_GetUnk78
.globl SIO2_SetUnk7C
.globl SIO2_GetUnk7C
.globl SIO2_DataIn
.globl SIO2_DataOut
.globl SIO2_GetStat
.globl SIO2_SetStat
.globl SIO2_SetCtrl3BC
.globl SIO2_SwitchCtrlC
.globl SIO2_SwitchCtrl1

.text

SIO2_GetCtrl:
    lui     $v0, 0xBF81
    lw      $v0, -32152($v0)
    jr      $ra

SIO2_SetCtrl:
    lui     $v1, 0xBF81
    jr      $ra
    sw      $a0, -32152($v1)

SIO2_GetStat6C:
    lui     $v0, 0xBF81
    lw      $v0, -32148($v0)
    jr      $ra

SIO2_SetPortNCtrl1:
    sll     $a0, $a0, 3
    lui     $at, 0xBF81
    addu    $at, $at, $a0
    jr      $ra
    sw      $a1, -32192($at)

SIO2_GetPortNCtrl1:
    sll     $a0, $a0, 3
    lui     $v0, 0xBF81
    addu    $v0, $v0, $a0
    lw      $v0, -32192($v0)
    jr      $ra

SIO2_GetPortNCtrl2:
    sll     $a0, $a0, 3
    lui     $v0, 0xBF81
    addu    $v0, $v0, $a0
    lw      $v0, -32188($v0)
    jr      $ra

SIO2_SetPortNCtrl2:
    sll     $a0, $a0, 3
    lui     $v0, 0xBF81
    addu    $v0, $v0, $a0
    jr      $ra
    sw      $a1, -32188($v0)

SIO2_GetStat70:
    lui     $v0, 0xBF81
    lw      $v0, -32144($v0)
    jr      $ra

SIO2_GetReg:
    sll     $a0, $a0, 2
    lui     $v0, 0xBF81
    addu    $v0, $v0, $a0
    lw      $v0, -32256($v0)
    jr      $ra

SIO2_SetReg:
    sll     $a0, $a0, 2
    lui     $v0, 0xBF81
    addu    $v0, $v0, $a0
    jr      $ra
    sw      $a1, -32256($v0)

SIO2_GetStat74:
    lui     $v0, 0xBF81
    lw      $v0, -32140($v0)
    jr      $ra

SIO2_GetUnk78:
    lui     $v1, 0xBF81
    lw      $v0, -32136($v1)
    jr      $ra

SIO2_SetUnk78:
    lui     $v1, 0xBF81
    jr      $ra
    sw      $a0, -32136($v1)

SIO2_GetUnk7C:
    lui     $v0, 0xBF81
    lw      $v0, -32132($v0)
    jr      $ra

SIO2_SetUnk7C:
    lui     $v1, 0xBF80
    jr      $ra
    sw      $a0, -32132($v1)

SIO2_DataIn:
    lui     $v0, 0xBF81
    lbu     $v0, -32156($v0)
    jr      $ra

SIO2_DataOut:
    lui     $v1, 0xBF81
    jr      $ra
    sb      $a0, -32160($v1)

SIO2_GetStat:
    lui     $v0, 0xBF81
    lw      $v0, -32128($v0)
    jr      $ra

SIO2_SetStat:
    lui     $v1, 0xBF81
    jr      $ra
    sw      $a0, -32128($v1)

SIO2_SetCtrl3BC:
    j       SIO2_SetCtrl
    addiu   $a0, $zero, 0x03BC

SIO2_SwitchCtrlC:
    addiu   $sp, $sp, -4
    sw      $ra, 0($sp)
    jal     SIO2_GetCtrl
    nop
    lw      $ra, 0($sp)
    ori     $a0, $v0, 0x000C
    j       SIO2_SetCtrl
    addiu   $sp, $sp, 4

SIO2_SwitchCtrl1:
    addiu   $sp, $sp, -4
    sw      $ra, 0($sp)
    jal     SIO2_GetCtrl
    nop
    lw      $ra, 0($sp)
    ori     $a0, $v0, 1
    j       SIO2_SetCtrl
    addiu   $sp, $sp, 4
