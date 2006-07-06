/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include <irx.h>

#include <dmacman.h>
#include <intrman.h>
#include <loadcore.h>
#include <thbase.h>
#include <thevent.h>

typedef struct SIO2_DMArg {

 void* m_pAddr;
 int   m_Size;
 int   m_Count;

} SIO2_DMArg;

typedef struct SIO2_TransferData {

 u32        m_Stat6c;           /*   0 */
 u32        m_PortCtrl1[ 4 ];   /*   4 */
 u32        m_PortCtrl2[ 4 ];   /*  20 */
 u32        m_Stat70;           /*  36 */
 u32        m_Regdata[ 16 ];    /*  40 */
 u32        m_Stat74;           /* 104 */
 u32        m_InSize;           /* 108 */
 u32        m_OutSize;          /* 112 */
 u8*        m_pIn;              /* 116 */
 u8*        m_pOut;             /* 120 */
 SIO2_DMArg m_DMAIn;            /* 124 */
 SIO2_DMArg m_DMAOut;           /* 136 */

} SIO2_TransferData;

extern struct irx_export_table _exp_sio2man;

static int                s_EventFlag;
static SIO2_TransferData* s_pTransferData;

static int  ( *s_Callback0 ) ( u32* );
static int  ( *s_Callback1 ) ( int  );
static int  ( *s_Callback2 ) ( int  );
static void ( *s_Callback3 ) ( void );

extern u32  SIO2_GetCtrl       ( void     );
extern void SIO2_SetCtrl       ( u32      );
extern u32  SIO2_GetStat6C     ( void     );
extern void SIO2_SetPortNCtrl1 ( int, u32 );
extern u32  SIO2_GetPortNCtrl1 ( int      );
extern void SIO2_SetPortNCtrl2 ( int, u32 );
extern u32  SIO2_GetPortNCtrl2 ( int      );
extern u32  SIO2_GetStat70     ( void     );
extern void SIO2_SetReg        ( int, u32 );
extern u32  SIO2_GetReg        ( int      );
extern u32  SIO2_GetStat74     ( void     );
extern void SIO2_SetUnk78      ( u32      );
extern u32  SIO2_GetUnk78      ( void     );
extern void SIO2_SetUnk7C      ( u32      );
extern u32  SIO2_GetUnk7C      ( void     );
extern void SIO2_DataOut       ( u8       );
extern u8   SIO2_DataIn        ( void     );
extern void SIO2_SetStat       ( u32      );
extern u32  SIO2_GetStat       ( void     );
extern void SIO2_SetCtrl3BC    ( void     );
extern void SIO2_SwitchCtrlC   ( void     );
extern void SIO2_SwitchCtrl1   ( void     );

static void _Send ( SIO2_TransferData* apData ) {

 int i;

 for ( i = 0; i < 4; ++i ) {

  SIO2_SetPortNCtrl1 ( i, apData -> m_PortCtrl1[ i ] );
  SIO2_SetPortNCtrl2 ( i, apData -> m_PortCtrl2[ i ] );

 }  /* end for */

 for ( i = 0; i < 16; ++i ) SIO2_SetReg ( i, apData -> m_Regdata[ i ] );

 for ( i = 0; i < apData -> m_InSize; ++i ) SIO2_DataOut ( apData -> m_pIn[ i ] );

 if ( apData -> m_DMAIn.m_pAddr ) {

  dmac_request (
   IOP_DMAC_SIO2in, apData -> m_DMAIn.m_pAddr, apData -> m_DMAIn.m_Size,
   apData -> m_DMAIn.m_Count, DMAC_FROM_MEM
  );
  dmac_transfer ( IOP_DMAC_SIO2in );

 }  /* end if */

 if ( apData -> m_DMAOut.m_pAddr ) {

  dmac_request (
   IOP_DMAC_SIO2out, apData -> m_DMAOut.m_pAddr, apData -> m_DMAOut.m_Size,
   apData -> m_DMAOut.m_Count, DMAC_TO_MEM
  );
  dmac_transfer ( IOP_DMAC_SIO2out );

 }  /* end if */

}  /* end _Send */

static void _Recv ( SIO2_TransferData* apData ) {

 int i;

 apData -> m_Stat6c = SIO2_GetStat6C ();
 apData -> m_Stat70 = SIO2_GetStat70 ();
 apData -> m_Stat74 = SIO2_GetStat74 ();

 for ( i = 0; i < apData -> m_OutSize; ++i ) apData -> m_pOut[ i ] = SIO2_DataIn ();

}  /* end _Recv */

static void _MainThread ( void* apParam ) {

 u32 lBits[ 4 ];
 u32 lEventFlag;

 while ( 1 ) {

  WaitEventFlag ( s_EventFlag, 0x00000155, 1, lBits );

  if ( lBits[ 0 ] & 0x00000001 ) {         /* SIO2_TransferInit0 */

   ClearEventFlag ( s_EventFlag, ~0x00000001 );
   lEventFlag = 0x00000002;

  } else if ( lBits[ 0 ] & 0x00000004 ) {  /* SIO2_TransferInit1 */

   ClearEventFlag ( s_EventFlag, ~0x00000004 );
   lEventFlag = 0x00000008;

  } else if ( lBits[ 0 ] & 0x00000010 ) {  /* SIO2_TransferInit2 */

   ClearEventFlag ( s_EventFlag, ~0x00000010 );
   lEventFlag = 0x00000020;

  } else if ( lBits[ 0 ] & 0x00000040 ) {  /* SIO2_TransferInit3 */

   ClearEventFlag ( s_EventFlag, ~0x00000040 );
   lEventFlag = 0x00000080;

  } else if ( lBits[ 0 ] & 0x00000100 ) {  /* SIO2_TransferInit4 */

   ClearEventFlag ( s_EventFlag, ~0x00000100 );
   lEventFlag = 0x00000200;

  } else break;
loop:
  SetEventFlag ( s_EventFlag, lEventFlag );

  WaitEventFlag ( s_EventFlag, 0x00001400, 1, lBits );  /* SIO2_Transfer or SIO2_TransferReset */

  if ( lBits[ 0 ] & 0x00001000 ) {  /* SIO2_TransferReset */

   ClearEventFlag ( s_EventFlag, ~0x00001000 );
   continue;

  }  /* end if */

  ClearEventFlag ( s_EventFlag, ~0x00000400 );  /* SIO2_Transfer */

  SIO2_SwitchCtrlC ();
  _Send ( s_pTransferData );
  SIO2_SwitchCtrl1 ();

  WaitEventFlag ( s_EventFlag, 0x00002000, 0, NULL );
  ClearEventFlag ( s_EventFlag, ~0x00002000 );

  _Recv ( s_pTransferData );
  lEventFlag = 0x00000800;

  goto loop;

 }  /* end while */

}  /* end _MainThread */

static int _IntHandler ( void* apArg ) {

 SIO2_SetStat (  SIO2_GetStat ()  );

 iSetEventFlag (  *( int* )apArg, 0x00002000  );

 return 1;

}  /* end _IntHandler */

void _dummy ( void ) {

}  /* end _dummy */

int _stop ( void ) {

 int lState;

 CpuSuspendIntr ( &lState );
 DisableIntr ( IOP_IRQ_SIO2, 0 );
 ReleaseIntrHandler ( IOP_IRQ_SIO2 );
 CpuResumeIntr ( lState );
 dmac_disable ( IOP_DMAC_SIO2in  );
 dmac_disable ( IOP_DMAC_SIO2out );

 return GetThreadId ();

}  /* end _stop */

int _start ( int argc, const char** argv ) {

 int retVal = MODULE_NO_RESIDENT_END;

 if (  RegisterLibraryEntries ( &_exp_sio2man ) == 0  ) {

  int          lState;
  int          lThreadID;
  iop_thread_t lThread;
  iop_event_t  lEvent;

  lEvent.attr = 2;
  lEvent.bits = 0;
  s_EventFlag = CreateEventFlag ( &lEvent );

  lThread.attr      = TH_C;
  lThread.thread    = _MainThread;
  lThread.stacksize = 0x2000;
  lThread.priority  = 24;
  lThreadID         = CreateThread ( &lThread );

  s_Callback0 = NULL;
  s_Callback1 = NULL;
  s_Callback2 = NULL;
  s_Callback3 = NULL;

  SIO2_SetCtrl3BC ();

  CpuSuspendIntr ( &lState );
  RegisterIntrHandler ( IOP_IRQ_SIO2, 1, _IntHandler, &s_EventFlag );
  EnableIntr ( IOP_IRQ_SIO2 );
  CpuResumeIntr ( lState );

  dmac_ch_set_dpcr ( IOP_DMAC_SIO2in,  3 );
  dmac_ch_set_dpcr ( IOP_DMAC_SIO2out, 3 );

  dmac_enable ( IOP_DMAC_SIO2in  );
  dmac_enable ( IOP_DMAC_SIO2out );

  StartThread ( lThreadID, NULL );

  retVal = MODULE_RESIDENT_END;

 }  /* end if */

 return retVal;

}  /* end _start */

