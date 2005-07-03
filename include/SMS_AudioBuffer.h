/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_AudioBuffer_H
# define __SMS_AudioBuffer_H

typedef struct SMS_AudioBuffer {

 unsigned char* m_pBeg;
 unsigned char* m_pEnd;
 unsigned char* m_pInp;
 unsigned char* m_pOut;
 unsigned char* m_pPos;
 unsigned int   m_Len;
 unsigned int   m_fWait;

 unsigned char* ( *Alloc   ) ( int  );
 int            ( *Release ) ( void );
 void           ( *Destroy ) ( void );

} SMS_AudioBuffer;

# define SMS_ADV_ABINP( p, s ) ( p ) -> m_pInp += ( s + 71 ) & 0xFFFFFFC0
# define SMS_ADV_ABOUT( p, s ) ( p ) -> m_pOut += ( s + 71 ) & 0xFFFFFFC0

SMS_AudioBuffer* SMS_InitAudioBuffer ( void );

#endif  /* __SMS_AudioBuffer_H */
