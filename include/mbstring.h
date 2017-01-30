/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __mbstring_H
# define __mbstring_H

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

unsigned int _mbstrlen  ( const char*              );
char*        _mbstrspnp ( const char*, const char* );
char*        _mbstrpbrk ( const char*, const char* );
char*        _mbstrtok  ( char*, const char*       );

# ifdef __cplusplus
}
# endif  /* __cplusplus */

#endif  /* __mbstring_H */
