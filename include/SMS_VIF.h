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
#ifndef __SMS_VIF_H
# define __SMS_VIF_H

# define VIF1_STAT (  *( volatile unsigned int* )0x10003C00  )

# define VIF_CODE_NOP       0x00
# define VIF_CODE_NOPi      0x80
# define VIF_CODE_DIRECT    0x50
# define VIF_CODE_DIRECTi   0xD0
# define VIF_CODE_DIRECTHL  0x51
# define VIF_CODE_DIRECTHLi 0xD1

# define VIF_CODE( CMD, NUM, IMM ) (       \
 (   (  ( unsigned int )CMD  ) << 24   ) | \
 (   (  ( unsigned int )NUM  ) << 16   ) | \
 (   (  ( unsigned int )IMM  )         )   \
)

# define VIF_DIRECT( SIZE ) (                                          \
 (  ( unsigned long int )VIF_CODE( VIF_CODE_DIRECT, 0, SIZE )  ) << 32 \
)

# define VIF_DIRECTHL( SIZE ) (                                          \
 (  ( unsigned long int )VIF_CODE( VIF_CODE_DIRECTHL, 0, SIZE )  ) << 32 \
)

#endif  /* __SMS_VIF_H */
