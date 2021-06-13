/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002
# by the XIPHOPHORUS Company http://www.xiph.org/
# Adopded for SMS in 2007 by Eugene Plotnikov
#
*/
#ifndef __SMS_OGG_H
#define __SMS_OGG_H

#ifndef __SMS_Codec_H
#include "SMS_Codec.h"
#endif  /* __SMS_Codec_H */

#include <string.h>

#define OV_EFAULT     -129
#define OV_EINVAL     -131
#define OV_ENOTVORBIS -132
#define OV_EBADHEADER -133
#define OV_EVERSION   -134
#define OV_ENOTAUDIO  -135
#define OV_EBADPACKET -136

typedef void vorbis_info_floor;
typedef void vorbis_info_mapping;
typedef void vorbis_info_residue;
typedef void vorbis_look_floor;
typedef void vorbis_look_residue;
typedef void vorbis_look_transform;

typedef struct oggpack_buffer {
 int            endbyte;
 int            endbit;
 unsigned char* buffer;
 unsigned char* ptr;
 int            storage;
} oggpack_buffer;

typedef struct ogg_sync_state {
 unsigned char* data;
 int            storage;
 int            fill;
 int            returned;
 int            unsynced;
 int            headerbytes;
 int            bodybytes;
} ogg_sync_state;

typedef struct ogg_stream_state {
 unsigned char* body_data;
 int            body_storage;
 int            body_fill;
 int            body_returned;
 int*           lacing_vals;
 long*          granule_vals;
 int            lacing_storage;
 int            lacing_fill;
 int            lacing_packet;
 int            lacing_returned;
 unsigned char  header[ 282 ];
 int            header_fill;
 int            e_o_s;
 int            b_o_s;
 int            serialno;
 int            pageno;
 long           packetno;
 int            granulepos;
} ogg_stream_state;

typedef struct ogg_page {
 unsigned char* header;
 int            header_len;
 unsigned char* body;
 int            body_len;
} ogg_page;

typedef struct ogg_packet {
 unsigned char* packet;
 int            bytes;
 int            b_o_s;
 int            e_o_s;
 long           granulepos;
 long           packetno;
} ogg_packet;

typedef struct vorbis_info {
 int   version;
 int   channels;
 int   rate;
 int   bitrate_upper;
 int   bitrate_nominal;
 int   bitrate_lower;
 int   bitrate_window;
 void* codec_setup;
} vorbis_info;

typedef struct vorbis_comment {
 char** user_comments;
 int*   comment_lengths;
 int    comments;
 char*  vendor;
} vorbis_comment;

typedef struct vorbis_dsp_state {
 int          analysisp;
 vorbis_info* vi;
 float**      pcm;
 float**      pcmret;
 int          pcm_storage;
 int          pcm_current;
 int          pcm_returned;
 int          preextrapolate;
 int          eofflag;
 int          lW;
 int          W;
 int          nW;
 int          centerW;
 long         granulepos;
 long         sequence;
 long         glue_bits;
 long         time_bits;
 long         floor_bits;
 long         res_bits;
 void*        backend_state;
} vorbis_dsp_state;

typedef struct vorbis_block {
 float**             pcm;
 oggpack_buffer      opb;
 int                 lW;
 int                 W;
 int                 nW;
 int                 pcmend;
 int                 mode;
 int                 eofflag;
 long                granulepos;
 long                sequence;
 vorbis_dsp_state*   vd;
 void*               localstore;
 int                 localtop;
 int                 localalloc;
 int                 totaluse;
 struct alloc_chain* reap;
 int                 glue_bits;
 int                 time_bits;
 int                 floor_bits;
 int                 res_bits;
 void*               internal;
} vorbis_block;

typedef struct vorbis_info_mode {
 int blockflag;
 int windowtype;
 int transformtype;
 int mapping;
} vorbis_info_mode;

typedef struct static_codebook {
 int  dim;
 int  entries;
 int* lengthlist;
 int  maptype;
 int  q_min;
 int  q_delta;
 int  q_quant;
 int  q_sequencep;
 int* quantlist;
 int  allocedp;
} static_codebook;

typedef struct codebook {
 int                    dim;
 int                    entries;
 int                    used_entries;
 const static_codebook* c;
 float*                 valuelist;
 unsigned int*          codelist;
 int*                   dec_index;
 char*                  dec_codelengths;
 unsigned int*          dec_firsttable;
 int                    dec_firsttablen;
 int                    dec_maxlength;
} codebook;

typedef struct codec_setup_info {
 int                  modes;
 int                  maps;
 int                  floors;
 int                  residues;
 int                  books;
 int                  blocksizes   [   2 ];
 vorbis_info_mode*    mode_param   [  64 ];
 int                  map_type     [  64 ];
 vorbis_info_mapping* map_param    [  64 ];
 int                  floor_type   [  64 ];
 vorbis_info_floor*   floor_param  [  64 ];
 int                  residue_type [  64 ];
 vorbis_info_residue* residue_param[  64 ];
 static_codebook*     book_param   [ 256 ];
 codebook*            fullbooks;
 int                  halfrate_flag;
} codec_setup_info;

