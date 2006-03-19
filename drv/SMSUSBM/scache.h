#ifndef __SCACHE_H
# define __SCACHE_H

int  scache_init         ( int                  );
void scache_close        ( void                 );
int  scache_readSector   ( unsigned int, void** );

#endif  /* end __SCACHE_H */
