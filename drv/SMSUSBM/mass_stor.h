#ifndef __MASS_STOR_H
# define __MASS_STOR_H

int mass_stor_init       ( void                         );
int mass_stor_disconnect ( int                          );
int mass_stor_connect    ( int                          );
int mass_stor_probe      ( int                          );
int mass_stor_readSector ( unsigned int, unsigned char* );

#endif  /* __MASS_STOR_H */
