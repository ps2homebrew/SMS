#
#  _____     ___ ____
#   ____|   |    ____|      PS2 OpenSource Project
#  |     ___|   |____
#  ------------------------------------------------------------------------
#  crt0.s                   Standard startup file.
#

.set noat
.set noreorder

.global _start
.global start_demo
	.text

	nop
	nop

	.ent _start
_start:

# Clear bss elf segment (static uninitalised data)
zerobss:
        la      $2, _fbss
        la      $3, _end
loop:
        nop
        nop
        nop
        sq      $0,($2)
        sltu    $1,$2,$3
        bne     $1,$0,loop
        addiu   $2,$2,16
        
# Main start
        addiu	$29, -128
        sd	$31, 0($29)
	jal	start_demo
	nop
        ld      $31, 0($29)
        jr	$31
        addiu   $29, 128
        
	.end	_start

	.bss
	.align	6
