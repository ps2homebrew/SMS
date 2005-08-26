/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2005 Damien Ciabrini (dciabrin), Olivier Parra (yo6)
#               2005 - Adopted for SMS by Eugene Plotnikov
# Licensed (like the original source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
*/
#ifndef __ROM_H
# define __ROM_H

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void* ROM_LocateModule ( const char*, int* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __ROM_H */
