#include <ums_driver.h>
#include <usbd_macro.h>
#include <smsutils.h>
#include <sifcmd.h>
#include <sifman.h>
#include <sysclib.h>
#include <sysmem.h>
#include <thbase.h>
#include <thsemap.h>

#include <stdio.h>

typedef struct CSWPack {
 unsigned int  m_Sign;
 unsigned int  m_Tag;
 unsigned int  m_DataResidue;
 unsigned char m_Status;
} CSWPack;

typedef struct SenseData {
 unsigned char m_Error;
 unsigned char m_Res1;
 unsigned char m_SenseKey;
 unsigned char m_Info[ 4 ];
 unsigned char m_AddSenseLen;
 unsigned char m_Res2[ 4 ];
 unsigned char m_AddSenseCode;
 unsigned char m_AddSenseCodeQual;
 unsigned char m_Res3[ 4 ];
} SenseData;

static unsigned int _int_be ( unsigned char* apData ) {
 return (  ( unsigned int )apData[ 0 ] << 24  ) |
        (  ( unsigned int )apData[ 1 ] << 16  ) |
        (  ( unsigned int )apData[ 2 ] <<  8  ) |
        (  ( unsigned int )apData[ 3 ] <<  0  );
}  /* end _int_be */

void UmsNotifyEE ( int aCode, int aUnit ) {

 int lID, lCmdData[ 5 ];

 lCmdData[ 3 ] = aCode;
 lCmdData[ 4 ] = aUnit;

 lID = sceSifSendCmd ( 18, lCmdData, 20, NULL, NULL, 0 );

 while (   sceSifDmaStat ( lID ) >= 0  ) DelayThread ( 1024 );

}  /* end UmsNotifyEE */

void UmsBulkProbeEndPoint ( USBMDevice* apDev, UsbEndpointDescriptor* apEP ) {

 if ( apEP -> bmAttributes == USB_ENDPOINT_XFER_BULK ) {
  if (  ( apEP -> bEndpointAddress & 0x80 ) == 0 && apDev -> m_OutEP < 0 ) {
   apDev -> m_OutEPAddr  = apEP -> bEndpointAddress;
   apDev -> m_OutEP      = UsbOpenEndpointAligned ( apDev -> m_DevID, apEP );
   apDev -> m_OutPktSize = apEP -> wMaxPacketSizeHB * 256 + apEP -> wMaxPacketSizeLB;
  } else if (  ( apEP -> bEndpointAddress & 0x80 ) != 0 && apDev -> m_InpEP < 0 ) {
   apDev -> m_InpEPAddr  = apEP -> bEndpointAddress;
   apDev -> m_InpEP      = UsbOpenEndpointAligned ( apDev -> m_DevID, apEP );
   apDev -> m_InpPktSize = apEP -> wMaxPacketSizeHB * 256 + apEP -> wMaxPacketSizeLB;
  }  /* end if */
 }  /* end if */

}  /* end UmsBulkProbeEndPoint */

void UmsBulkRelease ( USBMDevice* apDev ) {

 if ( apDev -> m_InpEP >= 0 ) UsbCloseEndpoint ( apDev -> m_InpEP );
 if ( apDev -> m_OutEP >= 0 ) UsbCloseEndpoint ( apDev -> m_OutEP );

}  /* end UmsBulkRelease */

static void _usbm_callback_transfer ( int aStatus, int anBytes, void* apArg ) {

 (  ( USBMDevice* )apArg  ) -> m_nBytes += anBytes;

}  /* end _usbm_callback_transfer */

static void _usbm_callback_csw ( int aStatus, int anBytes, void* apArg ) {

 (  ( USBMDevice* )apArg  ) -> m_Status = aStatus;

 SignalSema (   (  ( USBMDevice* )apArg  ) -> m_Sync   );

}  /* end _usbm_callback_csw */

static void _usbm_reset ( USBMDevice* apDev, int aMode ) {

 int lSync = apDev -> m_Sync;

 if (  UsbControlTransfer(
        apDev -> m_CtlEP, 0x21, 0xFF, 0, apDev -> m_IntNum,
        0, NULL,
		_usbm_callback_csw, apDev
       ) != USB_RC_OK
 ) return;

 WaitSema ( lSync );

 if (  ( aMode & 1 ) && UsbClearEndpointFeature (
                         apDev -> m_CtlEP, 0, apDev -> m_InpEP,
                         _usbm_callback_csw, apDev
                        ) == USB_RC_OK
 ) WaitSema ( lSync );

 if (  ( aMode & 2 ) && UsbClearEndpointFeature (
                         apDev -> m_CtlEP, 0, apDev -> m_OutEP,
                         _usbm_callback_csw, apDev
                        ) == USB_RC_OK
 ) WaitSema ( lSync );

}  /* end _usbm_reset */

