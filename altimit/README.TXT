Altimit v0.1 (alpha test version)
(c)2004 t0mb0la (Thomas Hawcroft) tomhawcroft@comcast[dot]net

Thank you for trying this program. Please feel free to send any comments, recommendations or bug reports to my mailbox above.
The source code will soon be released, when the missing features are finalised, PS2SDK is fully functional and the source and makefiles have been modified to build with it.
For the time being, I'd like to hear what improvements you would like to see and any problems you encounter.

"What do I do to make it work?"

I have put two executables in the zip file:
altimit.elf; which is a version of the program without the IOP modules included, so you'll need to make the required IRX files available to it (I've included these modules in the zip for convenience (with the exception of PS2HOST.IRX, which I want to check with Pukko before releasing). Altimit looks for these files in the following locations: same folder the elf was loaded from (mc0, cdrom, host), mc0:/SYS-MODULES, mc0:/BWLINUX and pfs0:/SYS-MODULES (although pfs is probably a bit too optimistic at the moment).
altimitlzo.elf; an sjcrunch version of the program with all the IOP modules included, with the exception of USBD.IRX (which you will need to supply yourself, if you wish to use USB mouse or keyboard); you'll need to place this in one of the folders mentioned above. Please bear in mind that some versions of this IRX are not compatible, I took mine from my 'legally owned' copy of Everquest Online, but I understand the version on the Network Adapter Setup disc will work just as well.

The first time Altimit is loaded, it will create its configuration file in mc0:/SYS-CONF
Default load options are very basic, only the rom modules for memory card, rom module for pad, IOMANX.IRX, FILEXIO.IRX and CDVD.IRX are loaded.
IOMANX.IRX is excluded if you load Altimit with ps2link from host:, and the host: device should still be available.



Main program screen.

Once the program has loaded, you should be able to see the main screen where (at the moment) you can use the pad to select from File Browser, Altimit Configuration and Altimit Information.

File Browser is pretty much similar to PS2MENU, except there are not a mass of pad buttons to perform functions.
Pad up/down move the highlight bar up/down the list of objects.
Pad left/right change to parent/highlighted subfolder.
Cross changes to a highlighted folder, this will be expanded to also return file information.
Triangle closes the browser (and is cancel for most operations).
Square opens a function menu which can be used to create a new folder or perform some operation on the highlighted object.
Select toggles between a Source and a Destination list.
When first opened, you will be presented with a list of available devices, which you navigate as if they were folders.

Altimit Configuration can be used to set your preferred viewing mode and center the screen. There is a Pointer option for mouse or 'joymouse^' pointer. You can also select which devices you want to be available when the program loads; adding or removing devices will require you to reload Altimit for these changes to take place.
Pad up/down/left/right change the highlighted button.
Cross 'presses' the highlighted button.
Triangle closes the window, any changes are saved automatically.

Altimit Information does little more than display program information. This will be expanded to include instructions for program use and other information.
Pad up/down scroll the text.
Triangle closes the window.


Mouse, keyboard and pad options.

Joymouse^, if you do not have a mouse, but have an analog option on your joypad, you can use the left analog stick to move a cursor on the screen. This can be used to select window buttons by pointing the cursor and pressing the pad cross button.

Keyboard, many of the pad buttons are duplicated on the keyboard.
Keys up/down/left/right have the same effect as those on the dpad.
Tab key has the same effect as pad select.
Enter key has the same effect as pad cross.
Backspace key has the same effect as pad circle.
F1 key has the same effect as pad square.
Esc key has the same effect as pad triangle.
There is an on screen keyboard for file operations such as rename, and add folder. Joymouse, dpad or mouse can be used to press 'virtual' keys on this keyboard; clearly with a USB keyboard it's easier to type characters instead. Pad buttons L1/R1 can be used to move the text cursor left and right, and these are mapped to Home/End on a USB keyboard; keys Shift, Caps Lock, Insert, Backspace and Enter operate as you would normally expect.

Mouse, this is something I've been unable to test on my system. I don't have a PS2 USB mouse. I tried using a PC USB mouse, but it was not detected or enabled by my console.
Normal mouse operation should move the screen cursor.
Left mouse button has the same effect as pad cross.
Right mouse button has the same effect as pad square.