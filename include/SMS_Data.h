/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006/7 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_Data_H
#define __SMS_Data_H

#define SMS_PS2ATAD_SIZE    11821
#define SMS_PS2HDD_SIZE     24953
#define SMS_PS2FS_SIZE      50405
#define SMS_USB_MASS_SIZE    9950
#define SMS_USBD_SIZE        9121
#define SMS_PS2IP_SIZE      67061
#define SMS_PS2SMAP_SIZE     7909
#define SMS_PS2HOST_SIZE    16349
#define SMS_VU0_MPG_SIZE     1376
#define SMS_VU0_DATA_SIZE     256
#define SMS_SMB_SIZE        19641
#define SMS_SMSUTILS_SIZE    3733
#define SMS_AUDSRV_SIZE      8989
#define SMS_PS2DEV9_SIZE     8149
#define SMS_POWEROFF_SIZE    2229
#define SMS_CDVD_SIZE       20093
#define SMS_SIO2MAN_SIZE     5817

#define SMS_PS2ATAD_OFFSET    0
#define SMS_PS2HDD_OFFSET     (  ( SMS_PS2ATAD_OFFSET    + SMS_PS2ATAD_SIZE    + 15 ) & 0xFFFFFFF0  )
#define SMS_PS2FS_OFFSET      (  ( SMS_PS2HDD_OFFSET     + SMS_PS2HDD_SIZE     + 15 ) & 0xFFFFFFF0  )
#define SMS_USB_MASS_OFFSET   (  ( SMS_PS2FS_OFFSET      + SMS_PS2FS_SIZE      + 15 ) & 0xFFFFFFF0  )
#define SMS_USBD_OFFSET       (  ( SMS_USB_MASS_OFFSET   + SMS_USB_MASS_SIZE   + 15 ) & 0xFFFFFFF0  )
#define SMS_PS2IP_OFFSET      (  ( SMS_USBD_OFFSET       + SMS_USBD_SIZE       + 15 ) & 0xFFFFFFF0  )
#define SMS_PS2SMAP_OFFSET    (  ( SMS_PS2IP_OFFSET      + SMS_PS2IP_SIZE      + 15 ) & 0xFFFFFFF0  )
#define SMS_PS2HOST_OFFSET    (  ( SMS_PS2SMAP_OFFSET    + SMS_PS2SMAP_SIZE    + 15 ) & 0xFFFFFFF0  )
#define SMS_VU0_MPG_OFFSET    (  ( SMS_PS2HOST_OFFSET    + SMS_PS2HOST_SIZE    + 15 ) & 0xFFFFFFF0  )
#define SMS_VU0_DATA_OFFSET   (  ( SMS_VU0_MPG_OFFSET    + SMS_VU0_MPG_SIZE    + 15 ) & 0xFFFFFFF0  )
#define SMS_SMB_OFFSET        (  ( SMS_VU0_DATA_OFFSET   + SMS_VU0_DATA_SIZE   + 15 ) & 0xFFFFFFF0  )
#define SMS_SMSUTILS_OFFSET   (  ( SMS_SMB_OFFSET        + SMS_SMB_SIZE        + 15 ) & 0xFFFFFFF0  )
#define SMS_AUDSRV_OFFSET     (  ( SMS_SMSUTILS_OFFSET   + SMS_SMSUTILS_SIZE   + 15 ) & 0xFFFFFFF0  )
#define SMS_PS2DEV9_OFFSET    (  ( SMS_AUDSRV_OFFSET     + SMS_AUDSRV_SIZE     + 15 ) & 0xFFFFFFF0  )
#define SMS_POWEROFF_OFFSET   (  ( SMS_PS2DEV9_OFFSET    + SMS_PS2DEV9_SIZE    + 15 ) & 0xFFFFFFF0  )
#define SMS_CDVD_OFFSET       (  ( SMS_POWEROFF_OFFSET   + SMS_POWEROFF_SIZE   + 15 ) & 0xFFFFFFF0  )
#define SMS_SIO2MAN_OFFSET    (  ( SMS_CDVD_OFFSET       + SMS_CDVD_SIZE       + 15 ) & 0xFFFFFFF0  )
#define SMS_DATA_BUFFER_SIZE  (  ( SMS_SIO2MAN_OFFSET    + SMS_SIO2MAN_SIZE    + 63 ) & ~63         )

extern unsigned char  g_DataBuffer[ SMS_DATA_BUFFER_SIZE ] __attribute__(   (  aligned( 64 )  )   );
extern const    float g_PCMClamps [                    8 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );

#define SMS_SYNTH_BUFFER      &g_DataBuffer[ SMS_SMSUTILS_OFFSET ]
#define SMS_AUDIO_BUFFER      (   ( unsigned char* )(  ( unsigned int )( SMS_SYNTH_BUFFER + 4352 + 63 ) & ~63  )   )
#define SMS_AUDIO_BUFFER_SIZE ( &g_DataBuffer[ SMS_DATA_BUFFER_SIZE ] - SMS_AUDIO_BUFFER )
#endif  /* __SMS_Data_H */
