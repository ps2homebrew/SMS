/*
	in_ps2.h - PS2 USB Keyboard key map.
	
	by Nicolas Plourde <nicolasplourde@hotmail.com>
	
	Copyright (c) Nicolas Plourde - july 2004
*/

#define PS2_TAB			0x2B
#define PS2_ENTER		0x28
#define PS2_ESCAPE		0x29
#define PS2_SPACE		0x2C
#define PS2_BACKSPACE   0x2A
#define PS2_UPARROW		0x52
#define PS2_DOWNARROW   0x51
#define PS2_LEFTARROW   0x50
#define PS2_RIGHTARROW  0x4F
#define PS2_ALT			0xE2
#define PS2_CTRL		0xE0
#define PS2_SHIFT		0xE1
#define PS2_F1			0x3A
#define PS2_F2			0x3B
#define PS2_F3			0x3C
#define PS2_F4			0x3D
#define PS2_F5			0x3E
#define PS2_F6			0x3F
#define PS2_F7			0x40
#define PS2_F8			0x41
#define PS2_F9			0x42
#define PS2_10			0x43
#define PS2_11			0x44
#define PS2_12			0x45
#define PS2_INS			0x49
#define PS2_DEL			0x4C
#define PS2_PGDN		0x4E
#define PS2_PGUP		0x4B
#define PS2_HOME		0x4A
#define PS2_END			0x4D
#define PS2_PAUSE		0x48

u8 us_keymap[PS2KBD_KEYMAP_SIZE] = 
  { 
    0,
    0,
    0,
    0,
    'a',
    'b',
    'c',
    'd',
    'e',
    'f',
    'g',
    'h',
    'i',
    'j',
    'k',
    'l',
    'm',
    'n',
    'o',
    'p',
    'q',
    'r',
    's',
    't',
    'u',
    'v',
    'w',
    'x',
    'y',
    'z',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    10, /* line feed */
    0, /* Esc */
    0x7,/* BS */
    0x9, /* TAB */
    0x20,
    '-',
    '=',
    '[',
    ']',
    '\\',
    '#',
    ';',
    '\'',
    '`',
    ',',
    '.',
    '/',
    0, /* CL */
    0, // F1
    0, // F2
    0, // F3
    0, // F4
    0, // F5
    0, // F6
    0, // F7
    0, // F8 
    0, // F9
    0, // F10
    0, // F11
    0, // F12
    0, // PrintScr
    0, // Scroll Lock
    0, // Pause
    0, // Insert
    0, // Home 
    0, // Pg Up
    0, // Delete
    0, // End
    0, // Pg Down
    0, // Right
    0, // Left
    0, // Down
    0, // Up
    0, // Numlock
    '/', // Keypad 
    '*',
    '-',
    '+',
    10,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '.',
    '\\',
    0,
    0,
    '=',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };
