/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_Data_H
# define __SMS_Data_H

# define SMS_AUDSRV_SIZE      6781
# define SMS_IDCT_CONST_SIZE   368
# define SMS_IOMANX_SIZE      7417
# define SMS_FILEXIO_SIZE     8089
# define SMS_PS2DEV9_SIZE     9905
# define SMS_PS2ATAD_SIZE    11805
# define SMS_PS2HDD_SIZE     26257
# define SMS_PS2FS_SIZE      54785
# define SMS_POWEROFF_SIZE    2925
# define SMS_USB_MASS_SIZE   22717
# define SMS_CDVD_SIZE       20065
# define SMS_PS2IP_SIZE      78909
# define SMS_PS2SMAP_SIZE    12625
# define SMS_PS2HOST_SIZE    16349
# define SMS_USBD_SIZE       26105

# define SMS_AUDSRV_OFFSET     0
# define SMS_IDCT_CONST_OFFSET (  ( SMS_AUDSRV_OFFSET     + SMS_AUDSRV_SIZE     + 15 ) & 0xFFFFFFF0  )
# define SMS_IOMANX_OFFSET     (  ( SMS_IDCT_CONST_OFFSET + SMS_IDCT_CONST_SIZE + 15 ) & 0xFFFFFFF0  )
# define SMS_FILEXIO_OFFSET    (  ( SMS_IOMANX_OFFSET     + SMS_IOMANX_SIZE     + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2DEV9_OFFSET    (  ( SMS_FILEXIO_OFFSET    + SMS_FILEXIO_SIZE    + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2ATAD_OFFSET    (  ( SMS_PS2DEV9_OFFSET    + SMS_PS2DEV9_SIZE    + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2HDD_OFFSET     (  ( SMS_PS2ATAD_OFFSET    + SMS_PS2ATAD_SIZE    + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2FS_OFFSET      (  ( SMS_PS2HDD_OFFSET     + SMS_PS2HDD_SIZE     + 15 ) & 0xFFFFFFF0  )
# define SMS_POWEROFF_OFFSET   (  ( SMS_PS2FS_OFFSET      + SMS_PS2FS_SIZE      + 15 ) & 0xFFFFFFF0  )
# define SMS_USB_MASS_OFFSET   (  ( SMS_POWEROFF_OFFSET   + SMS_POWEROFF_SIZE   + 15 ) & 0xFFFFFFF0  )
# define SMS_CDVD_OFFSET       (  ( SMS_USB_MASS_OFFSET   + SMS_USB_MASS_SIZE   + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2IP_OFFSET      (  ( SMS_CDVD_OFFSET       + SMS_CDVD_SIZE       + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2SMAP_OFFSET    (  ( SMS_PS2IP_OFFSET      + SMS_PS2IP_SIZE      + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2HOST_OFFSET    (  ( SMS_PS2SMAP_OFFSET    + SMS_PS2SMAP_SIZE    + 15 ) & 0xFFFFFFF0  )
# define SMS_USBD_OFFSET       (  ( SMS_PS2HOST_OFFSET    + SMS_PS2HOST_SIZE    + 15 ) & 0xFFFFFFF0  )
# define SMS_DATA_BUFFER_SIZE  (  ( SMS_USBD_OFFSET       + SMS_USBD_SIZE       + 15 ) & 0xFFFFFFF0  )

extern unsigned char g_DataBuffer[ SMS_DATA_BUFFER_SIZE ];

# define VD_STACK &g_DataBuffer[ 0x00000 ]
# define AD_STACK &g_DataBuffer[ 0x08000 ]
# define AUD_BUFF &g_DataBuffer[ 0x10000 ]

#endif  /* __SMS_Data_H */
