Multi-Z80 32 Bit emulator 
Copyright 1996, 1997, 1998, Neil Bradley, All rights reserved

			    MZ80 License agreement
			    -----------------------

(MZ80 Refers to both the assembly code emitted by makez80.c and makez80.c
itself)

MZ80 May be distributed in unmodified form to any medium.

MZ80 May not be sold, or sold as a part of a commercial package without
the express written permission of Neil Bradley (neil@synthcom.com). This
includes shareware.

Modified versions of MZ80 may not be publicly redistributed without author
approval (neil@synthcom.com). This includes distributing via a publicly
accessible LAN. You may make your own source modifications and distribute
MZ80 in source or object form, but if you make modifications to MZ80
then it should be noted in the top as a comment in makez80.c.

MZ80 Licensing for commercial applications is available. Please email
neil@synthcom.com for details.

Synthcom Systems, Inc, and Neil Bradley will not be held responsible for
any damage done by the use of MZ80. It is purely "as-is".

If you use MZ80 in a freeware application, credit in the following text:

"Multi-Z80 CPU emulator by Neil Bradley (neil@synthcom.com)"

must accompany the freeware application within the application itself or
in the documentation.

Legal stuff aside:

If you find problems with MZ80, please email the author so they can get
resolved. If you find a bug and fix it, please also email the author so
that those bug fixes can be propogated to the installed base of MZ80
users. If you find performance improvements or problems with MZ80, please
email the author with your changes/suggestions and they will be rolled in
with subsequent releases of MZ80.

The whole idea of this emulator is to have the fastest available 32 bit
Multi-Z80 emulator for the x86, giving maximum performance.
 
                         MZ80 Contact information
			  -------------------------

Author      : Neil Bradley (neil@synthcom.com)
Distribution: ftp://ftp.synthcom.com/pub/emulators/cpu/makez80.zip (latest)

You can join the cpuemu mailing list on Synthcom for discussion of Neil
Bradley's Z80 (and other) CPU emulators. Send a message to 
"cpuemu-request@synthcom.com" with "subscribe" in the message body. The
traffic is fairly low, and is used as a general discussion and announcement
for aforementioned emulators.


			     MZ80 Documentation
			     -------------------

MZ80 Is a full featured Z80 emulator coded in 32 bit assembly. It runs well
over a hundred games, in addition to it supporting many undocumented Z80
instructions required to run some of the Midway MCR games.

MZ80 Contains a makez80.c program that must be compiled. It is the program
that emits the assembly code that NASM will compile. This minimizes the
possibility of bugs creeping in to MZ80 for the different addressing modes
for each instruction. It requires NASM 0.97 or greater.

The goal of MZ80 is to have a high performance Z80 emulator that is capable
of running multiple emulations concurrently at full speed, even on lower-end
machines (486/33). MZ80 Harnesses the striking similarities of both the Z80
and the x86 instruction sets to take advantage of flag handling which greatly
reduces the time required to emulate a processor, so no extra time is spent
computing things that are already available in the native x86 processor,
allowing it to perform leaps and bounds over comparable C based Z80 emulators
on the same platform.

MZ80 Is designed exclusively for use with NASM, the Netwide Assembler. This
gives the ultimate in flexibility, as NASM can emit object files that work
with Watcom, Microsoft Visual C++ (4.0-current), DJGPP, Borland C++, and
gcc under FreeBSD or Linux. MZ80 Has been tested with each one of these
compilers and is known to work properly on each.


			    What's in the package
			    ---------------------

MZ80.TXT               - This text file

MAKEZ80.C              - Multi Z80 32 Bit emulator emitter program

MZ80.H                 - C Header file for MZ80 functions


			  What's new in this release
			  --------------------------