void SIO2_TransferInit0 ( void ) {

 SetEventFlag ( s_EventFlag, 0x00000001 );
 WaitEventFlag ( s_EventFlag, 0x00000002, 0, NULL );
 ClearEventFlag ( s_EventFlag, ~0x00000002 );

}  /* end SIO2_TransferInit0 */

void SIO2_TransferInit1 ( void ) {

 SetEventFlag ( s_EventFlag, 0x00000004 );
 WaitEventFlag ( s_EventFlag, 0x00000008, 0, NULL );
 ClearEventFlag ( s_EventFlag, ~0x00000008 );

}  /* end SIO2_TransferInit1 */

void SIO2_TransferInit2 ( void ) {

 SetEventFlag ( s_EventFlag, 0x00000010 );
 WaitEventFlag ( s_EventFlag, 0x00000020, 0, NULL );
 ClearEventFlag ( s_EventFlag, ~0x00000020 );

}  /* end SIO2_TransferInit2 */

void SIO2_TransferInit3 ( void ) {

 SetEventFlag ( s_EventFlag, 0x00000040 );
 WaitEventFlag ( s_EventFlag, 0x00000080, 0, NULL );
 ClearEventFlag ( s_EventFlag, ~0x00000080 );

}  /* end SIO2_TransferInit3 */

void SIO2_TransferInit4 ( void ) {

 SetEventFlag ( s_EventFlag, 0x00000100 );
 WaitEventFlag ( s_EventFlag, 0x00000200, 0, NULL );
 ClearEventFlag ( s_EventFlag, ~0x00000200 );

}  /* end SIO2_TransferInit4 */

int SIO2_Transfer ( SIO2_TransferData* apData ) {

 s_pTransferData = apData;

 SetEventFlag ( s_EventFlag, 0x00000400 );
 WaitEventFlag ( s_EventFlag, 0x00000800, 0, NULL );
 ClearEventFlag ( s_EventFlag, ~0x00000800 );

 return 1;

}  /* end SIO2_Transfer */

void SIO2_TransferReset ( void ) {

 SetEventFlag ( s_EventFlag, 0x00001000 );

}  /* end SIO2_TransferReset */

void SIO2_SetCallback0 (  int ( *apCallback ) ( u32* )  ) {

 s_Callback0 = apCallback;

}  /* end SIO2_SetCallback0 */

void SIO2_SetCallback1 (  int ( *apCallback ) ( int )  ) {

 s_Callback1 = apCallback;

}  /* end SIO2_SetCallback1 */

void SIO2_SetCallback2 (  int ( *apCallback ) ( int )  ) {

 s_Callback2 = apCallback;

}  /* end SIO2_SetCallback2 */

void SIO2_SetCallback3 (  void ( *apCallback ) ( void )  ) {

 s_Callback3 = apCallback;

}  /* end SIO2_SetCallback3 */

int SIO2_Callback0 ( u32* apArg ) {

 int i, retVal = 1;

 if ( s_Callback0 ) return s_Callback0 ( apArg );

 for ( i = 0; i < 4; ++i, ++apArg ) {

  if (  ( *apArg + 1 ) < 2  )

   apArg[ 4 ] = 1;

  else {

   apArg[ 4 ] = 0;
   retVal     = 0;

  }  /* end else */

 }  /* end for */

 return retVal;

}  /* end SIO2_Callback0 */

int SIO2_Callback1 ( int anArg ) {

 return s_Callback1 ? s_Callback1 ( anArg ) : 1;

}  /* end SIO2_Callback1 */

int SIO2_Callback2 ( int anArg ) {

 return s_Callback2 ? s_Callback2 ( anArg ) : 1;

}  /* end SIO2_Callback2 */

void SIO2_Callback3 ( void ) {

 if ( s_Callback3 ) s_Callback3 ();

}  /* end SIO2_Callback3 */

int SIO2_TransferLegacy ( SIO2_TransferData* apData ) {

 SIO2_Transfer ( apData );
 SIO2_TransferReset ();

 return 1;

}  /* end SIO2_TransferLegacy */

void SIO2_TransferInit0W ( void ) {

}  /* end SIO2_TransferInit0W */

void SIO2_TransferInit1W ( void ) {

}  /* end SIO2_TransferInit1W */

void SIO2_TransferResetW ( void ) {

}  /* end SIO2_TransferResetW */