typedef struct vorbis_func_mapping {
 vorbis_info_mapping* ( *unpack    ) ( vorbis_info*, oggpack_buffer*       );
 void                 ( *free_info ) ( vorbis_info_mapping*                );
 int                  ( *inverse   ) ( vorbis_block*, vorbis_info_mapping* );
} vorbis_func_mapping;

typedef struct vorbis_func_floor {
 vorbis_info_floor* ( *unpack    ) ( vorbis_info*, oggpack_buffer*                    );
 vorbis_look_floor* ( *look      ) ( vorbis_dsp_state *,vorbis_info_floor*            );
 void               ( *free_info ) ( vorbis_info_floor*                               );
 void               ( *free_look ) ( vorbis_look_floor*                               );
 void*              ( *inverse1  ) ( vorbis_block*, vorbis_look_floor*                );
 int                ( *inverse2  ) ( vorbis_block*, vorbis_look_floor*, void*, float* );
} vorbis_func_floor;

typedef struct vorbis_func_residue {
 vorbis_info_residue* ( *unpack    ) ( vorbis_info*, oggpack_buffer*                           );
 vorbis_look_residue* ( *look      ) ( vorbis_dsp_state*, vorbis_info_residue*                 );
 void                 ( *free_info ) ( vorbis_info_residue*                                    );
 void                 ( *free_look ) ( vorbis_look_residue*                                    );
 int                  ( *inverse   ) ( vorbis_block*, vorbis_look_residue*, float**, int*, int );
} vorbis_func_residue;

typedef struct mdct_lookup {
 int    n;
 int    log2n;
 float* trig;
 int*   bitrev;
 float  scale;
} mdct_lookup;

typedef struct alloc_chain {
 void*               ptr;
 struct alloc_chain* next;
} alloc_chain;


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

char* ogg_sync_buffer       ( ogg_sync_state*, int                );
int   ogg_sync_wrote        ( ogg_sync_state*, int                );
int   ogg_sync_pageout      ( ogg_sync_state*, ogg_page*          );
int   ogg_sync_pageseek     ( ogg_sync_state*, ogg_page*          );
void  ogg_page_checksum_set ( ogg_page*                           );
int   ogg_page_serialno     ( ogg_page*                           );
int   ogg_stream_init       ( ogg_stream_state*, int              );
int   ogg_stream_pagein     ( ogg_stream_state*, ogg_page*        );
int   _ogg_packetout        ( ogg_stream_state*, ogg_packet*, int );
void  ogg_stream_clear      ( ogg_stream_state*                   );
void  ogg_sync_clear        ( ogg_sync_state*                     );

static void inline ogg_sync_init ( ogg_sync_state* apSS ) {
 memset (  apSS, 0, sizeof ( *apSS )  );
}  /* end ogg_sync_init */

static int inline ogg_page_version ( ogg_page* apPage ) {
 return ( int )apPage -> header[ 4 ];
}  /* end ogg_page_version */

static int inline ogg_page_continued ( ogg_page* apPage ) {
 return ( int )apPage -> header[ 5 ] & 0x01;
}  /* end ogg_page_continued */

static int inline ogg_page_bos ( ogg_page* apPage ) {
 return ( int )apPage -> header[ 5 ] & 0x02;
}  /* end ogg_page_bos */

static int inline ogg_page_eos ( ogg_page* apPage ) {
 return ( int )apPage -> header[ 5 ] & 0x04;
}  /* end ogg_page_eos */

static int inline ogg_stream_packetout ( ogg_stream_state* apSS, ogg_packet* apPacket ) {
 return _ogg_packetout ( apSS, apPacket, 1 );
}  /* end ogg_stream_packetout */

void vorbis_info_init          ( vorbis_info*                               );
void vorbis_info_clear         ( vorbis_info*                               );
int  vorbis_synthesis_headerin ( vorbis_info*, vorbis_comment*, ogg_packet* );
void vorbis_comment_clear      ( vorbis_comment*                            );
int  vorbis_synthesis_init     ( vorbis_dsp_state*, vorbis_info*            );
void vorbis_block_init         ( vorbis_dsp_state*, vorbis_block*           );
int  vorbis_block_clear        ( vorbis_block*                              );
int  vorbis_synthesis          ( vorbis_block*, ogg_packet*                 );
int  vorbis_synthesis_blockin  ( vorbis_dsp_state*, vorbis_block*           );
int  vorbis_synthesis_pcmout   ( vorbis_dsp_state*, float***                );
int  vorbis_synthesis_read     ( vorbis_dsp_state*, int                     );
void vorbis_dsp_clear          ( vorbis_dsp_state*                          );

static void inline vorbis_comment_init ( vorbis_comment* apComent ) {
 memset (  apComent, 0, sizeof ( *apComent )  );
}  /* end vorbis_comment_init */

void SMS_Codec_OGGV_Open ( SMS_CodecContext* );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_OGG_H */
