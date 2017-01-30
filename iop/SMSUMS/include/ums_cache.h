#ifndef __ums_cache_H
# define __ums_cache_H

# ifndef __ums_H
#  include "ums.h"
# endif  /* __ums_H */

void  CacheInit ( USBMDevice*               );
void* CacheRead ( USBMDevice*, unsigned int );

#endif  /* __ums_cache_H */
