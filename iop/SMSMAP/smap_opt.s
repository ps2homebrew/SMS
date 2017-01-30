.set noreorder
.set nomacro
.set noat

.globl SMAP_CopyFromFIFO
.globl SMAP_CopyToFIFO

.text
SMAP_CopyFromFIFO:
    lhu     $v1,  8($a0)
    li      $v0, -4
    lui     $a3, 0xB000
    addiu   $v1, $v1, 3
    and     $v1, $v1, $v0
    srl     $at, $v1, 5
    lw      $a0, 4($a0)
    sh      $a1, 0x1034($a3)
    beqz    $at, 3f
    andi    $v1, $v1, 0x1F
4:
    lw      $t0, 4608($a3)
    lw      $t1, 4608($a3)
    lw      $t2, 4608($a3)
    lw      $t3, 4608($a3)
    lw      $t4, 4608($a3)
    lw      $t5, 4608($a3)
    lw      $t6, 4608($a3)
    lw      $t7, 4608($a3)
    addiu   $at, $at, -1
    sw      $t0,  0($a0)
    sw      $t1,  4($a0)
    sw      $t2,  8($a0)
    sw      $t3, 12($a0)
    sw      $t4, 16($a0)
    sw      $t5, 20($a0)
    sw      $t6, 24($a0)
    sw      $t7, 28($a0)
    bgtz    $at, 4b
    addiu   $a0, $a0, 32
3:
    beqz    $v1, 1f
    nop
2:
    lw      $v0, 4608($a3)
    addiu   $v1, $v1, -4
    sw      $v0, 0($a0)
    bnez    $v1, 2b
    addiu   $a0, $a0, 4
1:
    jr      $ra
    nop

SMAP_CopyToFIFO:
    srl     $at, $a1, 4
    lui     $v1, 0xB000
    lhu     $v0, 0x1004($v1)
    beqz    $at, 3f
    andi    $a1, $a1, 0xF
4:
    lwr     $t0,  0($a0)
    lwl     $t0,  3($a0)
    lwr     $t1,  4($a0)
    lwl     $t1,  7($a0)
    lwr     $t2,  8($a0)
    lwl     $t2, 11($a0)
    lwr     $t3, 12($a0)
    lwl     $t3, 15($a0)
    addiu   $at, $at, -1
    sw      $t0, 4352($v1)
    sw      $t1, 4352($v1)
    sw      $t2, 4352($v1)
    addiu   $a0, $a0, 16
    bgtz    $at, 4b
    sw      $t3, 4352($v1)
3:
    beqz    $a1, 1f
    nop
2:
    lwr     $at, 0($a0)
    lwl     $at, 3($a0)
    addiu   $a1, $a1, -4
    sw      $at, 4352($v1)
    bnez    $a1, 2b
    addiu   $a0, $a0, 4
1:
    jr      $ra
    addiu   $v0, $v0, 4096