Revision 2.4:

	* Fixed improper HALT handling (didn't advance the PTR when it should)
	* Fixed SRL (IX+$xx) instruction so that carry wasn't trashed
	* Fixed single stepping problems with it giving too much time to 
	  any given instruction
	* Fixed half carry flag handling with 16 bit SBC and ADD instructions
	* Fixed DAA emulation so that parity flags weren't getting trashed

Revision 2.3:

	* Fixed many stack handling bugs
	* Timing problems fixed. The prior version was causing massive 
	  overruns on maximum timeslices with some insutructions.

Revision 2.2:

	* Fixed a bug in CPI/CPD/CPIR/CPDR that mishandled flags
	* All known bugs are out of mz80 now
	* Added the -cs option to route all stack operations through the
	  handlers (required for games like Galaga)

Revision 2.1:

	* Fixed a bug in CPI/CPD/CPIR/CPDR that caused intermittent lockups.
	  Also fixed a bug that caused erratic behavior in several video games.
	* Added INI/IND/INIR/INDR instruction group
	* Added OUTI/OUTD/OTIR/OTDR instruction group

Revision 1.0:

	* First release! The whole thing is new!


ASSEMBLING FOR USE WITH WATCOM C/C++
------------------------------------

Watcom, by default, uses register calling conventions, as does MZ80. To
create a proper emulator for Watcom:

	makez80 MZ80.asm

From here:

	nasm -f win32 MZ80.asm

Link the MZ80.obj with your Watcom linker.


ASSEMBLING FOR USE WITH MICROSOFT VISUAL C++ AND BORLAND C++
--------------------------------------------------------------------

Visual C++ and Borland C++ use stack calling conventions by default. To
create a proper emulator for these compilers:

	makez80 MZ80.asm -s

For Visual C++ or Borland C++:

	nasm -f win32 MZ80.asm

Link with your standard Visual C++ or Borland C++.


ASSEMBLING FOR USE WITH DJGPP, GCC/FREEBSD, OR GCC/LINUX
--------------------------------------------------------------------

DJGPP Uses stack calling conventions:

	makez80 MZ80.asm -s

To assemble:

	nasm -f coff MZ80.asm

Link with your standard DJGPP linker. The same holds true for GCC under
FreeBSD or Linux.


STEPS TO EMULATION
------------------

NOTE: -ss Is the "single stepping" option that will cause a single instruction
to be executed. It will ignore the value you pass into mz80exec(),
execute one instruction, and return. dwElapsedTicks will contain the time of
the single instruction executed. It will slow the emulator down by a factor
of 5 at least, but will allow you to generate tracefiles in the unlikely
event of a CPU emulator bug. ;-) The operation of the emulator otherwise
will not be affected. The CPUBENCH.C program has been updated to be able to
identify the different versions.

NOTE: -16 Is a command line option that will treat all I/O as 16 bit. That
is, in an instruction like "IN AL, (C)", the addressed passed to the I/O
handler will be BC instead of just C. Bear this in mind when considering your
emulated platform.

There are a few steps you want to go through to get proper emulation, and a
few guidelines must be followed.

1) Create a MZ80CONTEXT

2) Create your virtual 64K memory space using whatever means of obtaining
   memory you need to do.

3) Set mz80Base in your context to be the base of your 64K memory space

4) Load up your image to be emulated within that 64K address space.

5) Set z80IoRead and z80IoWrite to their appropriate structure arrays. Here's
   an example:

struct z80PortRead ReadPorts[] =
{
	{0x10,	0x1f,	SoundChip1Read},
	{0x20,	0x2f,	SoundChip2Read}
	{(UINT32) -1,     (UINT32) -1, NULL}
};

When an IN instruction occurs, mz80 will probe this table looking for a
handler to the address of the "IN" instruction. If it is found in the list,
it's up to the handler to return the proper value. Otherwise, a value of
0ffh is returned internally if no handler for that I/O address is found. In
the case above, SoundChip1Read is called when the I/O address is between 0x10-
0x1f. A similar structure is used for I/O writes as well (OUT):

struct z80PortWrite WritePorts[] =
{
	{0x20,	0x2f,	SoundChip2Write},
	{0x30,	0x36,	VideoCtrlWrite},
	{(UINT32) -1, 	(UINT32) -1, NULL}
}

Of course, this does the opposite that the z80PortRead struct, and instead
looks for a handler to hand some data to. If it doesn't find an appropriate
handler, nothing happens.

6) Set mz80MemoryRead & mz80MemoryWrite to their appropriate structure
   arrays. Here is an example:

struct MemoryWriteByte GameWrite[] =
{
	{0x3000, 0x3fff,  VideoWrite},
	{0x4000, 0x4fff,  SpriteWrite},
	{(UINT32) -1,     (UINT32) -1, NULL}
};

The above example says that any time a write occurs in the 0x3000-0x3fff
range, call the VideoWrite routine. The same holds true for the SpriteWrite
region as well.

NOTE: When your write handler is called, it is passed the address of the
write and the data that is to be written to it. If your handler doesn't
write the data to the virtual image, the mz80 internal code will not.

NOTE: These routines will *NOT* be called when execution asks for these
addresses. It will only call them when a particular instruction uses the
memory at these locations.

If you wish for a region to be RAM, just leave it out of your memory region
exception list. The WriteMemoryByte routine will treat it as read/write
RAM and will write to mz80Base + addr directly.

