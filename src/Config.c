#include "Config.h"

#include <libmc.h>
#include <stdio.h>
#include <fileio.h>
#include <string.h>

SMSConfig g_Config;

int LoadConfig ( void ) {

 int retVal = 0;
 int lRes;

 mcGetInfo ( 0, 0, &lRes, &lRes, &lRes );
 mcSync ( 0, NULL, &lRes );

 if ( lRes > -2 ) {

  mcTable lDir __attribute__(   (  aligned( 64 )  )   );

  mcGetDir ( 0, 0, "/SMS", 0, 1, &lDir );
  mcSync ( 0, NULL, &lRes );

  if ( lRes ) {

   int lFD = fioOpen ( "mc0:SMS/SMS.cfg", O_RDONLY );

   if ( lFD >= 0 ) {

    if (  fioRead (  lFD, &g_Config, sizeof ( g_Config )  ) == sizeof ( g_Config )  ) retVal = 1;

    fioClose ( lFD );

   }  /* end if */

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end LoadCondig */

int SaveConfig ( void ) {

 int retVal = 0;

 int lRes;

 mcGetInfo ( 0, 0, &lRes, &lRes, &lRes );
 mcSync ( 0, NULL, &lRes );

 if ( lRes > -2 ) {

  mcTable lDir __attribute__(   (  aligned( 64 )  )   );

  mcGetDir ( 0, 0, "/SMS", 0, 1, &lDir );
  mcSync ( 0, NULL, &lRes );

  if (  lRes || !fioMkdir ( "mc0:SMS" )  ) {

   int lFD = fioOpen ( "mc0:SMS/icon.sys", O_RDONLY );

   if ( lFD < 0 ) {

    static iconIVECTOR lBgClr[ 4 ] SMS_DATA_SECTION = {
     {  68,  23, 116,  0 },  /* top left     */
     { 255, 255, 255,  0 },  /* top right    */
     { 255, 255, 255,  0 },  /* bottom left  */
     {  68,  23, 116,  0 }   /* bottom right */
	};
    static iconFVECTOR lLightDir[ 3 ] SMS_DATA_SECTION = {
     {  0.5F,  0.5F,  0.5F, 0.0F },
     {  0.0F, -0.4F, -0.1F, 0.0F },
     { -0.5F, -0.5F,  0.5F, 0.0F }
	};
	static iconFVECTOR lLightCol[ 3 ] SMS_DATA_SECTION = {
     { 0.3F, 0.3F, 0.3F, 0.0F },
     { 0.4F, 0.4F, 0.4F, 0.0F },
     { 0.5F, 0.5F, 0.5F, 0.0F }
	};
	static iconFVECTOR lAmb SMS_DATA_SECTION = { 0.5F, 0.5F, 0.5F, 0.0F };

    mcIcon lIcon; memset ( &lIcon, 0, sizeof ( mcIcon )  );

	strcpy ( lIcon.head, "PS2D" );
	strcpy_sjis (  ( short* )&lIcon.title, "SMS"  );

	lIcon.nlOffset =   16;
	lIcon.trans    = 0x60;

    memcpy ( lIcon.bgCol,        lBgClr,    sizeof ( lBgClr    )  );
    memcpy ( lIcon.lightDir,     lLightDir, sizeof ( lLightDir )  );
    memcpy ( lIcon.lightCol,     lLightCol, sizeof ( lLightCol )  );
    memcpy ( lIcon.lightAmbient, lAmb,      sizeof ( lAmb      )  );

    strcpy ( lIcon.view, "SMS.icn" );
    strcpy ( lIcon.copy, "SMS.icn" );
    strcpy ( lIcon.del,  "SMS.icn" );

 	lFD = fioOpen ( "mc0:SMS/icon.sys", O_WRONLY | O_CREAT );

	if ( lFD >= 0 ) {

     fioWrite (  lFD, &lIcon, sizeof ( lIcon )  );
     fioClose ( lFD );

     lFD = fioOpen ( "mc0:/SMS/SMS.icn", O_WRONLY | O_CREAT );

     if ( lFD >= 0 ) {

      fioWrite (  lFD, g_IconSMS, sizeof ( g_IconSMS )  );
      fioClose ( lFD );

     }  /* end if */

    }  /* end if */

   } else fioClose ( lFD );

   lFD = fioOpen ( "mc0:/SMS/SMS.cfg", O_WRONLY | O_CREAT );

   if ( lFD >= 0 ) {

    if (  fioWrite (  lFD, &g_Config, sizeof ( g_Config )  ) == sizeof ( g_Config )  ) retVal = 1;

    fioClose ( lFD );

   }  /* end if */

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end SaveConfig */
