/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_ioctl_H
#define __SMS_ioctl_H

#define _IOC(type, nr)		(((type) << 8)|(nr))

#define DEV9_CTL_TYPE		'd'

/* DEV9 devctl().  */
#define DEV9CTLSHUTDOWN 	_IOC(DEV9_CTL_TYPE, 1)	/* Shutdown DEV9.  */
#define DEV9CTLTYPE         _IOC(DEV9_CTL_TYPE, 2)	/* Return type of device.  */
#define DEV9CTLSMAPSTOP     _IOC(DEV9_CTL_TYPE, 3)  /* Shutdown SMAP */
#define DEV9CTLINIT         _IOC(DEV9_CTL_TYPE, 4)  /* Initialize DEV9 */

#ifdef _IOP
#  define PS2HDD_IOCTL_TRANSFER_DATA    0
#endif  /* _IOP */
#define PS2HDD_IOCTL_FLUSH_CACHE       1
#define PS2HDD_IOCTL_SET_PART_ERROR    2
#define PS2HDD_IOCTL_NUMBER_OF_SUBS    3
#ifdef _IOP
#  define PS2HDD_IOCTL_GETSIZE          4
#  define PS2HDD_IOCTL_ADD_SUB          5
#endif  /* _IOP */
#define PS2HDD_IOCTL_DELETE_LAST_SUB   6
#ifdef _IOP
#  define PS2HDD_IOCTL_GET_PART_ERROR   7
#endif  /* _IOP */
#define PS2HDD_IOCTL_MAX_SECTORS       8
#define PS2HDD_IOCTL_TOTAL_SECTORS     9
#define PS2HDD_IOCTL_SHUTDOWN         10
#ifdef _IOP
#  define PS2HDD_IOCTL_IDLE            11
#  define PS2HDD_IOCTL_SWAP_TMP        12
#endif  /* _IOP */
#define PS2HDD_IOCTL_SMART_STAT       13
#define PS2HDD_IOCTL_FORMAT           14
#ifdef _IOP
#  define PS2HDD_DEVCTL_SET_OSDMBR     15
#endif  /* _IOP */
#define PS2HDD_IOCTL_GET_SECTOR_ERROR 16
#define PS2HDD_IOCTL_STATUS           17

#ifdef _IOP
#  define PS2HDD_TMODE_READ  0
#  define PS2HDD_TMODE_WRITE 1

typedef struct {
 unsigned int sub;
 unsigned int sector;
 unsigned int size;     /* in sectors                     */
 unsigned int mode;		/* ATAD_MODE_READ/ATAD_MODE_WRITE */
 void*        buffer;
} hddIoctl2Transfer_t;

typedef struct {
 unsigned int start;
 unsigned int size;
} hddSetOsdMBR_t;
#endif  /* _IOP */

typedef struct PS2HDDMountInfo {

 unsigned int  m_Mode;
 unsigned char m_Path[ 124 ];

} PS2HDDMountInfo;

#define PFS_IOCTL_MOUNT         0
#define PFS_IOCTL_UMOUNT        1
#define PFS_IOCTL_GET_ZONE_SIZE 2
#define PFS_IOCTL_GET_ZONE_FREE 3
#define PFS_IOCTL_CLOSE_ALL     4

#endif  /* __SMS_ioctl_H */