If you wish to protect ROM regions (not often necessary), create a range that
encompasses the ROM image, and have it call a routine that does nothing. This
will prevent data from being written back onto the ROM image.

Leave your last entry in the table as shown above, with a null handler and
0xffffffff-0xffffffff as your read address. Even though the Z80 only
addresses 64K of space, the read/write handlers are defined as 32 bit so
the compiler won't pass junk in the upper 16 bits of the address lines. Not
only that, it allows orthoganality for future CPU emulators that may use
these upper bits.

You can do a mz80GetContext() if you'd like to read the current context of
the registers. Note that by the time your handler gets called, the program
counter will be pointing to the *NEXT* instruction.

struct MemoryReadByte GameRead[] =
{
	{0x2000,        0x200f,         ReadHandler},
	{(UINT32) -1,     (UINT32) -1,  NULL}
};

Same story here. If you have a special handler for an attempted read at a
particular address, place its range in this table and create a handler
routine for it.   

If you don't define a handler for a particular region, then the ReadMemoryByte
in mz80.ASM will actually read the value out of mz80Base + the offset 
required to complete the instruction.

7) Set the intAddr and nmiAddr to the addresses where you want mz80 to start
   executing when an interrupt or NMI happens. Take a look at the section
   entitled "INTERRUPTS" below for more information on this.

8) Call mz80SetContext() on your Z80 context

9) Call mz80Reset(). This will prime the program counter and cause a virtual
   CPU-wide reset.

10) Once you have those defined, you're ready to begin emulation. There's some
    sort of main loop that you'll want. Maybe something like:

	while (hit == 0)
	{
		if (lastSec != (UINT32) time(0))
		{
			diff = (mz80clockticks - prior) / 3000000;
			printf("%ld Clockticks, %ld frames, %ld Times original speed\n", MZ80clockticks - prior, frames, diff);
			frames = 0;
			prior = mz80clockticks;
			lastSec = time(0);
			if (kbhit())
			{
				getch();
				hit = 1;
			}
		}

		/* 9000 Cycles per NMI (~3 milliseconds @ 3MHZ) */

		dwResult = mz80exec(9000);
		mz80clockticks += mz80GetElapsedTicks(TRUE);
		mz80nmi();

		/* If the result is not 0x80000000, it's an address where
		   an invalid instruction was hit. */

		if (0x80000000 != dwResult)
		{
			mz80GetContext(&sCpu1);
			printf("Invalid instruction at %.2x\n", sCpu1.MZ80pc);
			exit(1);
		}
	}

Call mz80exec() With the # of virtual CPU cycles you'd like mz80 to
execute. Be sure to use the mz80GetElapsedTicks() call *AFTER* execution to
see how many virtual CPU cycles it actually executed. For example, if you tell
mz80 to execute 500 virtual CPU cycles, it may only execute 496, but it
will never go over the value you pass to it. Use the mz80GetElapsedTicks()
call for more accurate cycle counting. If you pass FALSE to the
mz80GetElapsedTicks() function, the internal CPU elapsed tick clock will not
be reset. The elapsed tick counter is something that continues to increase
every emulated instruction, and like an odometer, will keep counting unless
you pass TRUE to mz80GetElapsedTicks(), of which case it will return you
the current value of the elapsed ticks and set it to 0 when complete.

NOTE: The bigger value you pass to mz80exec, the greater benefit you get out
of the virtual registers persisting within the emulator, and it will run
faster. Pass in a value that is large enough to take advantage of it, but
not so often that you can't handle nmi or int's properly.

If you wish to create a virtual NMI, call mz80nmi(), and it will be taken
the next time you call mz80exec. Note that mz80nmi() doesn't actually
execute any code.

If you wish to create a virtual interrupt, call mz80int(), and it will be
taken the next time you call mz80exec. mz80int() Doesn't actually execute
any code. NOTE: mz80int() is defined with a UINT32 as a formal parameter.
Depending upon what interrupt mode you're executing in (described later),
it may or may not take a value.

NMI's can interrupt interrupts, but not the other way around. If your program
is already in an interrupt, another one will not be taken. The same holds
true for an NMI - Just like a real Z80!


MUTLI-PROCESSOR NOTES
---------------------

Doing multi processor support is a bit trickier, but is still fairly straight-
forward.

For each processor to be emulated, go through steps 1-7 above - giving each
CPU its own memory space, register storage, and read/write handlers.


EXECUTION OF MULTI-CPU'S:
-------------------------

When you're ready to execute a given CPU, do the following:

	mz80SetContext(contextPointer);

