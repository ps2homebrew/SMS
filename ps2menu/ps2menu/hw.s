    .file   1 "hw.s"

.set nomips16
.set noat




.data 1
.p2align 4

VRendf:     .word 0
VRendid:    .word 0
VRcount:    .word 0
VRstartid:  .word 0




# DMA lists ensue

.p2align 4
set_fb1:
    .dword 0x0000000070000010, 0x0000000000000000   	# DMA tag (last dword is padding)
    .dword 0x100000000000800f, 0xfffffffffffffffe   	# GIF tag
    .dword 0x00080000, 0x4c             		# framebuf: ptr = 0, width 8*64 = 512 (frame_1)
    .dword 0x00000000, 0x18             		# offset (xyoffset_1)
    .dword 0x0a0000c0, 0x4e             		# zbuffer @ 0x180000, 16-bit (zbuf_1)
    .dword 0x00000001, 0x1a             		# use the prim register (prmodecont)
    .dword 0x00000000, 0x45             		# dither off (dthe)
    .dword 0x00000001, 0x46				# (colclamp)
    .dword 0x00030000, 0x47				# (test_1) (note: all pixels pass, to make sure clear poly overwrites all other pixels)
    .dword 0x17f000001ff00000, 0x40     		# clipping window (scissor_1)
    .dword 0x00000004, 0x00				# (prim)
    .dword 0x3f80000000000000, 0x01			# (rgbaq)
    .dword 0x0000000000000000, 0x0d			# (xyz3 w/o drawing kick)
    .dword 0x0000000000002000, 0x0d			# (xyz3 w/o drawing kick)
    .dword 0x0000000018000000, 0x05			# (xyz2)
    .dword 0x0000000018002000, 0x05			# (xyz2)
    .dword 0x00070000, 0x47				# (test_1)

.p2align 4
set_fb2:
    .dword 0x0000000070000010, 0x0000000000000000   	# DMA tag
    .dword 0x100000000000800f, 0xfffffffffffffffe   	# GIF tag
    .dword 0x00080060, 0x4c             		# framebuf: ptr = 0xc0000, width 8*64 = 512
    .dword 0x00000000, 0x18             		# offset
    .dword 0x0a0000c0, 0x4e             		# zbuffer @ 0x180000, 16-bit
    .dword 0x00000001, 0x1a             		# use the prim register
    .dword 0x00000000, 0x45             		# dither off
    .dword 0x00000001, 0x46
    .dword 0x00030000, 0x47
    .dword 0x17f000001ff00000, 0x40     		# clipping window
    .dword 0x00000004, 0x00
    .dword 0x3f80000000000000, 0x01
    .dword 0x00000000, 0x0d
    .dword 0x00002000, 0x0d
    .dword 0x18000000, 0x05
    .dword 0x18002000, 0x05
    .dword 0x00070000, 0x47




# ----------------------------
# ----------------------------
# ----------------------------





.text
.p2align 4
.set noreorder


# ----------------------------

# void install_VRstart_handler();
    .globl install_VRstart_handler
    .ent install_VRstart_handler
install_VRstart_handler:
    di

    # install handler
    li $4, 2
    la $5, VRstart_handler
    addiu $6, $0, 0
    li $3, 16
    syscall
    nop

    la $4, VRstartid
    sw $2, 0($4)

    # enable the handler
    li $4, 2
    li $3, 20
    syscall
    nop

    la $4, VRcount
    sw $0, 0($4)

    ei

    jr $31
    nop

    .end install_VRstart_handler

# untested
# void remove_VRstart_handler();
    .globl remove_VRstart_handler
    .ent remove_VRstart_handler
remove_VRstart_handler:
    di

    lui $2, %hi(VRstartid)
    addiu $4, $0, 2
    ori $2, %lo(VRstartid)
    addiu $3, $0, 17
    lw $5, 0($2)
    syscall
    nop

    ei

    jr $31
    nop

    .end remove_VRstart_handler
# untested

    .ent VRstart_handler
VRstart_handler:
    la $2, VRcount
    lw $3, 0($2)
    nop
    addiu $3, 1
    sw $3, 0($2)

    daddu $2, $0, $0

    jr $31
    nop

    .end VRstart_handler

    .set at
