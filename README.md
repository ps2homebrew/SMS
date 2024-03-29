# Simple Media System (SMS)

Copyright 2001-2005, ps2dev - http://www.ps2dev.org
All rights reserved.
Created by Eugene Plotnikov <e-plotnikov@operamail.com>

[SMS Documentation](https://ps2homebrew.github.io/SMS/)

[![CI](https://github.com/ps2homebrew/SMS/actions/workflows/compilation.yml/badge.svg)](https://github.com/ps2homebrew/SMS/actions?query=workflow%3ACI-compile)

## Introduction

Simple Media System (SMS) is a result of my curiosity about Sony's Playstation2, MIPS, DivX and MP3. My original goal was to create a simplest DivX player able to play good resolution movies at good frame rate on the unmodded PS2 without any extra equipment such as HDD and network adapter.

Why "system"? Just because having only a player program is not enough. Since PS2 is forced to ignore "illegal" disks, I've took an idea of Drakonite with his UMCDR and developed my own "UMCDR". (I called it CDDAFS). It just stores data on CD audio disk, which can be read on
(I hope) any PS2. So this is a first component of SMS. Second component is a PC program that makes cue/bin files in order to burn such a disk. And, finally, comes a player program itself. This system is not complete yet, but it can play DivX movies with sound at good frame rate already. Player's base is a famous ffmpeg project. I took just some parts of it (DivX 5XX and MP3) and made some modifications specific to PS2 hardware. I've tested it with a couple of DivX/XviD movies with different screen resolutions and encoding methods and it seems to be working. So, I've decided to put all this stuff to the CVS at ps2dev in hope that it could be interesting for PS2 enthusiasts who use it as base for their development. This "document" provides basic info about SMS.

## CDDAFS

It's just a method of storing a data on audio CD, so PS2 hardware can authenticate such a CD as "valid" one. Logical structure of such a CD is quite simple. Audio tracks are "directories". Track 0 is a control track, which stores global disk information.

Following "diagram" presents a logical disk structure:

    Track 0:
    GUID             - 16  bytes
    Disk name        - 64  bytes
    Disk description - 512 bytes
    Disk picture     - 9216 bytes (48x48x4 raw RGB image)[^1]
    ; then follows image table
    Number of images - 2 bytes (short, little endian)[^1]
    Image 1          - 4096 bytes (32*32*4 raw RGB image)[^1]
    .......
    Image N          - 4096 bytes (32*32*4 raw RGB image)[^1]
    ; then follows "directory" table (number of directories is number of tracks from CD TOC minus one)
    Track name        1 - 32 bytes
    Track image index 1 -  2 bytes (short, little endian, zero based)
    Reserved          1 - 30 bytes (just for possible future enhancements)
    Track name        2 - 32 bytes
    Track image index 2 -  2 bytes (short, little endian, zero based)
    Reserved          2 - 30 bytes (just for possible future enhancements)
    .......
    Other tracks:
    ; Directory entry 1
    Number of files     -  2 bytes (short, little endian)
    File name           - 32 bytes
    File image index    -  2 bytes (short, little endian, zero based)
    Reserved            - 30 bytes
    Start offset        -  4 bytes (physical sector relative to start sector of the current track, int, little endian, zero based)
    File size(in bytes) -  4 bytes (int, little endian)
    ; Directory entry 2
    ; etc.
    ; File data

[^1]: these "pictures" and "images" are in fact RGBA raw bitmaps, so future browser can be decorated with them;

Such a disk (without extra ECC codes) proved reliable enough in my configuration.
Source code that handles CD I/O located in CDDA.c (low level sector read operations) and in FileContext.c (logical read operations). I've also started implementation of "standard" I/O (STIO_xxx routines). Well, for me CDDA is enough. So anyone can finish "standard" I/O (perhaps, using asynchronous fileXio routines from ps2sdk).

## PC program

I've called it CDXplorer. Not fully completed yet, but it does its job of
making cue/bin pair. This program is uses Win32 API, so it is not compatible with linux (even WINE has troubles running it). I'm using Nero's Disk-At-Once/96 burning option. It is also possible to read CDDAFS using this proggy. Just insert CDDAFS disk in the drive and run the program. Maybe someone would like to create fs driver to support it :)?

## SMS Player

Just a test program for now and PS2 optimized ffmpeg. No GUI. It is possible to benchmark video/audio decoding process (I've got sometimes peak ~70 fps with 640x464 movie (~60fps is average)). I was able to play 720x528 DivX with sound at 25fps (but in this case there're slight sound distortions on scenes with a lot of motions (mainly "global" ones). Just watching a movie is also possible. Modify main's source at your taste. For sound output I'm using 'audsrv' 0.75 (magnificent work of gawd).

## ffmpeg modifications

Two things here. Using IPU for colorspace conversion (interrupt driven process that performs CSC->Display sequence for 640x464 picture in just ~4ms). This process goes in parallel with video/audio decoding. Second, I've made a transition from planar image format to the packed one, so video decoder produces macroblocks in IPU format.
This notable reduced data traffic between main CPU and memory. SPR is also
heavily used in conjunction with DMA transfers. This things gave ~300% performance boost (comparing to the original ffmpeg) in video decoding. MP3 decoder is virtually unmodified.

## Things to do
 ----------------------------------------------------------------------
There still a lot to do. As I'm a professional programmer, I just can't do this alone quickly. SMS is just a working skeleton of the future player. I've created it in my own free time. The following things I'd like to do with it:
- more testing (I've tested it with just about a dozen of DivX/XviD movies);
- more optimizations (mainly for GMC decoding. I've made just an 'ad hoc' code to show that it works. But this code is ugly and slow). QPel works, but it is unoptimized too (just an original ffmpeg). The MP3 decoder from ffmpeg is also almost untouched;
- better A/V synchronization. In fact there's no synchronization at all.
  But because multithreading on PS2 is not preemptive it happens "automatically" with a surprisingly good accuracy :). The only thing to fix was initial "audio preload". I'm not sure if I made it correctly, but it seems to be working at the moment. Realtime clock with 1ms resolution is also implemented, so it could be used for more precise synchronization;
- creation of the GUI. In case of CDDAFS it would be just a disk browser.
  There's already a base for it. There're routines to use ROM font (FONTM) and display icons from CDDAFS disk. There're also routines to load directory/file lists. The only thing missing is a pad management and an event loop;
- implementation of pause/stop functionality; As movie starts, there's no way back now;
- error checking (there're minimal checks only, so if, for example, press 'eject' CD button, then...a very interesting effect can be achieved :);
- btw, can somebody modify Makefile in order to make whole project more structured (all these 'ee', 'iop' etc. directories)?

## Restrictions
 ----------------------------------------------------------------------
- no support for interlaced movies;
- no support for non-interleaved avi files;
- only CodecID_MPEG4 support (that is DivX 5XX/XviD (I never, however, tried these 'MP4S', 'BLZ0' etc. (what are they?)) and MP3;
- other possible restrictions due to lack of detailed knowledge about things :);

## How to compile
 ----------------------------------------------------------------------
I'm using latest toolchain/ps2sdk from ps2dev and Win32 platform. Just 'make'.
Do not use 'old' ps2sdk (earlier than january 2005), since there's a bug in 'realloc', so program crashes immediately. In my setup I use ps2link at PS2 side and InLink at the PC. Note also that performace depends on PS2 cache usage, so even changing link order in Makefile can get more than 10% penalty (Currently Makefile contains 'best' link order (I think so). Pay attention to the location of 'audsrv.irx' (currently 'host:audsrv.irx').

## How to debug
 ----------------------------------------------------------------------
Crazy enough, but I've made the code Win32 compatible (with exception of some files). So, I'm using MSVC6 to debug major bugs, memory leaks and to understand ffmpeg's code. 'printf' does debug for PS2 :);

## How to make modifications
 ----------------------------------------------------------------------
At your taste (be careful, any alignment failure causes a crash (especially with structures allocated dynamically)). But first compile and run existing code (the only thing to modify is a filename in 'main.c'). To do that it's necessary to waste at least one CD-R for CDDAFS. Use CDXplorer application to make cue/bin pair. It is possible to copy files back from CDDAFS disk. Drag-and-drop is supported (one direction only: to the application). Context menus are also available. Number of directories has upper limit of 99 (only one level), and number of files is restricted by the CD capacity (I've put a 800MB movie, for example, on the ordinary 700MB disk without any problem). I have SCPH-30004R PAL unit, so it works at least on this model.

## Modified ps2sdk code
 ----------------------------------------------------------------------
It could be noted that there's a lot of modified ps2sdk code here. Why?

I wanted maximum performance. For 'audsrv' for example there's an unnecessary (I think so) calls for memcpy and FlushCache. 'libcdvd' caused strange problems of data corruprion when SifLoadModule was called. GSkit uses SPR badly needed for video decoding. Instead, I decode MP3 stream directly to the uncached, 64 byte aligned memory, I'm using 64 byte aligned buffers for disk I/O, and GS kit was modified to avoid SPR usage. Maybe I'm just a stupid person :o).

## License
 ----------------------------------------------------------------------
For my code it is the same as for ps2sdk (AFL). For ffmpeg - "GNU Lesser General Public License as published by the Free Software Foundation;
either version 2 of the License, or (at your option) any later version" :).

## Miscellaneous
 ----------------------------------------------------------------------
I've made this just for myself. I don't want to make any profit from it. Anyone is free to make anything with it (according to aforementioned licenses). If my code sucks (especially my formatting habits) - it sucks. I don't care. I just hope that it will help PS2 enthusiasts to make their programs quicker. I will continue slowly to make it more like a real system, but for me alone
it will take ages to finish it :).

## Questions
 ----------------------------------------------------------------------
- by email (see beginning of file);
- using private messages at ps2dev's forums;

## Thanks (in no particular order) ;o)
 ----------------------------------------------------------------------
- Sony for PS2 and Linux for it;
- Marcus R. Brown for discovering an exploit (without it I would never begin);
- Drakonite for UMCDR idea;
- Pixel for making an estimation of the code and for helping me out with CVS;
- J.F. for trying this out (btw. I don't know if you succeeded with it or not);
- PS2Reality team for inspiration and timer code;
- ffmpeg contributors;
- Chris "Neovanglist" Gilbert for GS and DMA code;
- gawd for 'audsrv' (virgin version was enough :));
- all others whose posts in the forums helped me in development;

[Changelog](CHANGELOG.md)

[Documentation](docs/README.md)
