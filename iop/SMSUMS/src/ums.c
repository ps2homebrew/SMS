#include <usbd.h>
#include <ums.h>
#include <ums_driver.h>
#include <ums_fat.h>
#include <ums_cache.h>

#include <smsutils.h>
#include <errno.h>
#include <io_common.h>
#include <sys/stat.h>
#include <ioman.h>
#include <sysclib.h>
#include <sysmem.h>
#include <thbase.h>
#include <thsemap.h>

#include <stdio.h>

#define USB_SUBCLASS_MASS_SFF_8020I 0x02
#define USB_SUBCLASS_MASS_SFF_8070I 0x05
#define USB_SUBCLASS_MASS_SCSI      0x06
#define USB_PROTOCOL_MASS_BULK_ONLY 0x50

USBMDevice g_DevList[ MAX_DEV ];

static UsbDriver s_Driver;

static int _ums_mount ( USBMDevice*, unsigned char );

static int _ums_probe ( int aDevID ) {

 UsbDeviceDescriptor*    lpDev;
 UsbConfigDescriptor*    lpCfg;
 UsbInterfaceDescriptor* lpInt;

 lpDev = UsbGetDeviceStaticDescriptor ( aDevID, NULL, USB_DT_DEVICE );

 if ( !lpDev || lpDev -> bNumConfigurations < 1 ) return 0;

 lpCfg = UsbGetDeviceStaticDescriptor ( aDevID, lpDev, USB_DT_CONFIG );

 if ( !lpCfg || lpCfg -> bNumInterfaces < 1 ||
                lpCfg -> wTotalLength < sizeof ( UsbConfigDescriptor ) + sizeof ( UsbInterfaceDescriptor )
 ) return 0;

 lpInt = ( UsbInterfaceDescriptor* )(  ( char* )lpCfg + lpCfg -> bLength  );

 if ( lpInt -> bInterfaceClass != USB_CLASS_MASS_STORAGE || (
      lpInt -> bInterfaceSubClass != USB_SUBCLASS_MASS_SCSI &&
      lpInt -> bInterfaceSubClass != USB_SUBCLASS_MASS_SFF_8070I
     ) || lpInt -> bInterfaceProtocol != USB_PROTOCOL_MASS_BULK_ONLY
       || lpInt -> bNumEndpoints < 2
 ) return 0;

 return 1;

}  /* end _ums_probe */

static void _connect_thread ( void* apParam ) {

 USBMDevice* lpDev = ( USBMDevice* )apParam;

 if (  UmsWarmUp ( lpDev )  ) {

  if (  WaitSema ( lpDev -> m_Lock ) < 0  ) return;
   lpDev -> m_pCacheBuf = AllocSysMemory ( 0, 131072, NULL );
   if ( lpDev -> m_pCacheBuf ) {
    UmsNotifyEE ( 1, lpDev -> m_UnitID );
    lpDev -> m_fEE       = 0x01;
    lpDev -> m_CBW.m_LUN = 0xFF;
   }  /* end if */
  SignalSema ( lpDev -> m_Lock ); 

 }  /* end if */

}  /* end _connect_thread */

static int _ums_connect ( int aDevID ) {

 int                     i, lUnitID, lnEP;
 UsbDeviceDescriptor*    lpDev;
 UsbConfigDescriptor*    lpCfg;
 UsbInterfaceDescriptor* lpInt;
 UsbEndpointDescriptor*  lpEP;
 USBMDevice*             lpMyDev = NULL;
 iop_thread_t            lThread;

 for ( i = 0; i < MAX_DEV; ++i ) if ( g_DevList[ i ].m_UnitID < 0 ) {
  lpMyDev = &g_DevList[ i ];
  break;
 }  /* end for */

 if ( !lpMyDev ) return 1;

 lUnitID = i;

 lpMyDev -> m_DevID = aDevID;
 lpMyDev -> m_InpEP = -1;
 lpMyDev -> m_OutEP = -1;
 lpMyDev -> m_CtlEP = UsbOpenEndpoint ( aDevID, NULL );

 lpDev = UsbGetDeviceStaticDescriptor ( aDevID, NULL,  USB_DT_DEVICE );
 lpCfg = UsbGetDeviceStaticDescriptor ( aDevID, lpDev, USB_DT_CONFIG );
 lpInt = ( UsbInterfaceDescriptor* )(  ( char* )lpCfg + lpCfg -> bLength  );

 lpMyDev -> m_IntNum = lpInt -> bInterfaceNumber;
 lpMyDev -> m_IntAlt = lpInt -> bAlternateSetting;

 lnEP = lpInt -> bNumEndpoints;
 lpEP = UsbGetDeviceStaticDescriptor ( aDevID, NULL, USB_DT_ENDPOINT );
 UmsBulkProbeEndPoint ( lpMyDev, lpEP );

 for ( i = 1; i < lnEP; ++i ) {
  lpEP = ( UsbEndpointDescriptor* )(  ( char* )lpEP + lpEP -> bLength  );
  UmsBulkProbeEndPoint ( lpMyDev, lpEP );
 }  /* end for */

 if ( lpMyDev -> m_InpEP < 0 ) {
  if ( lpMyDev -> m_OutEP >= 0 ) UsbCloseEndpoint ( lpMyDev -> m_OutEP );
  return 1;
 }  /* end if */

 lpMyDev -> m_UnitID = lUnitID;
 lpMyDev -> m_CfgID  = lpCfg -> bConfigurationValue;
 lpMyDev -> m_Sync   = CreateMutex ( 0 );
 lpMyDev -> m_Lock   = CreateMutex ( 1 );

 lThread.attr      = TH_C;
 lThread.thread    = _connect_thread;
 lThread.stacksize = 2048;
 lThread.priority  = 64;
 lpMyDev -> m_ThreadID = CreateThread ( &lThread );
 StartThread ( lpMyDev -> m_ThreadID, lpMyDev );

 return 0;

}  /* end _ums_connect */