static int UsbmTransact ( USBMDevice* apDev, int aPipe, char* apData, int aSize ) {

 int     retVal = -1;
 int     lSync  = apDev -> m_Sync;
 CSWPack lCSW;

 lCSW.m_Sign = 0x53425355;

 apDev -> m_CBW.m_Sign    = 0x43425355;
 apDev -> m_CBW.m_Tag     = apDev -> m_Tag++;
 apDev -> m_CBW.m_DataLen = aSize;

 if (  UsbBulkTransfer ( apDev -> m_OutEP, &apDev -> m_CBW, 31, NULL, 0 ) == USB_RC_OK  ) {

  if ( apData ) {

   int lBlockSize = aSize;

   if ( aSize > 4096 ) lBlockSize = 4096;

   apDev -> m_nBytes = 0;

   while ( aSize ) {

    if ( aSize < lBlockSize ) lBlockSize = aSize;

    if (  UsbBulkTransfer (
           aPipe, apData, lBlockSize,
           _usbm_callback_transfer, apDev
          ) != USB_RC_OK
    ) break;

    apData += lBlockSize;
    aSize  -= lBlockSize;

   }  /* end while */

  }  /* end if */

  if (  UsbBulkTransfer (
         apDev -> m_InpEP, &lCSW, 13, _usbm_callback_csw, apDev
        ) == USB_RC_OK
  ) {

   WaitSema ( lSync );
   retVal = apDev -> m_Status;

  }  /* end if */

 }  /* end if */

 if ( retVal == USB_RC_STALL || retVal < 0 ) {

  if (  UsbClearEndpointFeature (
         apDev -> m_CtlEP, 0, apDev -> m_InpEP, _usbm_callback_csw, apDev
        ) == USB_RC_OK
  ) WaitSema ( lSync );

  if (  UsbBulkTransfer (
         apDev -> m_InpEP, &lCSW, 13, _usbm_callback_csw, apDev
        ) == USB_RC_OK
  ) {

   WaitSema ( lSync );
   retVal = apDev -> m_Status;

  }  /* end if */

  if ( lCSW.m_Sign != 0x53425355 || lCSW.m_Tag != apDev -> m_Tag ) _usbm_reset ( apDev, 3 );

 }  /* end if */

 return retVal;

}  /* end UsbmTransact */

