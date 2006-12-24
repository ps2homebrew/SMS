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
#ifndef __SMS_SMB_H
# define __SMS_SMB_H

# define SMB_IOCTL_LOGIN  0x00000000
# define SMB_IOCTL_LOGOUT 0x00000001
# define SMB_IOCTL_MOUNT  0x00000002
# define SMB_IOCTL_UMOUNT 0x00000003
# define SMB_IOCTL_SENUM  0x00000004
# define SMB_IOCTL_ECHO   0x00000005

# define SMB_ERROR_NEGOTIATE 0x00000001
# define SMB_ERROR_LOGIN     0x00000002
# define SMB_ERROR_COMM      0x00000003

# define SMB_SENUM_SIZE 8096

typedef struct SMBLoginInfo {

 char m_ServerIP  [ 16 ] __attribute__(  ( packed )  );
 char m_ServerName[ 16 ] __attribute__(  ( packed )  );
 char m_ClientName[ 16 ] __attribute__(  ( packed )  );
 char m_UserName  [ 32 ] __attribute__(  ( packed )  );
 char m_Password  [ 64 ] __attribute__(  ( packed )  );
 int  m_fAsync           __attribute__(  ( packed )  );

} SMBLoginInfo;

typedef struct SMBMountInfo {

 int  m_Unit        __attribute__(  ( packed )  );
 char m_Path[ 512 ] __attribute__(  ( packed )  );

} SMBMountInfo;

typedef struct SMBShareInfo {

 unsigned char  m_Name[ 13 ];
 unsigned char  m_Pad;
 unsigned short m_Type;
 unsigned char* m_pRemark;

} SMBShareInfo;

typedef struct SMBSEnumInfo {

 int           m_Unit  __attribute__(  ( packed )  );
 SMBShareInfo* m_pInfo __attribute__(  ( packed )  );

} SMBSEnumInfo;
#endif  /* __SMS_SMB_H */