static void _ums_umount ( USBMDevice* apDev ) {

 if ( apDev -> m_FS.m_pData ) {
  apDev -> m_FS.Destroy ( apDev -> m_FS.m_pData );
  apDev -> m_FS.m_pData = NULL;
 }  /* end if */

 apDev -> m_CBW.m_LUN = 0xFF;

}  /* end _ums_umount */

static int _ums_disconnect ( int aDevID ) {

 int i;

 for ( i = 0; i < MAX_DEV; ++i ) if ( g_DevList[ i ].m_UnitID >= 0 &&
                                      g_DevList[ i ].m_DevID  == aDevID
                                 ) {
  USBMDevice* lpDev = &g_DevList[ i ];

  _ums_umount ( lpDev );

  UmsCloseUnit   ( lpDev );
  UmsBulkRelease ( lpDev );

  lpDev -> m_UnitID = -1;

  DeleteSema ( lpDev -> m_Sync );
  DeleteSema ( lpDev -> m_Lock );

  TerminateThread ( lpDev -> m_ThreadID );
  DeleteThread ( lpDev -> m_ThreadID );

  break;

 }  /* end for */

 return 0;

}  /* end _ums_disconnect */

static int _ums_mount ( USBMDevice* apDev, unsigned char aLUN ) {

 int i, retVal = 0;

 apDev -> m_CBW.m_LUN = aLUN;

 for ( i = 0; i < 6; ++i ) {

  if (  UmsStart ( apDev )  ) {

   DelayThread ( 1024 * 256 );

   if (  UmsTestReady ( apDev )  ) break;

  }  /* end if */

 }  /* end for */

 if ( i < 6 ) {

  if (  UmsRequestSense ( apDev ) && UmsGetCapacity ( apDev ) && FATFS_Init ( apDev )  ) {

   CacheInit ( apDev );
   retVal = 1;

  }  /* end if */

 }  /* end if */

 if ( !retVal ) {
  apDev -> m_CBW.m_LUN = 0xFF;
  apDev -> m_Status    = 0;
 }  /* end if */
#ifdef _DEBUG
 char lBuf[ 64 ];
 sprintf ( lBuf, "_ums_mount: %d\r\n", retVal );
 Log ( lBuf );
#endif  /* _DEBUG */
 return retVal;

}  /* end _ums_mount */

static const char* _ums_skip_unit_name ( const char* apName ) {

 while ( apName[ 0 ] == '/'                ) ++apName;
 while ( apName[ 0 ] && apName[ 0 ] != '/' ) ++apName;

 if ( !apName[ 0 ] ) return "/";

 return apName;

}  /* end _ums_skip_unit_name */

static int _ums_init ( iop_device_t* apDev ) {
 return 0;
}  /* end _ums_init */

static int _ums_deinit ( iop_device_t* apDev ) {
 return 0;
}  /* end _ums_deinit */

static int _ums_format ( iop_file_t* apFile, ... ) {
 return -ENOSYS;
}  /* end _ums_format */

static int _ums_open ( iop_file_t* apFile, const char* apName, int aMode, ... ) {

 int          retVal;
 unsigned int lUnit = ( unsigned int )apFile -> unit;
 USBMDevice*  lpDev;

 if ( lUnit > MAX_DEV - 1 ) return -ENODEV;

 lpDev = &g_DevList[ lUnit ];

 if (  lpDev -> m_CBW.m_LUN > 15 || WaitSema ( lpDev -> m_Lock ) < 0 || lpDev -> m_UnitID != lUnit  ) return -ENODEV;
  if ( lpDev -> m_MaxLUN ) apName = _ums_skip_unit_name ( apName );
  retVal = lpDev -> m_FS.open ( lpDev, apFile, apName, aMode );
 SignalSema ( lpDev -> m_Lock );

 return retVal;

}  /* end _ums_open */