int UmsWarmUp ( USBMDevice* apDev ) {

 int           i, lnLUN, retVal = 0;
 int           lIP       = apDev -> m_InpEP;
 int           lSync     = apDev -> m_Sync;
 unsigned char lBuf[ 40 ];

 if (  UsbSetDeviceConfiguration (
        apDev -> m_CtlEP, apDev -> m_CfgID, _usbm_callback_csw, apDev
       ) == USB_RC_OK
 ) {

  WaitSema ( lSync );

  if (  UsbSetInterface (
         apDev -> m_CtlEP, apDev -> m_IntNum, apDev -> m_IntAlt,
         _usbm_callback_csw, apDev
        ) == USB_RC_OK
  ) {

   WaitSema ( lSync );

   apDev -> m_MaxLUN = lnLUN = UmsGetMaxLUN ( apDev );

   for ( i = 0; i <= lnLUN; ++i ) {
// inquiry
    apDev -> m_CBW.m_Flags        = 0x80;
    apDev -> m_CBW.m_LUN          = i;
    apDev -> m_CBW.m_CmdLen       = 6;
    apDev -> m_CBW.m_CmdData[ 0 ] = 0x12;
    apDev -> m_CBW.m_CmdData[ 1 ] = 0;
    apDev -> m_CBW.m_CmdData[ 2 ] = 0;
    apDev -> m_CBW.m_CmdData[ 3 ] = 0;
    apDev -> m_CBW.m_CmdData[ 4 ] = 36;
    apDev -> m_CBW.m_CmdData[ 5 ] = 0;

    if (  !UsbmTransact ( apDev, lIP, lBuf, 36 )  ) {

     int lLen;

     mips_memset ( apDev -> m_LUName[ i ], 0, 19 );
     mips_memcpy ( apDev -> m_LUName[ i ], &lBuf[ 16 ], 16 );
     lLen = strlen ( apDev -> m_LUName[ i ] ) - 1;

     while ( lLen && apDev -> m_LUName[ i ][ lLen ] == ' ' ) {
      apDev -> m_LUName[ i ][ lLen ] = '\x00';
      lLen                          -= 1;
     }  /* end while */

     if ( i > 0 ) {
      lLen += 1;
      while ( 1 ) {
       int j = 0;
       for ( ; j < i; ++j )
        if (  !strcmp ( apDev -> m_LUName[ i ], apDev -> m_LUName[ j ] )  ) {
         apDev -> m_LUName[ i ][ lLen + 0 ] = (  i > 9 ?          1 : 0  ) + '0';
         apDev -> m_LUName[ i ][ lLen + 1 ] = (  i > 9 ? ( i - 10 ) : i  ) + '0';
         break;
        }  /* end if */
       if ( j == i ) break;
      }  /* end while */
     }  /* end if */

    }  /* end if */

   }  /* end for */

   retVal = 1;

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end UmsWarmUp */

void UmsCloseUnit ( USBMDevice* apDev ) {

 if ( apDev -> m_fEE ) {
  UmsNotifyEE ( 2, apDev -> m_UnitID );
  apDev -> m_fEE = 0;
 }  /* end if */

 if ( apDev -> m_pCacheBuf ) {
  FreeSysMemory ( apDev -> m_pCacheBuf );
  apDev -> m_pCacheBuf = NULL;
 }  /* end if */

}  /* end UmsCloseUnit */

int UmsGetMaxLUN ( USBMDevice* apDev ) {

 int retVal = 0;
 int lSync  = apDev -> m_Sync;

 if (  UsbControlTransfer(
        apDev -> m_CtlEP, 0xA1, 0xFE, 0, apDev -> m_IntNum,
        1, &retVal, _usbm_callback_csw, apDev
       ) != USB_RC_OK
 ) return 0;

 WaitSema ( lSync );

 return retVal;

}  /* end UmsGetMaxLUN */

int UmsGetCapacity ( USBMDevice* apDev ) {

 unsigned char lBuf[ 8 ];

 apDev -> m_CBW.m_CmdLen       = 10;
 apDev -> m_CBW.m_CmdData[ 0 ] = 0x25;
 apDev -> m_CBW.m_CmdData[ 1 ] = 0;
 (  ( _u32* )&apDev -> m_CBW.m_CmdData[ 2 ]  ) -> m_Val = 0;
 (  ( _u32* )&apDev -> m_CBW.m_CmdData[ 6 ]  ) -> m_Val = 0;

 if (  !UsbmTransact ( apDev, apDev -> m_InpEP, lBuf, 8 )  ) {

  apDev -> m_MaxLBA     = _int_be ( &lBuf[ 0 ] );
  apDev -> m_SectorSize = _int_be ( &lBuf[ 4 ] );

  return 1;

 }  /* end if */

 return 0;

}  /* end UmsGetCapacity */

int UmsRequestSense ( USBMDevice* apDev ) {

 unsigned char lBuf[ 18 ];
 SenseData*    lpSD = ( SenseData* )&lBuf[ 0 ];

 apDev -> m_CBW.m_Flags        = 0x80;
 apDev -> m_CBW.m_CmdLen       = 6;
 apDev -> m_CBW.m_CmdData[ 0 ] = 0x03;
 apDev -> m_CBW.m_CmdData[ 1 ] = 0;
 apDev -> m_CBW.m_CmdData[ 2 ] = 0;
 apDev -> m_CBW.m_CmdData[ 3 ] = 0;
 apDev -> m_CBW.m_CmdData[ 4 ] = 18;
 apDev -> m_CBW.m_CmdData[ 5 ] = 0;
#ifndef _DEBUG
 if (  !UsbmTransact ( apDev, apDev -> m_InpEP, lBuf, 18 ) && ( lpSD -> m_SenseKey < 2 || lpSD -> m_SenseKey == 5 || lpSD -> m_SenseKey == 6 )  ) return 1;
#else
 int lSts = UsbmTransact ( apDev, apDev -> m_InpEP, lBuf, 18 );
 char lLogBuf[ 64 ];
 sprintf ( lLogBuf, "Request sense: %d (sk: %d)\r\n", lSts, lpSD -> m_SenseKey );
 Log ( lLogBuf );
 if (  !lSts && ( lpSD -> m_SenseKey < 2 || lpSD -> m_SenseKey == 5 || lpSD -> m_SenseKey == 6 )  ) return 1;
#endif  /* _DEBUG */
 return 0;

}  /* end UmsRequestSense */

int UmsStart ( USBMDevice* apDev ) {

 apDev -> m_CBW.m_CmdLen       = 6;
 apDev -> m_CBW.m_CmdData[ 0 ] = 0x1B;
 apDev -> m_CBW.m_CmdData[ 1 ] = 1;
 apDev -> m_CBW.m_CmdData[ 2 ] = 0;
 apDev -> m_CBW.m_CmdData[ 3 ] = 0;
 apDev -> m_CBW.m_CmdData[ 4 ] = 1;
 apDev -> m_CBW.m_CmdData[ 5 ] = 0;
#ifndef _DEBUG
 if (  !UsbmTransact ( apDev, 0, NULL, 0 )  ) return !apDev -> m_Status;
#else
 char lLogBuf[ 64 ];
 int lSts = UsbmTransact ( apDev, 0, NULL, 0 );
 sprintf ( lLogBuf, "Start: %d (sts: %d)\r\n", lSts, apDev -> m_Status );
 Log ( lLogBuf );
 if ( !lSts ) return !apDev -> m_Status;
#endif  /* _DEBUG */
 return 0;

}  /* end UmsStart */

int UmsTestReady ( USBMDevice* apDev ) {

 apDev -> m_CBW.m_CmdLen       = 6;
 apDev -> m_CBW.m_CmdData[ 0 ] = 0x00;
 apDev -> m_CBW.m_CmdData[ 1 ] = 0;
 apDev -> m_CBW.m_CmdData[ 2 ] = 0;
 apDev -> m_CBW.m_CmdData[ 3 ] = 0;
 apDev -> m_CBW.m_CmdData[ 4 ] = 0;
 apDev -> m_CBW.m_CmdData[ 5 ] = 0;
#ifndef _DEBUG
 if (  !UsbmTransact ( apDev, 0, NULL, 0 )  ) return !apDev -> m_Status;
#else
 char lLogBuf[ 64 ];
 int lSts = UsbmTransact ( apDev, 0, NULL, 0 );
 sprintf ( lLogBuf, "Test ready: %d (sts: %d)\r\n", lSts, apDev -> m_Status );
 Log ( lLogBuf );
 if ( !lSts ) return !apDev -> m_Status;
#endif  /* _DEBUG */
 return 0;

}  /* end UmsTestReady */

int UmsRead ( USBMDevice* apDev, int aSector, void* apData, int aSize ) {

 int lnSectors = ( aSize + apDev -> m_SectorSize - 1 ) / apDev -> m_SectorSize;
 int retVal    = 0;

 if ( aSector + lnSectors > apDev -> m_MaxLBA ) {
  lnSectors = apDev -> m_MaxLBA - aSector + 1;
  aSize     = lnSectors * apDev -> m_SectorSize;
 }  /* end if */

 apDev -> m_CBW.m_Flags        = 0x80;
 apDev -> m_CBW.m_CmdLen       = 10;
 apDev -> m_CBW.m_CmdData[ 0 ] = 0x28;
 apDev -> m_CBW.m_CmdData[ 1 ] = 0;
 apDev -> m_CBW.m_CmdData[ 2 ] = ( aSector & 0xFF000000 ) >> 24;
 apDev -> m_CBW.m_CmdData[ 3 ] = ( aSector & 0x00FF0000 ) >> 16;
 apDev -> m_CBW.m_CmdData[ 4 ] = ( aSector & 0x0000FF00 ) >>  8;
 apDev -> m_CBW.m_CmdData[ 5 ] = ( aSector & 0x000000FF ) >>  0;
 apDev -> m_CBW.m_CmdData[ 6 ] = 0;
 apDev -> m_CBW.m_CmdData[ 7 ] = ( lnSectors & 0x0000FF00 ) >> 8;
 apDev -> m_CBW.m_CmdData[ 8 ] = ( lnSectors & 0x000000FF ) >> 0;
 apDev -> m_CBW.m_CmdData[ 9 ] = 0;

 if (  !UsbmTransact ( apDev, apDev -> m_InpEP, apData, aSize )  ) retVal = apDev -> m_nBytes;

 return retVal;

}  /* end UmsRead */
