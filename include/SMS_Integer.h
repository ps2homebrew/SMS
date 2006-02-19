/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2004 Michael Niedermayer <michaelni@gmx.at>
#               2005 - Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_Integer_H
# define __SMS_Integer_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

# define SMS_INTEGER_SIZE 8

typedef struct SMS_Integer {

 uint16_t m_V[ SMS_INTEGER_SIZE ]; 

} SMS_Integer;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SMS_Integer SMS_Integer_mul_i  ( SMS_Integer, SMS_Integer               );
SMS_Integer SMS_Integer_int2i  ( int64_t                                );
SMS_Integer SMS_Integer_add_i  ( SMS_Integer, SMS_Integer               );
int64_t     SMS_Integer_i2int  ( SMS_Integer                            );
int32_t     SMS_Integer_log2_i ( SMS_Integer                            );
SMS_Integer SMS_Integer_shr_i  ( SMS_Integer, int32_t                   );
SMS_Integer SMS_Integer_div_i  ( SMS_Integer, SMS_Integer               );
SMS_Integer SMS_Integer_mod_i  ( SMS_Integer*, SMS_Integer, SMS_Integer );
int32_t     SMS_Integer_cmp_i  ( SMS_Integer, SMS_Integer               );
SMS_Integer SMS_Integer_sub_i  ( SMS_Integer, SMS_Integer               );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_Integer_H */
