#include "PAD.h"
#include "SIF.h"

#include <kernel.h>
#include <string.h>

#define PAD_SERVER_1 0x8000010F
#define PAD_SERVER_2 0x8000011F

#define PAD_CMD_INIT         0x00000100
#define PAD_CMD_OPEN         0x80000100
#define PAD_CMD_INFO_ACT     0x80000102
#define PAD_CMD_INFO_COMB    0x80000103
#define PAD_CMD_INFO_MODE    0x80000104
#define PAD_CMD_SET_MMODE    0x80000105
#define PAD_CMD_SET_ACTDIR   0x80000106
#define PAD_CMD_SET_ACTALIGN 0x80000107
#define PAD_CMD_GET_BTNMASK  0x80000108
#define PAD_CMD_SET_BTNINFO  0x80000109
#define PAD_CMD_SET_VREF     0x8000010A
#define PAD_CMD_GET_PORTMAX  0x8000010B
#define PAD_CMD_GET_SLOTMAX  0x8000010C
#define PAD_CMD_CLOSE        0x8000010D
#define PAD_CMD_END          0x8000010E

typedef struct PAData {

 unsigned int  m_Frame;
 unsigned char m_State;
 unsigned char m_ReqState;
 unsigned char m_OK;
 unsigned char m_Unkn7;
 unsigned char m_Data[ 32 ];
 unsigned int  m_Length;
 unsigned int  m_Unkn44;
 unsigned int  m_Unkn48;
 unsigned int  m_Unkn52;
 unsigned int  m_Unkn54;
 unsigned int  m_Unkn60;

} PAData;

typedef struct PADState {

 int            m_Open;
 unsigned int   m_Port;
 unsigned int   m_Slot;
 PAData*        m_pData;
 unsigned char* m_pBuf;

} PADState;

static SifRpcClientData_t s_Client[   2 ] __attribute__(   (  aligned( 64 )  )   );
static char               s_Buffer[ 128 ] __attribute__(   (  aligned( 16 )  )   );

static PADState s_PadState[ 8 ][ 2 ];

static inline PAData* _PadmaStr ( int aPort, int aSlot ) {

 PAData* retVal = s_PadState[ aPort ][ aSlot ].m_pData;
    
 return retVal[ 0 ].m_Frame < retVal[ 1 ].m_Frame ? &retVal[ 1 ] : &retVal [ 0 ];

}  /* end _PadmaStr */

int PAD_Init ( void ) {

 int i, retVal = 0;

 if ( s_Client[ 0 ].server == NULL && s_Client[ 1 ].server == NULL ) {

  *( u32* )( &s_Buffer[ 0 ] ) = PAD_CMD_INIT;

  if (  SIF_BindRPC ( &s_Client[ 0 ], PAD_SERVER_1 ) &&
        SIF_BindRPC ( &s_Client[ 1 ], PAD_SERVER_2 ) &&
        SifCallRpc  (
         &s_Client[ 0 ], 1, 0, s_Buffer, 128, s_Buffer, 128, 0, 0
        ) >= 0
  ) retVal = 1;

  for ( i = 0; i < 8; ++i ) {

   s_PadState[ i ][ 0 ].m_Open = 0;
   s_PadState[ i ][ 0 ].m_Port = 0;
   s_PadState[ i ][ 0 ].m_Slot = 0;
   s_PadState[ i ][ 1 ].m_Open = 0;
   s_PadState[ i ][ 1 ].m_Port = 0;
   s_PadState[ i ][ 1 ].m_Slot = 0;

  }  /* end for */

 }  /* end if */

 return retVal;

}  /* end PAD_Init */

int PAD_Quit ( void ) {

 int retVal = 0;

 *( u32* )( &s_Buffer[ 0 ] ) = PAD_CMD_END;
    
 if (  SifCallRpc (
        &s_Client[ 0 ], 1, 0, s_Buffer, 128, s_Buffer, 128, 0, 0
       ) >= 0
 ) {

  retVal = *( int* )( & s_Buffer[ 12 ] );

  if ( retVal == 1 ) {

   s_Client[ 0 ].server = NULL;
   s_Client[ 1 ].server = NULL;

  }  /* end if */

 }  /* end if */
    
 return retVal;

}  /* end PAD_Quit */

