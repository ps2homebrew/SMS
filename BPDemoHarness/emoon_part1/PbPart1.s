	.file	1 "PbPart1.c"
	.section .mdebug.eabi64
	.previous
	.rdata
	.align	3
$LC1:
	.ascii	"numcoords: %d\n\000"
	.align	3
$LC0:
	.ascii	"Adress: 0x%x\n\000"
	.text
	.align	2
	.p2align 3,,7
	.globl	PbPart1_DrawObject
	.ent	PbPart1_DrawObject
PbPart1_DrawObject:
	.frame	$sp,160,$31		# vars= 0, regs= 20/0, args= 0, extra= 0
	.mask	0xc0ff0000,-16
	.fmask	0x00000000,0
	subu	$sp,$sp,160
	sd	$16,0($sp)
	move	$16,$4
	move	$4,$0
	sd	$fp,128($sp)
	sd	$23,112($sp)
	dsll	$16,$16,32
	sd	$22,96($sp)
	lui	$fp,%hi($LC1) # high
	sd	$21,80($sp)
	dli	$23,0x10000001		# 268435457
	sd	$20,64($sp)
	li	$21,1846018048			# 0x6e080000
	sd	$19,48($sp)
	move	$20,$0
	move	$19,$5
	sd	$18,32($sp)
	sd	$17,16($sp)
	ori	$21,$21,0x5
	sd	$31,144($sp)
	.set	noreorder
	.set	nomacro
	jal	PbMeshData_Get
	move	$17,$5
	.set	macro
	.set	reorder

	li	$3,805306368			# 0x30000000
	move	$18,$2
	li	$4,16777216			# 0x1000000
	dli	$2,0x30000004		# 805306372
	li	$5,83886080			# 0x5000000
	or	$16,$16,$2
	dli	$6,0x10000001		# 268435457
	li	$2,83886080			# 0x5000000
	ori	$4,$4,0x404
	sw	$2,8($17)
	ori	$5,$5,0x2
	li	$2,1812201472			# 0x6c040000
	sw	$3,12($17)
	sw	$4,40($17)
	addu	$18,$18,16
	sw	$2,44($17)
	move	$22,$4
	sd	$16,32($17)
	addu	$17,$17,80
	sd	$6,48($19)
	sw	$5,76($19)
	sd	$6,0($19)
	sw	$0,16($19)
	sw	$0,20($19)
	sw	$0,24($19)
	sw	$0,28($19)
	sw	$0,56($19)
	sw	$0,60($19)
	sw	$0,64($19)
	sw	$0,68($19)
	sw	$0,72($19)
	.p2align 3
$L6:
	lui	$2,%hi($LC0) # high
	lw	$16,0($18)
	addu	$18,$18,16
	addiu	$4,$2,%lo($LC0) # low
	.set	noreorder
	.set	nomacro
	jal	out
	move	$5,$18
	.set	macro
	.set	reorder

	addiu	$4,$fp,%lo($LC1) # low
	.set	noreorder
	.set	nomacro
	jal	out
	move	$5,$16
	.set	macro
	.set	reorder

	sd	$23,0($17)
	addu	$17,$17,8
	addu	$2,$20,1
	sw	$22,0($17)
	andi	$20,$2,0xffff
	addu	$17,$17,4
	sw	$21,0($17)
	addu	$17,$17,4
	sw	$0,0($17)
	addu	$17,$17,4
	sw	$0,0($17)
	addu	$17,$17,4
	sw	$0,0($17)
	addu	$17,$17,4
	sw	$0,0($17)
	.set	noreorder
	.set	nomacro
	beq	$20,$0,$L6
	addu	$17,$17,4
	.set	macro
	.set	reorder

	dli	$2,0x70000001		# 1879048193
	sd	$2,0($17)
	addu	$17,$17,8
	sw	$0,0($17)
	addu	$17,$17,4
	sw	$0,0($17)
	addu	$17,$17,4
	sw	$0,0($17)
	addu	$17,$17,4
	sw	$0,0($17)
	addu	$17,$17,4
	sw	$0,4($17)
	.set	noreorder
	.set	nomacro
	jal	PbDma_Wait01
	sw	$0,0($17)
	.set	macro
	.set	reorder

	li	$5,1			# 0x1
	.set	noreorder
	.set	nomacro
	jal	PbDma_Send01Chain
	move	$4,$19
	.set	macro
	.set	reorder

	ld	$16,0($sp)
	ld	$31,144($sp)
	move	$2,$3
	ld	$fp,128($sp)
	ld	$23,112($sp)
	ld	$22,96($sp)
	ld	$21,80($sp)
	ld	$20,64($sp)
	ld	$19,48($sp)
	ld	$18,32($sp)
	ld	$17,16($sp)
	#nop
	.set	noreorder
	.set	nomacro
	j	$31
	addu	$sp,$sp,160
	.set	macro
	.set	reorder

	.end	PbPart1_DrawObject
$Lfe1:
	.size	PbPart1_DrawObject,$Lfe1-PbPart1_DrawObject
	.ident	"GCC: (GNU) 3.2.2"