# clears flag and waits until it gets reset (blocking call)
# void WaitForNextVRstart(int numvrs);
# numvrs = number of vertical retraces to wait for
    .globl WaitForNextVRstart
    .ent WaitForNextVRstart
WaitForNextVRstart:
    la $2, VRcount
    sw $0, 0($2)

WaitForNextVRstart.lp:
    lw $3, 0($2)
    nop
    blt $3, $4, WaitForNextVRstart.lp
    nop

    jr $31
    nop

    .end WaitForNextVRstart
    .set noat

# has start-of-Vertical-Retrace occurred since the flag was last cleared ?
# (non-blocking call)
# int TestVRstart();
    .globl TestVRstart
    .ent TestVRstart
TestVRstart:
    la $3, VRcount
    lw $2, 0($3)

    jr $31
    nop

    .end TestVRstart


# clear the start-of-Vertical-Retrace flag
# void ClearVRcount();
    .globl ClearVRcount
    .ent ClearVRcount
ClearVRcount:
    la $2, VRcount
    sw $0, 0($2)

    jr $31
    nop

    .end ClearVRcount

# ----------------------------
# ----------------------------


# void initGraph(int mode);
# mode = 3 for PAL, 2 for NTSC
    .globl initGraph
    .ent initGraph
initGraph:
    addiu $29, -8
    sd $4, 0($29)           # push 'mode'

    lui $3, 0x1200
    daddiu $2, $0, 0x0200
    sd $2, 0x1000($3)       # reset GS
    sync.p                  # definitely a fun instruction!
    sd $0, 0x1000($3)

    # putIMR thru bios (GsPutIMR)
    ori $4, $0, 0xff00
    addiu $3, $0, 0x0071
    syscall
    nop

    # sceSetGSCrt
    ori $4, $0, 1           # interlace mode (for high vertical resolution)
    ld $5, 0($29)           # NTSC/PAL mode
    ori $6, $0, 0           # field mode
    addiu $3, $0, 2
    syscall
    nop

    addiu $29, 8            # clean up stack

    jr $31
    nop

    .end initGraph


# sets interlaced field-mode 512x384 ... you get 60 vr int's / second
#   which means you must wait for 2 vr's before swapping buffers
# void SetVideoMode();
    .globl SetVideoMode
    .ent SetVideoMode
SetVideoMode:
    di

    lui $2, 0x1200

    daddu $3, $0, $0
    ori $3, 0xff61
    sd $3, 0($2)

    addiu $3, $0, 1
    sd $3, 0x0020($2)

    addiu $3, $0, 0x1000
    sd $3, 0x0070($2)

    li $3, 0x0017F9FF0205C272  # the magic number
    sd $3, 0x0080($2)

    # set the background color !
    sd $0, 0x00e0($2)

    ei

    jr $31
    nop

    .end SetVideoMode


# use this function to swap framebuffers
# void SetDrawFrameBuffer(int which);
# which = 0 : framebuffer 1
#         1 : framebuffer 2
    .globl SetDrawFrameBuffer
    .ent SetDrawFrameBuffer
SetDrawFrameBuffer:
    addiu $29, -4
    sw $31, 0($29)

    jal Dma02Wait
    nop

    andi $2, $4, 1
    la $4, set_fb1

    beq $2, $0, SetDrawFrameBuffer.sendDma
    nop

    la $4, set_fb2

SetDrawFrameBuffer.sendDma:

    jal SendDma02
    nop

    lw $31, 0($29)
    addiu $29, 4

    jr $31
    nop

    .end SetDrawFrameBuffer


# use this function to set which buffer is output to the screen
# void SetCrtFrameBuffer(int which);
# which = 0: framebuffer 1
#         1: framebuffer 2
    .globl SetCrtFrameBuffer
    .ent SetCrtFrameBuffer
SetCrtFrameBuffer:

    sll $4, 31
    sra $4, 31      # we have 0->0 or 1->0xffffffff
    andi $4, 0x0060 # addr of framebuffer 2
    ori $4, 0x1000  # width

    lui $2, 0x1200
    sw $4, 0x0070($2)

    jr $31
    nop

    .end SetCrtFrameBuffer








# ----------------------------
# ----------------------------



# DMA stuff


    .set at
# Duke's DmaReset !
# void DmaReset();
    .globl DmaReset
    .ent DmaReset