This will load up all information saved before into the emulator and ready it
for execution. Then execute step 7 above to do your virtual NMI's, interrupts,
etc... All CPU state information is saved within a context.

When the execution cycle is complete, do the following to save the updated
context away for later:

	mz80GetContext(contextPointer);

Give each virtual processor a slice of time to execute. Don't make the values
too small or it will spend its time swapping contexts. While this in itself
isn't particularly CPU expensive, the more time you spend executing the better.
mz80 Keeps all of the Z80 register in native x86 register (including most
of the flags, HL, BC, and A). If no context swap is needed, then you get the
added advantage of the register storage. For example, let's say you were 
running two Z80s - one at 2.0MHZ and one at 3.0MHZ. An example like this 
might be desirable:

	mz80SetContext(cpu1Context);	// Set CPU #1's information
	mz80exec(2000);		// 2000 Instructions for 2.0MHZ CPU
	mz80GetContext(cpu1Context);	// Get CPU #1's state info

	mz80SetContext(cpu2Context);	// Set CPU #2's state information
	mz80exec(3000);		// 3000 Instructions for 3.0MHZ CPU
	mz80GetContext(cpu2Context);	// Get CPU #2's state information

This isn't entirely realistic, but if you keep the instruction or timing
ratios between the emulated CPUs even, then timing is a bit more accurate.

NOTE: If you need to make a particular CPU give up its own time cycle because
of a memory read/write, simply trap a particular address (say, a write to a
slave processor) and call mz80ReleaseTimeslice(). It will not execute any 
further instructions, and will give up its timeslice. Put this in your 
read/write memory trap.

NOTE: You are responsible for "holding back" the processor emulator from
running too fast.


INTERRUPTS
----------

The Z80 has three interrupt modes: IM 0 - IM 2. Each act differently. Here's
a description of each:

IM 0

This mode is the "default" mode that the Z80 (and mz80 for that matter) comes
up in. When you call mz80reset(), the interrupt address is set to 38h and
the NMI address is set to 66h. So when you're in IM 0 and mz80int() is
called, the formal parameter is ignored and the z80intAddr/z80nmiAddr values
are appropriately loaded into the program counter.

IM 1

This mode will cause the Z80 to be able to pull a "single byte instruction"
off the bus when an interrupt occurs. Since we're not doing bus cycle
emulation, it acts identically to mode 0 (described above). The formal
parameter to mz80int() is ignored.

IM 2

This mode causes the Z80 to read the upper 8 bits from the current value
of the "I" register, and the lower 8 bits from the value passed into mz80int().
So, if I contained 35h, and you did an mz80int(0x64), then an interrupt at
address 3564h would be taken. Simple!


OTHER GOODIES
-------------

MZ80 Has a nice feature for allowing the same handler to handle different
data regions on a single handler. Here's an example:

struct PokeyDataStruct Pokey1;
struct PokeyDataStruct Pokey2;

struct MemoryWriteByte GameWrite[] =
{
	{0x1000, 0x100f,  PokeyHandler, Pokey1},
	{0x1010, 0x101f,  PokeyHandler, Pokey2},
	{(UINT32) -1,     (UINT32) -1, NULL}
};

void PokeyHandler(UINT32 dwAddr, UINT8 bData, struct sMemoryWriteByte *psMem)
{
	struct PokeyDataStruct *psPokey = psMem->pUserArea;

	// Do stuff with psPokey here....
}

This passes in the pointer to the sMemoryWriteByte structure that caused
the handler to be called. The pUserArea is a user defined address that can
be anything. It is not necessary to fill it in with anything or even
initialize it if the handler doesn't actually use it.

This allows a single handler to handle multiple data references. This is
particularly useful when handling sound chip emulation, where there might
be more than one of a given device. Sure beats having multiple unique
handlers that are identical with the exception of the data area where it
writes! This allows a good deal of flexibility.

The same construct holds for MemoryReadByte, z80PortRead, and z80PortWrite,
so all can take advantage of this feature.


FINAL NOTES
-----------

I have debugged MZ80.ASM to the best of my abilities. There might still be
a few bugs floating around in it, but I'm not aware of any. If you see any
problems, please point them out to me, as I am eager to make mz80 the best
emulator that I can. 

If you have questions, comments, etc... about mz80, please don't hesitate
to send me an email. And if you use mz80 in your emulator, I'd love to take
a look at your work. If you have special needs, or need implementation
specific hints, feel free to email me, Neil Bradley (neil@synthcom.com). I
will do my best to help you.

Enjoy!

Neil Bradley
neil@synthcom.com