static int _ums_close ( iop_file_t* apFile ) {

 int          retVal;
 unsigned int lUnit = ( unsigned int )apFile -> unit;
 USBMDevice*  lpDev = &g_DevList[ lUnit ];

 if (  WaitSema ( lpDev -> m_Lock ) < 0 || lpDev -> m_UnitID != lUnit  ) return -ENODEV;
  retVal = lpDev -> m_FS.close ( apFile );
 SignalSema ( lpDev -> m_Lock );

 return retVal;

}  /* end _ums_close */

static int _ums_read ( iop_file_t* apFile, void* apBuf, int aSize ) {

 int          retVal;
 unsigned int lUnit = ( unsigned int )apFile -> unit;
 USBMDevice*  lpDev = &g_DevList[ lUnit ];

 if (  WaitSema ( lpDev -> m_Lock ) < 0 || lpDev -> m_UnitID != lUnit  ) return -ENODEV;
  retVal = lpDev -> m_FS.read ( apFile, apBuf, aSize );
 SignalSema ( lpDev -> m_Lock );

 return retVal;

}  /* end _ums_read */

static int _ums_write ( iop_file_t* apFile, void* apBuf, int aSize ) {
 return -EACCES;
}  /* end _ums_write */

static int _ums_lseek ( iop_file_t* apFile, unsigned long aPos, int aDisp ) {

 int          retVal;
 unsigned int lUnit = ( unsigned int )apFile -> unit;
 USBMDevice*  lpDev = &g_DevList[ lUnit ];

 if (  WaitSema ( lpDev -> m_Lock ) < 0 || lpDev -> m_UnitID != lUnit  ) return -ENODEV;
  retVal = lpDev -> m_FS.seek ( apFile, aPos, aDisp );
 SignalSema ( lpDev -> m_Lock );

 return retVal;

}  /* end _ums_lseek */

static int _ums_ioctl ( iop_file_t* apFile, unsigned long aSize, void* apData ) {
 return -ENOSYS;
}  /* end _ums_ioctl */

static int _ums_remove ( iop_file_t* apFile, const char* apName ) {
 return -ENOSYS;
}  /* end _ums_remove */

static int _ums_mkdir ( iop_file_t* apFile, const char* apName ) {
 return -ENOSYS;
}  /* end _ums_mkdir */

static int _ums_rmdir ( iop_file_t* apFile, const char* apName ) {
 return -ENOSYS;
}  /* end _ums_rmdir */

static int _ums_dopen ( iop_file_t* apFile, const char* apName ) {

 int          retVal;
 unsigned int lUnit = ( unsigned int )apFile -> unit;
 USBMDevice*  lpDev = &g_DevList[ lUnit ];

 if (  WaitSema ( lpDev -> m_Lock ) < 0 || lpDev -> m_UnitID != lUnit  ) return -ENODEV;

  if ( lpDev -> m_CBW.m_LUN > 15 ) {

   if ( lpDev -> m_MaxLUN ) {

    if ( apName[ 0 ] == '/' && apName[ 1 ] == '\x00' ) {
root:
     apFile -> privdata = ( void* )0x80000000;
     retVal             = ( int )lpDev;

     goto end;

    } else {

     int         i, lLen = lpDev -> m_MaxLUN;
     char        lUnitName[ 19 ];
     const char* lpStart = apName;
     const char* lpEnd;

     while ( lpStart[ 0 ] == '/'             ) ++lpStart;
     lpEnd = lpStart;
     while ( lpEnd[ 0 ] && lpEnd[ 0 ] != '/' ) ++lpEnd;

     if (  ( i = lpEnd - lpStart ) > 18  ) goto error;

     mips_memcpy ( lUnitName, lpStart, i );
     lUnitName[ i ] = '\x00';

     for ( i = 0; i <= lLen; ++i ) if (  !strcmp ( lUnitName, lpDev -> m_LUName[ i ] ) && _ums_mount ( lpDev, i )  ) goto success;

     goto error;

    }  /* end else */

   } else if (  !_ums_mount ( lpDev, 0 )  ) {
error:
    _ums_umount ( lpDev );
    retVal = -ENODEV;

    goto end;

   }  /* end if */

  }  /* end if */
success:
  if ( lpDev -> m_MaxLUN ) {

   if ( apName[ 0 ] == '/' && apName[ 1 ] == '\x00' ) {

    _ums_umount ( lpDev );

    goto root;

   }  /* end if */

   apName = _ums_skip_unit_name ( apName );

  }  /* end if */
#ifdef _DEBUG
  Log ( "_ums_dopen finalizing\r\n" );
#endif  /* _DEBUG */
  if (  !UmsTestReady ( lpDev ) || !UmsRequestSense ( lpDev )  ) goto error;

  retVal = lpDev -> m_FS.dopen ( lpDev, apFile, apName );
end:
 SignalSema ( lpDev -> m_Lock );
#ifdef _DEBUG
 char lBuf[ 64 ];
 sprintf ( lBuf, "_ums_dopen: %d\r\n", retVal );
 Log ( lBuf );
 FlushLog ();
#endif  /* _DEBUG */
 return retVal;

}  /* end _ums_dopen */

