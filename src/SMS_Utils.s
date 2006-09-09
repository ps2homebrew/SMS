/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) ????? (BroadQ maybe)
# I think this code is GPL'ed (thanks JF for pointer).
# Adopted for SMS in 2006 by Eugene Plotnikov.
#
*/
.set noat
.set noreorder
.set nomacro

.globl MUL64

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
