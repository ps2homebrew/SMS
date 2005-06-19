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
#ifndef __SIF_H
# define __SIF_H

# include <tamtypes.h>
# include <sifrpc.h>
# include <sifcmd.h>

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

int SIF_BindRPC ( SifRpcClientData_t*, int );

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */
#endif  /* __SIF_H */