static int _ums_dclose ( iop_file_t* apFile ) {

 int          retVal;
 unsigned int lUnit = ( unsigned int )apFile -> unit;
 USBMDevice*  lpDev = &g_DevList[ lUnit ];

 if (  WaitSema ( lpDev -> m_Lock ) < 0 || lpDev -> m_UnitID != lUnit ) {
  if (  ( int )apFile -> privdata > 0  ) FreeSysMemory ( apFile -> privdata );
  return -ENODEV;
 }  /* end if */
  retVal = ( int )apFile -> privdata < 0 ? 0 : lpDev -> m_FS.dclose ( apFile );
 SignalSema ( lpDev -> m_Lock );

 return retVal;

}  /* end _ums_dclose */

static int _ums_dread ( iop_file_t* apFile, void* apData ) {

 int          retVal;
 unsigned int lUnit = ( unsigned int )apFile -> unit;
 USBMDevice*  lpDev = &g_DevList[ lUnit ];

 if (  WaitSema ( lpDev -> m_Lock ) < 0 || lpDev -> m_UnitID != lUnit  ) return -ENODEV;

  if (  ( int )apFile -> privdata < 0  ) {

   int lIdx = ( int )apFile -> privdata & ~0x80000000;

   if ( lIdx > lpDev -> m_MaxLUN )

    retVal = 0;

   else {

    io_dirent_t* lpEntry = ( io_dirent_t* )apData;

    mips_memset (  apData, 0, sizeof ( io_dirent_t )  );
    strcpy ( lpEntry -> name, lpDev -> m_LUName[ lIdx ] );
    lIdx                += 1;
    lpEntry -> stat.mode = FIO_SO_IFDIR;
    apFile -> privdata   = ( void* )( lIdx | 0x80000000 );

    retVal = 1;

   }  /* end else */

  } else retVal = lpDev -> m_FS.dread ( apFile, apData );

 SignalSema ( lpDev -> m_Lock );

 return retVal;

}  /* end _ums_dread */

static int _ums_getstat ( iop_file_t* apFile, const char* apName, void* apData ) {
 return -ENOSYS;
}  /* end _ums_getstat */

static int _ums_chstat ( iop_file_t* apFile, const char* apName, void* apData, unsigned int aSize ) {
 return -ENOSYS;
}  /* end _ums_chstat */

static iop_device_ops_t s_FSDriverOps = {
 _ums_init,
 _ums_deinit,
 _ums_format,
 _ums_open,
 _ums_close,
 _ums_read,
 _ums_write,
 _ums_lseek,
 _ums_ioctl,
 _ums_remove,
 _ums_mkdir,
 _ums_rmdir,
 _ums_dopen,
 _ums_dclose,
 _ums_dread,
 _ums_getstat,
 _ums_chstat
};

static iop_device_t s_FSDriver = {
 "ums", IOP_DT_FS, 2, "USB mass storage driver", &s_FSDriverOps
};

int _start ( int argc, char** argv ) {

 int i;

 for ( i = 0; i < MAX_DEV; ++i ) g_DevList[ i ].m_UnitID = -1;

 s_Driver.name       = "ums";
 s_Driver.probe      = _ums_probe;
 s_Driver.connect    = _ums_connect;
 s_Driver.disconnect = _ums_disconnect;

 return UsbRegisterDriver ( &s_Driver ) < 0 || AddDrv ( &s_FSDriver );

}  /* end start */
#ifdef _DEBUG
char g_Log[ 16384 ];

void Log ( const char* apStr ) {

 strcat ( g_Log, apStr );

}  /* end Log */

void FlushLog ( void ) {

 int lFD = open ( "mc0:/ums.log", O_WRONLY | O_APPEND );

 if ( lFD < 0 ) lFD = open ( "mc0:/ums.log", O_WRONLY | O_APPEND | O_CREAT );

 if ( lFD >= 0 ) {
  write (  lFD, g_Log, strlen ( g_Log )  );
  close ( lFD );
 }  /* end if */

}  /* end FlushLog */
#endif  /* _DEBUG */
