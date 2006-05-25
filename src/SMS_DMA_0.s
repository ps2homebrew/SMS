.set noreorder
.set noat
.set nomacro

.globl DMA_Stop

.text

DMA_Stop:
    lui     $v1, 0x0001
1:
    di
    sync.p
    mfc0    $v0, $12
    and     $v0, $v0, $v1
    nop
    bne     $v0, $zero, 1b
    lui     $v0, 0x1001
    lui     $a3, 0xFFFE
    lw      $a1, -2784($v0)
    ori     $a3, 0xFFFF
    or      $a1, $a1, $v1
    sra     $v1, $a3, 8
    sw      $a1, -2672($v0)
    lw      $a2, 0($a0)
    and     $a2, $a2, $v1
    sw      $a2, 0($a0)
    lw      $a1, -2784($v0)
    and     $a1, $a1, $a3
    sw      $a1, -2672($v0)
    jr      $ra
    ei