int PAD_OpenPort ( int aPort, int aSlot, void* apData ) {

 int     i;
 PAData* lpDMA = ( PAData* )apData;
    
 if (  ( u32 )apData & 0x3F  ) return 0;
    
 for ( i = 0; i < 2; ++i ) {

  memset ( lpDMA[ i ].m_Data, 0xFF, 32 );
  lpDMA[ i ].m_Frame    = 0;
  lpDMA[ i ].m_Length   = 0;
  lpDMA[ i ].m_State    = PAD_STATE_EXECCMD;
  lpDMA[ i ].m_ReqState = PAD_RSTAT_BUSY;
  lpDMA[ i ].m_OK       = 0;
  lpDMA[ i ].m_Length   = 0;

 }  /* end for */
    
 *( u32* )( &s_Buffer[  0 ] ) = PAD_CMD_OPEN;
 *( u32* )( &s_Buffer[  4 ] ) = aPort;
 *( u32* )( &s_Buffer[  8 ] ) = aSlot;
 *( u32* )( &s_Buffer[ 16 ] ) = ( u32 )apData;
    
 if (  SifCallRpc (
        &s_Client[ 0 ], 1, 0, s_Buffer, 128, s_Buffer, 128, 0, 0
       ) >= 0
 ) {

  s_PadState[ aPort ][ aSlot ].m_Open  = 1;
  s_PadState[ aPort ][ aSlot ].m_pData = UNCACHED_SEG( apData );
  s_PadState[ aPort ][ aSlot ].m_pBuf  = *( char** )( &s_Buffer[ 20 ] );

  return *( u32* )( &s_Buffer[ 12 ] );

 }  /* end if */

 return 0;

}  /* end PAD_OpenPort */

int PAD_ClosePort ( int aPort, int aSlot ) {

 *( u32* )( &s_Buffer[  0 ] ) = PAD_CMD_CLOSE;
 *( u32* )( &s_Buffer[  4 ] ) = aPort;
 *( u32* )( &s_Buffer[  8 ] ) = aSlot;
 *( u32* )( &s_Buffer[ 16 ] ) = 1;
    
 if (   SifCallRpc(
         &s_Client[ 0 ], 1, 0, s_Buffer, 128, s_Buffer, 128, 0, 0
        ) >= 0
 ) {

  s_PadState[ aPort ][ aSlot ].m_Open = 0;

  return *( int* )( &s_Buffer[ 12 ]  );

 }  /* end if */

 return 0;
    
}  /* end PAD_ClosePort */

unsigned char PAD_ReqState ( int aPort, int aSlot ) {
    
 PAData* lpData = _PadmaStr ( aPort, aSlot );
            
 return lpData -> m_ReqState;

}  /* end PAD_ReqState */

int PAD_State ( int aPort, int aSlot ) {

 PAData*       lpData = _PadmaStr ( aPort, aSlot );
 unsigned char lState = lpData -> m_State;

 if ( lState                        == PAD_STATE_STABLE &&
      PAD_ReqState ( aPort, aSlot ) == PAD_RSTAT_BUSY
 ) return PAD_STATE_EXECCMD;

 return lState;

}  /* end PAD_State */

unsigned short PAD_Read ( int aPort, int aSlot ) {

 PAData* lpData = _PadmaStr ( aPort, aSlot );

 return lpData -> m_Length ? (  ( PadButtonStatus* )( lpData -> m_Data )  ) -> m_Btns ^ 0xFFFF : 0;

}  /* end PAD_Read */

void PAD_SetReqState ( int aPort, int aSlot, int aState ) {

 _PadmaStr ( aPort, aSlot ) -> m_ReqState = aState;

}  /* end PAD_SetReqState */

int PAD_SetMainMode ( int aPort, int aSlot, int aMode, int aLock ) {
    
 *( u32* )( &s_Buffer[  0 ] ) = PAD_CMD_SET_MMODE;
 *( u32* )( &s_Buffer[  4 ] ) = aPort;
 *( u32* )( &s_Buffer[  8 ] ) = aSlot;
 *( u32* )( &s_Buffer[ 12 ] ) = aMode;
 *( u32* )( &s_Buffer[ 16 ] ) = aLock;
        
 if (  SifCallRpc (
        &s_Client[ 0 ], 1, 0, s_Buffer, 128, s_Buffer, 128, 0, 0
       ) < 0
 ) return 0;

 if (  *( int* )( &s_Buffer[ 20 ] ) == 1  )

  PAD_SetReqState ( aPort, aSlot, PAD_RSTAT_BUSY );

 return *( int* )( &s_Buffer[ 20 ] );    

}  /* end PAD_SetMainMode */
