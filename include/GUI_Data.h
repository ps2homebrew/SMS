#ifndef __GUI_Data_H
# define __GUI_Data_H

extern unsigned char g_ImgFile   [ 4096 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_ImgFolder [ 4096 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_ImgPart   [ 4096 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_ImgUSB    [ 9216 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_ImgCDROM  [ 9216 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_ImgHDD    [ 9216 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_ImgCDDA   [ 9216 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_ImgHost   [ 9216 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_ImgDVD    [ 9216 ] __attribute__(   (  section( ".data" )  )   );
#endif  /* __GUI_Data_H */
