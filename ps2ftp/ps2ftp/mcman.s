	.text
	.set	noreorder


/* ############################### IOMAN STUB ######## */
	.local	mcman_stub
mcman_stub:	.word 0x41E00000
		.word 0
		.word 0x101
		.ascii "mcman\0\0\0"
	
	.globl	mcman_call5
mcman_call5:                             # CODE XREF: sub_954+20p
		j	$31
		addiu   $0, 5

	.globl	mc_open
mc_open:                                 # CODE XREF: ms_open+18p
		j	$31
		addiu   $0, 6

	.globl mc_close
mc_close:                                # CODE XREF: ms_close+Cp
		j	$31
		addiu   $0, 7

	.globl mc_read
mc_read:				# CODE XREF: ms_read+A4p
					# ms_read+110p ...
		j	$31
		addiu   $0, 8

	.globl	mc_write
mc_write:				# CODE XREF: ms_write+30p
					# ms_write+A8p
		j	$31
		addiu	$0, 9

	.globl mc_seek
mc_seek:                                 # CODE XREF: ms_seek+18p
                j	$31
                addiu   $0, 0xA

	.globl mc_format
mc_format:                               # CODE XREF: ms_format+14p
                j	$31
                addiu   $0, 0xB

	.globl mc_getdir
mc_getdir:                               # CODE XREF: ms_getdir+70p
                j	$31
                addiu   $0, 0xC

	.globl mc_delete
mc_delete:                               # CODE XREF: ms_delete+18p
                j	$31
                addiu   $0, 0xD

	.globl mc_flush
mc_flush:                                # CODE XREF: ms_flush+Cp
                j	$31
                addiu   $0, 0xE

	.globl mc_chdir
mc_chdir:                                # CODE XREF: ms_chdir+28p
                j	$31
                addiu   $0, 0xF

	.globl mc_rename
mc_rename:                               # CODE XREF: ms_rename+48p
                j	$31
                addiu   $0, 0x10
                
	.globl mcman_call17
mcman_call17:                            # CODE XREF: sub_AB8+44p
                j	$31
                addiu   $0, 0x11

	.globl mcman_call18
mcman_call18:                            # CODE XREF: sub_BF8+6Cp
                j	$31
                addiu   $0, 0x12
                
	.globl mcman_call19
mcman_call19:                            # CODE XREF: sub_AB8+D0p
                                         # sub_DF8+13Cp
                j	$31
                addiu   $0, 0x13

	.globl mcman_call20
mcman_call20:                            # CODE XREF: sub_AB8+80p
                                         # sub_AB8+8Cp ...
                j	$31
                addiu   $0, 0x14

	.globl mcman_call29
mcman_call29:                            # CODE XREF: sub_BF8+A0p
                j	$31
                addiu   $0, 0x1D
	
	.globl mcman_call30
mcman_call30:                            # CODE XREF: sub_AB8+118p
                j	$31
                addiu   $0, 0x1E

	.globl mc_unformat
mc_unformat:                             # CODE XREF: ms_unformat+14p
                j	$31
                addiu   $0, 0x24

	.globl mcman_call37
mc_37:                                   # CODE XREF: sub_320+28p
                j	$31
                addiu   $0, 0x25

	.globl mc_getinfo3
mc_getinfo3:                             # CODE XREF: sub_954+70p
                j	$31
                addiu   $0, 0x26

	.globl mc_getinfo2
mc_getinfo2:                             # CODE XREF: sub_954+40p
                                         # sub_AB8+28p ...
                j	$31
                addiu   $0, 0x27

	.globl mcman_call40
mcman_call40:                            # CODE XREF: sub_320+1Cp
                j	$31
                addiu   $0, 0x28

	.word	0
	.word	0
