/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 by Eugene Plotnikov <e-plotnikov@operamail.com>
#
# Licensed under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/
#ifndef __SMSAudrv_UPS_H
# define __SMSAudrv_UPS_H

typedef void ( *UPSFunc ) ( const void*, void* );

UPSFunc SMSAudrv_GetUPS ( int, int, int, int* );

#endif  /* __SMSAudrv_UPS_H */
