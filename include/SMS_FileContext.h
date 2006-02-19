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
#ifndef __SMS_FileContext_H
# define __SMS_FileContext_H

# ifndef INLINE
#  ifdef _WIN32
#   define INLINE __inline
#  else  /* PS2 */
#   define INLINE inline
#  endif  /* WIN32 */
# endif  /* INLINE */

#define FILE_EOF( f ) (  ( f ) -> m_CurPos >= ( f ) -> m_Size  )

typedef enum STIOMode {
 STIOMode_Extended, STIOMode_Ordinary
} STIOMode;

typedef struct CDDAContext {

 unsigned int m_nTracks;
 unsigned int m_StartSector[ 100 ];
 unsigned int m_EndSector  [ 100 ];
 unsigned int m_Offset;
 char*        m_pName;
 char*        m_pDescription;
 void*        m_pData;

} CDDAContext;

typedef struct CDDADirectory {

 char*                 m_pName;
 unsigned int          m_Idx;
 unsigned int          m_ImgIdx;
 struct CDDADirectory* m_pNext;

} CDDADirectory;

typedef struct CDDAFile {

 char*            m_pName;
 unsigned int     m_ImgIdx;
 unsigned int     m_Size;
 unsigned int     m_Offset;
 struct CDDAFile* m_pNext;

} CDDAFile;

typedef struct FileContext {

 unsigned int   m_CurBuf;
 unsigned int   m_Size;
 unsigned int   m_Pos;
 unsigned int   m_CurPos;
 unsigned int   m_BufSize;
 unsigned char* m_pBase[ 2 ];
 unsigned char* m_pBuff[ 2 ];
 unsigned char* m_pPos;
 unsigned char* m_pEnd;
 void*          m_pData;
 unsigned int   m_StreamSize;
 void*          m_pOpenParam;
 char*          m_pPath;

 int  ( *Read    ) ( struct FileContext*, void*, unsigned int          );
 int  ( *Seek    ) ( struct FileContext*, unsigned int                 );
 int  ( *Fill    ) ( struct FileContext*                               );
 int  ( *Stream  ) ( struct FileContext*, unsigned int, unsigned int   );
 void ( *Destroy ) ( struct FileContext*                               );

 struct FileContext* ( *Open ) ( const char*, void* );

} FileContext;
# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

static INLINE int File_GetByte ( FileContext* apCtx ) {
 return apCtx -> m_pPos < apCtx -> m_pEnd || apCtx -> Fill ( apCtx ) > 0 ? ++apCtx -> m_CurPos, *apCtx -> m_pPos++ : 0;
}  /* end File_GetByte */

static INLINE unsigned int File_GetUShort ( FileContext* apCtx ) {
 unsigned int retVal = File_GetByte ( apCtx );
 return retVal | File_GetByte ( apCtx ) << 8;
}  /* end File_GetShort */

static INLINE unsigned int File_GetUInt ( FileContext* apCtx ) {
 unsigned int retVal = File_GetByte ( apCtx );
 retVal |= File_GetByte ( apCtx ) <<  8;
 retVal |= File_GetByte ( apCtx ) << 16;
 return retVal | File_GetByte ( apCtx ) << 24;
}  /* end File_GetInt */

void File_Skip      ( FileContext*, unsigned int        );
void File_GetString ( FileContext*, char*, unsigned int );

CDDAContext*         CDDA_InitContext     ( unsigned long                      );
void                 CDDA_DestroyContext  ( CDDAContext*                       );
const CDDADirectory* CDDA_DirectoryList   ( CDDAContext*                       );
CDDAFile*            CDDA_GetFileList     ( CDDAContext*, const CDDADirectory* );
void                 CDDA_DestroyFileList ( CDDAFile*                          );
int                  CDDA_GetPicture      ( CDDAContext*, int, void*           );
int                  CDDA_GetDiskPicture  ( CDDAContext*, void*                );

FileContext* CDDA_InitFileContext ( const char*, void* );
void         STIO_SetIOMode       ( STIOMode           );
FileContext* STIO_InitFileContext ( const char*, void* );
# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_FileContext_H */