DmaReset:
    sw  $0, 0x1000a080
    sw  $0, 0x1000a000
    sw  $0, 0x1000a030
    sw  $0, 0x1000a010
    sw  $0, 0x1000a050
    sw  $0, 0x1000a040
    li  $2, 0xff1f
    sw  $2, 0x1000e010
    sw  $0, 0x1000e000
    sw  $0, 0x1000e020
    sw  $0, 0x1000e030
    sw  $0, 0x1000e050
    sw  $0, 0x1000e040
    lw  $2, 0x1000e000
    ori $3,$2,1
    nop
    sw  $3, 0x1000e000
    nop
    jr  $31
    nop

    .end DmaReset
    .set noat


# the same as Duke's "SendPrim"
# void SendDma02(void *DmaTag);
    .globl SendDma02
    .ent SendDma02
SendDma02:
    li $3, 0x1000a000

    sw $4, 0x0030($3)
    sw $0, 0x0020($3)
    lw $2, 0x0000($3)
    ori $2, 0x0105
    sw $2, 0x0000($3)

    jr $31
    nop
    .end SendDma02


# Duke's Dma02Wait !
# void Dma02Wait();
    .globl Dma02Wait
    .ent Dma02Wait
Dma02Wait:
    addiu $29, -4
    sw $8, 0($29)

Dma02Wait.poll:
    lw $8, 0x1000a000
    nop
    andi $8, $8, 0x0100
    bnez $8, Dma02Wait.poll
    nop

    lw $8, 0($29)
    addiu $29, 4

    jr  $31
    nop

    .end Dma02Wait


# void DCacheFlush();
    .globl DCacheFlush
    .ent DCacheFlush
DCacheFlush:
    daddu $4,$0,$0
    li $3,0x64         # Kernel flush
    syscall
    nop
    jr  $31
    nop

    .end DCacheFlush




# ----------------------------
# ----------------------------


    .globl resetVU0
    .ent resetVU0
resetVU0:
    cfc2.ni $3, $28
    li $2, 0xffffff00
    and $2, $3
    ori $2, 2
    ctc2.ni $3, $28
    sync.p

    jr $31
    nop
    .end resetVU0


    .globl initMainThread
    .ent initMainThread
# void initMainThread();
initMainThread:
    la $4, _gp
    li $5, -1
    li $6, 0x100000
    li $7, 0x480000
    la $8, _root
    move    $28, $4
    addiu   $3, $0, 60
    syscall
    nop
    move    $29, $2

    li $4, 0
    li $3, 100
    syscall
    nop

    ei

    jr $31
    nop
    .end initMainThread


_root:
    addiu   $3, $0, 35      # ExitThread();
    syscall
    nop




# ----------------------------
# ----------------------------


    .globl qmemcpy
    .ent qmemcpy
# void qmemcpy(void *dest, void *src, int numqwords);
qmemcpy:
    lq $2, 0($5)
    addiu $6, -1
    sq $2, 0($4)
    addiu $4, 0x0010
    bnez $6, qmemcpy
    addiu $5, 0x0010

    jr $31
    nop
    .end qmemcpy


    .globl dmemcpy
    .ent dmemcpy
# void dmemcpy(void *dest, void *src, int numdwords);
dmemcpy:
    ld $2, 0($5)
    addiu $6, -1
    sd $2, 0($4)
    addiu $4, 0x0010
    bnez $6, dmemcpy
    addiu $5, 0x0010

    jr $31
    nop
    .end dmemcpy


    .globl wmemcpy
    .ent wmemcpy
# void wmemcpy(void *dest, void *src, int numwords);
wmemcpy:
    lw $2, 0($5)
    addiu $6, -1
    sw $2, 0($4)
    addiu $4, 0x0010
    bnez $6, wmemcpy
    addiu $5, 0x0010

    jr $31
    nop
    .end wmemcpy

# Dukes pal/ntsc auto-detection. Returns 3 for PAL, 2 for NTSC.
.globl	pal_ntsc
.ent	pal_ntsc
pal_ntsc:
	lui     $8,0x1fc8
	lb      $8,-0xae($8)
	li      $9,'E'
	beql    $8,$9,pal_mode
	li      $2,3                  # 2=NTSC, 3=PAL

	li		$2,2
pal_mode:
	jr		$31
	nop
.end	pal_ntsc
