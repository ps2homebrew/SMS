           TARGET      BIN, R5900
           ALIGN$$     16


code       SEGMENT     1000000h
      
           addiu       sp, -128
           sd	       s0, 0(sp)
           sd          s1, 8(sp)
           sd          s2, 16(sp)
           sd          s3, 24(sp)
           sd          s4, 32(sp)
           sd          s5, 40(sp)
           sd          s6, 48(sp)
           sd          s7, 56(sp)
           sd          ra, 64(sp)

; Start code here
           jal         start
           nop

           ld          s0, 0(sp)
           ld          s1, 8(sp)
           ld          s2, 16(sp)
           ld          s3, 24(sp)
           ld          s4, 32(sp)
           ld          s5, 40(sp)
           ld          s6, 48(sp)
           ld          s7, 56(sp)
           ld          ra, 64(sp)
           addiu       sp, 128

           jr          ra
           nop
 
start      PROC
           addiu       sp, -128
           sd          ra, 0(sp)

           move        s0, a0
           lui         s1, 0x1200

           sw	       zero, 0xe0(s1)
           lw	       t0, 0(s0) ; Load printf function
           lui         a0, (hello >> 16)
           jalr        t0
           ori         a0, a0, (hello & 0xFFFF)

loop:
           ld          v0, 0x1000(s1)
           andi        v0, v0, 8
           sd          v0, 0x1000(s1)
_vsync:    
           ld          v0, 0x1000(s1)
           andi        v0, v0, 8
           beq         v0, zero, _vsync
           lw          t0, 24(s0)       ; Load the current time value

           bgtz        t0, loop         ; If we still have time left
           nop
   
           ld          ra, 0(sp)
           jr          ra
           addiu       sp, 128
           
start      ENDP

           ALIGN 16
hello:     DB "Hello World (ASM)!",0xA,0
code       ENDS

           END
