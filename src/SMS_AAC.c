/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
** FAAD - Freeware Advanced Audio Decoder
** Copyright (C) 2002 M. Bakker
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** $Id: decode.c,v 1.16 2004/04/03 19:08:37 menno Exp $
** $Id: decode.c,v 1.16 2004/04/03 19:08:37 menno Exp $
** Adopted for SMS and optimized for R5900 CPU by Eugene Plotnikov
**/
#include "SMS_AAC.h"
#include "SMS_DSP.h"
#include "SMS_Bitio.h"
#include "SMS_RingBuffer.h"
#include "SMS_Codec.h"
#include "SMS_Locale.h"

#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

extern float PowF     ( float, float         );
extern void  pcm_syn1 ( short*, float**, int );
extern void  pcm_syn2 ( short*, float**, int );
extern void  pcm_synN ( short*, float**, int );

#define sqrtf SMS_sqrtf

#define LEN_SE_ID 3
#define ID_SCE    0x0
#define ID_CPE    0x1
#define ID_CCE    0x2
#define ID_LFE    0x3
#define ID_DSE    0x4
#define ID_PCE    0x5
#define ID_FIL    0x6
#define ID_END    0x7

#define LEN_TAG 4

#define ANC_DATA           0
#define EXT_FIL            0
#define EXT_FILL_DATA      1
#define EXT_DATA_ELEMENT   2
#define EXT_DYNAMIC_RANGE 11
#define EXT_SBR_DATA      13
#define EXT_SBR_DATA_CRC  14

#define INVALID_SBR_ELEMENT 255

#define BYTE_NUMBIT_LD 3
#define bit2byte( a ) (  ( a +7 ) >> BYTE_NUMBIT_LD  )
#define bit_set( A, B ) (   ( A ) & (  1 << ( B )  )   )

#define min( a, b ) (   (  ( a ) < ( b )  ) ? ( a ) : ( b )   )
#define max( a, b ) (   (  ( a ) > ( b )  ) ? ( a ) : ( b )   )

#define FRONT_CHANNEL_CENTER 1
#define FRONT_CHANNEL_LEFT   2
#define FRONT_CHANNEL_RIGHT  3
#define SIDE_CHANNEL_LEFT    4
#define SIDE_CHANNEL_RIGHT   5
#define BACK_CHANNEL_LEFT    6
#define BACK_CHANNEL_RIGHT   7
#define BACK_CHANNEL_CENTER  8
#define LFE_CHANNEL          9

#define MAX_WINDOW_GROUPS 8
#define MAX_SFB          51

#define ER_OBJECT_START      17
#define ONLY_LONG_SEQUENCE   0x0
#define LONG_START_SEQUENCE  0x1
#define EIGHT_SHORT_SEQUENCE 0x2
#define LONG_STOP_SEQUENCE   0x3

#define PAIR_LEN        2
#define QUAD_LEN        4
#define ZERO_HCB        0
#define FIRST_PAIR_HCB  5
#define NOISE_HCB      13
#define INTENSITY_HCB  15
#define INTENSITY_HCB2 14

#define ALPHA 0.90625F
#define A     0.953125F

#define TNS_MAX_ORDER 20
#define DRC_REF_LEVEL 80

#define IQ_TABLE_SIZE 8192

#define MAX_CHANNELS        64
#define MAX_SYNTAX_ELEMENTS 48

#define FAAD_FMT_16BIT 1

#define FAAD_NO_SBR 0
#define FAAD_RAW    0
#define FAAD_ADIF   1
#define FAAD_ADTS   2

#define FAAD_MAIN 1
#define FAAD_LC   2
#define FAAD_SSR  3

#define MAX_M       49
#define MAX_L_E      5
#define MAX_NTSRHFG 40

#define T_HFGEN 8
#define T_HFADJ 2

#define RATE 2

#define NO_TIME_SLOTS_960 15
#define NO_TIME_SLOTS     16

#define LO_RES 0
#define HI_RES 1

#define FIXFIX 0
#define FIXVAR 1
#define VARFIX 2
#define VARVAR 3

#define EXT_SBR_DATA_CRC 14

#define EPS 1E-12F

#define MAX_NTSR 32

typedef float     complex_t[ 2 ];
typedef complex_t qmf_t;

typedef struct qmfa_info {
 float*  x;
 int16_t x_index;
 uint8_t channels;
} qmfa_info;

typedef struct qmfs_info {
 float*  v;
 int16_t v_index;
 uint8_t channels;
} qmfs_info;

typedef struct sbr_hfadj_info {
 float G_lim_boost  [ MAX_L_E ][ MAX_M ];
 float Q_M_lim_boost[ MAX_L_E ][ MAX_M ];
 float S_M_boost    [ MAX_L_E ][ MAX_M ];
} sbr_hfadj_info;

typedef struct sbr_info {
 uint32_t       sample_rate;
 uint32_t       maxAACLine;
 uint8_t        rate;
 uint8_t        just_seeked;
 uint8_t        ret;
 uint8_t        amp_res[ 2 ];
 uint8_t        k0;
 uint8_t        kx;
 uint8_t        M;
 uint8_t        N_master;
 uint8_t        N_high;
 uint8_t        N_low;
 uint8_t        N_Q;
 uint8_t        N_L[ 4 ];
 uint8_t        n[ 2 ];
 uint8_t        f_master[ 64 ];
 uint8_t        f_table_res[ 2 ][ 64 ];
 uint8_t        f_table_noise[ 64 ];
 uint8_t        f_table_lim[ 4 ][ 64 ];
 uint8_t        table_map_k_to_g[ 64 ];
 uint8_t        abs_bord_lead[ 2 ];
 uint8_t        abs_bord_trail[ 2 ];
 uint8_t        n_rel_lead[ 2 ];
 uint8_t        n_rel_trail[ 2 ];
 uint8_t        L_E[ 2 ];
 uint8_t        L_E_prev[ 2 ];
 uint8_t        L_Q[ 2 ];
 uint8_t        t_E[ 2 ][ MAX_L_E + 1 ];
 uint8_t        t_Q[ 2 ][ 3 ];
 uint8_t        f[ 2 ][ MAX_L_E + 1 ];
 uint8_t        f_prev[ 2 ];
 float*         G_temp_prev[ 2 ][ 5 ];
 float*         Q_temp_prev[ 2 ][ 5 ];
 int8_t         GQ_ringbuf_index[ 2 ];
 int16_t        E[ 2 ][ 64 ][ MAX_L_E ];
 int16_t        E_prev[ 2 ][ 64 ];
 float          E_orig[ 2 ][ 64 ][ MAX_L_E ];
 float          E_curr[ 2 ][ 64 ][ MAX_L_E ];
 int32_t        Q[ 2 ][ 64 ][ 2 ];
 float          Q_div[ 2 ][ 64 ][ 2 ];
 float          Q_div2[ 2 ][ 64 ][ 2 ];
 int32_t        Q_prev[ 2 ][ 64 ];
 int8_t         l_A[ 2 ];
 int8_t         l_A_prev[ 2 ];
 uint8_t        bs_invf_mode[ 2 ][ MAX_L_E ];
 uint8_t        bs_invf_mode_prev[ 2 ][ MAX_L_E ];
 float          bwArray[ 2 ][ 64 ];
 float          bwArray_prev[ 2 ][ 64 ];
 uint8_t        noPatches;
 uint8_t        patchNoSubbands[ 64 ];
 uint8_t        patchStartSubband[ 64 ];
 uint8_t        bs_add_harmonic[ 2 ][ 64 ];
 uint8_t        bs_add_harmonic_prev[ 2 ][ 64 ];
 uint16_t       index_noise_prev[ 2 ];
 uint8_t        psi_is_prev[ 2 ];
 uint8_t        bs_start_freq_prev;
 uint8_t        bs_stop_freq_prev;
 uint8_t        bs_xover_band_prev;
 uint8_t        bs_freq_scale_prev;
 uint8_t        bs_alter_scale_prev;
 uint8_t        bs_noise_bands_prev;
 int8_t         prevEnvIsShort[ 2 ];
 int8_t         kx_prev;
 uint8_t        bsco;
 uint8_t        bsco_prev;
 uint8_t        M_prev;
 uint16_t       frame_len;
 uint8_t        Reset;
 uint32_t       frame;
 uint32_t       header_count;
 uint8_t        id_aac;
 qmfa_info*     qmfa[ 2 ];
 qmfs_info*     qmfs[ 2 ];
 qmf_t          Xsbr[ 2 ][ MAX_NTSRHFG ][ 64 ];
 uint8_t        numTimeSlotsRate;
 uint8_t        numTimeSlots;
 uint8_t        tHFGen;
 uint8_t        tHFAdj;
 uint8_t        bs_header_flag;
 uint8_t        bs_crc_flag;
 uint16_t       bs_sbr_crc_bits;
 uint8_t        bs_protocol_version;
 uint8_t        bs_amp_res;
 uint8_t        bs_start_freq;
 uint8_t        bs_stop_freq;
 uint8_t        bs_xover_band;
 uint8_t        bs_freq_scale;
 uint8_t        bs_alter_scale;
 uint8_t        bs_noise_bands;
 uint8_t        bs_limiter_bands;
 uint8_t        bs_limiter_gains;
 uint8_t        bs_interpol_freq;
 uint8_t        bs_smoothing_mode;
 uint8_t        bs_samplerate_mode;
 uint8_t        bs_add_harmonic_flag[ 2 ];
 uint8_t        bs_add_harmonic_flag_prev[ 2 ];
 uint8_t        bs_extended_data;
 uint8_t        bs_extension_id;
 uint8_t        bs_extension_data;
 uint8_t        bs_coupling;
 uint8_t        bs_frame_class[ 2 ];
 uint8_t        bs_rel_bord[ 2 ][ 9 ];
 uint8_t        bs_rel_bord_0[ 2 ][ 9 ];
 uint8_t        bs_rel_bord_1[ 2 ][ 9 ];
 uint8_t        bs_pointer[ 2 ];
 uint8_t        bs_abs_bord_0[ 2 ];
 uint8_t        bs_abs_bord_1[ 2 ];
 uint8_t        bs_num_rel_0[ 2 ];
 uint8_t        bs_num_rel_1[ 2 ];
 uint8_t        bs_df_env[ 2 ][ 9 ];
 uint8_t        bs_df_noise[ 2 ][ 3 ];
 qmf_t          X[ MAX_NTSR ][ 64 ];
 sbr_hfadj_info adj;
} sbr_info;

typedef const int8_t ( *sbr_huff_tab )[ 2 ];

typedef struct acorr_coef {
 complex_t r01;
 complex_t r02;
 complex_t r11;
 complex_t r12;
 complex_t r22;
 float     det;
} acorr_coef;

typedef struct {
 uint16_t   n;
 uint16_t   ifac[ 15 ];
 complex_t* work;
 complex_t* tab;
} cfft_info;

typedef struct {
 uint16_t   N;
 cfft_info  cfft;
 complex_t* sincos;
} mdct_info;

typedef struct {
 const float*    long_window [ 2 ];
 const float*    short_window[ 2 ];
 SMS_MDCTContext mdct256;
 SMS_MDCTContext mdct2048;
} fb_info;

typedef struct {
 int16_t r  [ 2 ];
 int16_t COR[ 2 ];
 int16_t VAR[ 2 ];
} pred_state;

typedef struct {
 uint8_t present;
 uint8_t num_bands;
 uint8_t pce_instance_tag;
 uint8_t excluded_chns_present;
 uint8_t band_top   [ 17 ];
 uint8_t prog_ref_level;
 uint8_t dyn_rng_sgn[ 17 ];
 uint8_t dyn_rng_ctl[ 17 ];
 uint8_t exclude_mask            [ MAX_CHANNELS ];
 uint8_t additional_excluded_chns[ MAX_CHANNELS ];
 float   ctrl1;
 float   ctrl2;
} drc_info;

typedef struct {
 uint8_t element_instance_tag;
 uint8_t object_type;
 uint8_t sf_index;
 uint8_t num_front_channel_elements;
 uint8_t num_side_channel_elements;
 uint8_t num_back_channel_elements;
 uint8_t num_lfe_channel_elements;
 uint8_t num_assoc_data_elements;
 uint8_t num_valid_cc_elements;
 uint8_t mono_mixdown_present;
 uint8_t mono_mixdown_element_number;
 uint8_t stereo_mixdown_present;
 uint8_t stereo_mixdown_element_number;
 uint8_t matrix_mixdown_idx_present;
 uint8_t pseudo_surround_enable;
 uint8_t matrix_mixdown_idx;
 uint8_t front_element_is_cpe         [ 16 ];
 uint8_t front_element_tag_select     [ 16 ];
 uint8_t side_element_is_cpe          [ 16 ];
 uint8_t side_element_tag_select      [ 16 ];
 uint8_t back_element_is_cpe          [ 16 ];
 uint8_t back_element_tag_select      [ 16 ];
 uint8_t lfe_element_tag_select       [ 16 ];
 uint8_t assoc_data_element_tag_select[ 16 ];
 uint8_t cc_element_is_ind_sw         [ 16 ];
 uint8_t valid_cc_element_tag_select  [ 16 ];
 uint8_t channels;
 uint8_t comment_field_bytes;
 uint8_t comment_field_data[ 257 ];
 uint8_t num_front_channels;
 uint8_t num_side_channels;
 uint8_t num_back_channels;
 uint8_t num_lfe_channels;
 uint8_t sce_channel[ 16 ];
 uint8_t cpe_channel[ 16 ];
} program_config;

typedef struct {
 uint8_t number_pulse;
 uint8_t pulse_start_sfb;
 uint8_t pulse_offset[ 4 ];
 uint8_t pulse_amp   [ 4 ];
} pulse_info;

typedef struct {
 uint8_t n_filt       [ 8 ];
 uint8_t coef_res     [ 8 ];
 uint8_t length       [ 8 ][ 4 ];
 uint8_t order        [ 8 ][ 4 ];
 uint8_t direction    [ 8 ][ 4 ];
 uint8_t coef_compress[ 8 ][ 4 ];
 uint8_t coef         [ 8 ][ 4 ][ 32 ];
} tns_info;

typedef struct {
 uint8_t limit;
 uint8_t predictor_reset;
 uint8_t predictor_reset_group_number;
 uint8_t prediction_used[ MAX_SFB ];
} pred_info;

typedef struct {
 uint8_t    max_sfb;
 uint8_t    num_swb;
 uint8_t    num_window_groups;
 uint8_t    num_windows;
 uint8_t    window_sequence;
 uint8_t    window_group_length[ 8 ];
 uint8_t    window_shape;
 uint8_t    scale_factor_grouping;
 uint16_t   sect_sfb_offset[ 8 ][ 15 * 8 ];
 uint16_t   swb_offset[ 52 ];
 uint16_t   swb_offset_max;
 uint8_t    sect_cb[ 8 ][ 15 * 8 ];
 uint16_t   sect_start[ 8 ][ 15 * 8 ];
 uint16_t   sect_end[ 8 ][ 15 * 8 ];
 uint8_t    sfb_cb[ 8 ][ 8 * 15 ];
 uint8_t    num_sec[ 8 ];
 uint8_t    global_gain;
 int16_t    scale_factors[ 8 ][ 51 ];
 uint8_t    ms_mask_present;
 uint8_t    ms_used[ MAX_WINDOW_GROUPS ][ MAX_SFB ];
 uint8_t    noise_used;
 uint8_t    is_used;
 uint8_t    pulse_data_present;
 uint8_t    tns_data_present;
 uint8_t    gain_control_data_present;
 uint8_t    predictor_data_present;
 pulse_info pul;
 tns_info   tns;
 pred_info  pred;
} ic_stream;

typedef struct {
 uint8_t   channel;
 int16_t   paired_channel;
 uint8_t   element_instance_tag;
 uint8_t   common_window;
 ic_stream ics1;
 ic_stream ics2;
} element;

typedef struct _mp4AudioSpecificConfig {
 unsigned char  objectTypeIndex;
 unsigned char  samplingFrequencyIndex;
 unsigned long  samplingFrequency;
 unsigned char  channelsConfiguration;
 unsigned char  frameLengthFlag;
 unsigned char  dependsOnCoreCoder;
 unsigned short coreCoderDelay;
 unsigned char  extensionFlag;
 unsigned char  aacSectionDataResilienceFlag;
 unsigned char  aacScalefactorDataResilienceFlag;
 unsigned char  aacSpectralDataResilienceFlag;
 unsigned char  epConfig;
 char           sbr_present_flag;
 char           forceUpSampling;
 char           downSampledSBR;
} _mp4AudioSpecificConfig;

typedef struct _NeAACDecConfiguration {
 unsigned char defObjectType;
 unsigned long defSampleRate;
 unsigned char useOldADTSFormat;
 unsigned char dontUpSampleImplicitSBR;
} _NeAACDecConfiguration, *_NeAACDecConfigurationPtr;

typedef struct _NeAACDecFrameInfo {
 unsigned int  bytesconsumed;
 unsigned int  samples;
 unsigned char channels;
 unsigned char error;
 unsigned int  samplerate;
 unsigned char sbr;
 unsigned char object_type;
 unsigned char header_type;
 unsigned char num_front_channels;
 unsigned char num_side_channels;
 unsigned char num_back_channels;
 unsigned char num_lfe_channels;
 unsigned char channel_position[ MAX_CHANNELS ];
 unsigned char ps;
} _NeAACDecFrameInfo;

typedef struct {

 uint8_t                adts_header_present;
 uint8_t                adif_header_present;
 uint8_t                sf_index;
 uint8_t                object_type;
 uint8_t                channelConfiguration;
 uint8_t                postSeekResetFlag;
 uint32_t               frame;
 uint8_t                downMatrix;
 uint8_t                upMatrix;
 uint8_t                first_syn_ele;
 uint8_t                has_lfe;
 uint8_t                fr_channels;
 uint8_t                fr_ch_ele;
 uint8_t                element_output_channels[ MAX_SYNTAX_ELEMENTS ];
 uint8_t                element_alloced        [ MAX_SYNTAX_ELEMENTS ];
 uint8_t                window_shape_prev[ MAX_CHANNELS ];
 fb_info                fb;
 drc_info               drc;
 float*                 time_out   [ MAX_CHANNELS ];
 float*                 fb_intermed[ MAX_CHANNELS ];
 pred_state*            pred_stat  [ MAX_CHANNELS ];
 uint32_t               __r1;
 uint32_t               __r2;
 uint8_t                pce_set;
 program_config         pce;
 uint8_t                element_id      [ MAX_CHANNELS ];
 uint8_t                internal_channel[ MAX_CHANNELS ];
 _NeAACDecConfiguration config;
 const uint8_t*         cmes;
 element                m_Element;
 float                  transf_buf[ 2 * 1024 ] __attribute__(   (  aligned( 16 )  )   );
 int8_t                 sbr_present_flag;
 int8_t                 forceUpSampling;
 int8_t                 downSampledSBR;
 uint8_t                sbr_alloced[ MAX_SYNTAX_ELEMENTS ];
 sbr_info*              sbr[ MAX_SYNTAX_ELEMENTS ];

} _NeAACDecStruct, *_NeAACDecHandle;

typedef struct {
 uint8_t offset;
 uint8_t extra_bits;
} hcb;

typedef struct {
 uint8_t bits;
 int8_t  x;
 int8_t  y;
} hcb_2_pair;

typedef struct {
 uint8_t bits;
 int8_t  x;
 int8_t  y;
 int8_t  v;
 int8_t  w;
} hcb_2_quad;

typedef struct {
 uint8_t is_leaf;
 int8_t  data[ 2 ];
} hcb_bin_pair;

typedef struct {
 uint8_t is_leaf;
 int8_t  data[ 4 ];
} hcb_bin_quad;

typedef struct {
 uint8_t        copyright_id_present;
 int8_t         copyright_id[ 10 ];
 uint8_t        original_copy;
 uint8_t        home;
 uint8_t        bitstream_type;
 uint32_t       bitrate;
 uint8_t        num_program_config_elements;
 uint32_t       adif_buffer_fullness;
 program_config pce[ 16 ];
} adif_header;

typedef struct {
 uint16_t syncword;
 uint8_t  id;
 uint8_t  layer;
 uint8_t  protection_absent;
 uint8_t  profile;
 uint8_t  sf_index;
 uint8_t  private_bit;
 uint8_t  channel_configuration;
 uint8_t  original;
 uint8_t  home;
 uint8_t  emphasis;
 uint8_t  copyright_identification_bit;
 uint8_t  copyright_identification_start;
 uint16_t aac_frame_length;
 uint16_t adts_buffer_fullness;
 uint8_t  no_raw_data_blocks_in_frame;
 uint16_t crc_check;
 uint8_t  old_format;
} adts_header;

static const unsigned int sine_long_1024[ 1024 ] __attribute__(   (  section( ".rodata" ), aligned( 16 )  )   ) = {
 0x3A490FD9, 0x3B16CBDB, 0x3B7B53A9, 0x3BAFEDA8, 0x3BE23160, 0x3C0A3A7B, 0x3C235C30, 0x3C3C7DCC,
 0x3C559F4C, 0x3C6EC0AA, 0x3C83F0F2, 0x3C90817A, 0x3C9D11EC, 0x3CA9A246, 0x3CB63286, 0x3CC2C2A9,
 0x3CCF52AF, 0x3CDBE294, 0x3CE87258, 0x3CF501F8, 0x3D00C8B9, 0x3D071062, 0x3D0D57F6, 0x3D139F75,
 0x3D19E6DD, 0x3D202E2D, 0x3D267564, 0x3D2CBC82, 0x3D330385, 0x3D394A6C, 0x3D3F9137, 0x3D45D7E4,
 0x3D4C1E73, 0x3D5264E2, 0x3D58AB31, 0x3D5EF15F, 0x3D65376A, 0x3D6B7D51, 0x3D71C315, 0x3D7808B3,
 0x3D7E4E2B, 0x3D8249BE, 0x3D856C52, 0x3D888ED2, 0x3D8BB13C, 0x3D8ED391, 0x3D91F5D0, 0x3D9517F9,
 0x3D983A0A, 0x3D9B5C05, 0x3D9E7DE7, 0x3DA19FB0, 0x3DA4C161, 0x3DA7E2F8, 0x3DAB0476, 0x3DAE25D9,
 0x3DB14721, 0x3DB4684E, 0x3DB7895F, 0x3DBAAA54, 0x3DBDCB2C, 0x3DC0EBE6, 0x3DC40C83, 0x3DC72D02,
 0x3DCA4D62, 0x3DCD6DA3, 0x3DD08DC4, 0x3DD3ADC5, 0x3DD6CDA5, 0x3DD9ED64, 0x3DDD0D02, 0x3DE02C7D,
 0x3DE34BD6, 0x3DE66B0C, 0x3DE98A1F, 0x3DECA90D, 0x3DEFC7D7, 0x3DF2E67C, 0x3DF604FB, 0x3DF92354,
 0x3DFC4187, 0x3DFF5F94, 0x3E013EBC, 0x3E02CD9B, 0x3E045C65, 0x3E05EB1B, 0x3E0779BC, 0x3E090848,
 0x3E0A96BF, 0x3E0C2521, 0x3E0DB36D, 0x3E0F41A4, 0x3E10CFC4, 0x3E125DCE, 0x3E13EBC1, 0x3E15799E,
 0x3E170763, 0x3E189511, 0x3E1A22A8, 0x3E1BB027, 0x3E1D3D8E, 0x3E1ECADD, 0x3E205813, 0x3E21E530,
 0x3E237235, 0x3E24FF20, 0x3E268BF2, 0x3E2818AA, 0x3E29A548, 0x3E2B31CC, 0x3E2CBE36, 0x3E2E4A85,
 0x3E2FD6B9, 0x3E3162D2, 0x3E32EECF, 0x3E347AB2, 0x3E360678, 0x3E379222, 0x3E391DB0, 0x3E3AA921,
 0x3E3C3476, 0x3E3DBFAD, 0x3E3F4AC7, 0x3E40D5C4, 0x3E4260A3, 0x3E43EB64, 0x3E457607, 0x3E47008B,
 0x3E488AF1, 0x3E4A1538, 0x3E4B9F5F, 0x3E4D2967, 0x3E4EB350, 0x3E503D19, 0x3E51C6C1, 0x3E535049,
 0x3E54D9B1, 0x3E5662F8, 0x3E57EC1D, 0x3E597522, 0x3E5AFE05, 0x3E5C86C6, 0x3E5E0F65, 0x3E5F97E2,
 0x3E61203C, 0x3E62A874, 0x3E643089, 0x3E65B87A, 0x3E674048, 0x3E68C7F3, 0x3E6A4F79, 0x3E6BD6DC,
 0x3E6D5E1A, 0x3E6EE533, 0x3E706C28, 0x3E71F2F7, 0x3E7379A1, 0x3E750026, 0x3E768685, 0x3E780CBE,
 0x3E7992D0, 0x3E7B18BC, 0x3E7C9E82, 0x3E7E2420, 0x3E7FA998, 0x3E809774, 0x3E815A08, 0x3E821C88,
 0x3E82DEF4, 0x3E83A14C, 0x3E84638F, 0x3E8525BF, 0x3E85E7D9, 0x3E86A9DF, 0x3E876BD0, 0x3E882DAD,
 0x3E88EF74, 0x3E89B126, 0x3E8A72C3, 0x3E8B344B, 0x3E8BF5BD, 0x3E8CB71A, 0x3E8D7861, 0x3E8E3992,
 0x3E8EFAAD, 0x3E8FBBB2, 0x3E907CA1, 0x3E913D79, 0x3E91FE3C, 0x3E92BEE7, 0x3E937F7D, 0x3E943FFB,
 0x3E950062, 0x3E95C0B3, 0x3E9680EC, 0x3E97410E, 0x3E980119, 0x3E98C10D, 0x3E9980E9, 0x3E9A40AD,
 0x3E9B0059, 0x3E9BBFEE, 0x3E9C7F6A, 0x3E9D3ECF, 0x3E9DFE1B, 0x3E9EBD4F, 0x3E9F7C6A, 0x3EA03B6D,
 0x3EA0FA57, 0x3EA1B928, 0x3EA277E0, 0x3EA3367F, 0x3EA3F505, 0x3EA4B372, 0x3EA571C5, 0x3EA62FFF,
 0x3EA6EE1F, 0x3EA7AC25, 0x3EA86A12, 0x3EA927E5, 0x3EA9E59D, 0x3EAAA33B, 0x3EAB60BF, 0x3EAC1E29,
 0x3EACDB78, 0x3EAD98AC, 0x3EAE55C6, 0x3EAF12C5, 0x3EAFCFA8, 0x3EB08C71, 0x3EB1491E, 0x3EB205B0,
 0x3EB2C227, 0x3EB37E82, 0x3EB43AC1, 0x3EB4F6E5, 0x3EB5B2EC, 0x3EB66ED8, 0x3EB72AA7, 0x3EB7E65B,
 0x3EB8A1F1, 0x3EB95D6C, 0x3EBA18CA, 0x3EBAD40B, 0x3EBB8F2F, 0x3EBC4A36, 0x3EBD0521, 0x3EBDBFEE,
 0x3EBE7A9E, 0x3EBF3530, 0x3EBFEFA5, 0x3EC0A9FD, 0x3EC16437, 0x3EC21E53, 0x3EC2D851, 0x3EC39231,
 0x3EC44BF2, 0x3EC50596, 0x3EC5BF1B, 0x3EC67882, 0x3EC731CA, 0x3EC7EAF3, 0x3EC8A3FD, 0x3EC95CE9,
 0x3ECA15B5, 0x3ECACE63, 0x3ECB86F1, 0x3ECC3F60, 0x3ECCF7AF, 0x3ECDAFDE, 0x3ECE67EE, 0x3ECF1FDE,
 0x3ECFD7AE, 0x3ED08F5E, 0x3ED146EE, 0x3ED1FE5E, 0x3ED2B5AD, 0x3ED36CDB, 0x3ED423EA, 0x3ED4DAD7,
 0x3ED591A4, 0x3ED6484F, 0x3ED6FEDA, 0x3ED7B543, 0x3ED86B8B, 0x3ED921B2, 0x3ED9D7B7, 0x3EDA8D9B,
 0x3EDB435D, 0x3EDBF8FD, 0x3EDCAE7C, 0x3EDD63D8, 0x3EDE1912, 0x3EDECE2A, 0x3EDF831F, 0x3EE037F2,
 0x3EE0ECA3, 0x3EE1A130, 0x3EE2559B, 0x3EE309E3, 0x3EE3BE08, 0x3EE4720A, 0x3EE525E9, 0x3EE5D9A4,
 0x3EE68D3C, 0x3EE740B1, 0x3EE7F401, 0x3EE8A72E, 0x3EE95A37, 0x3EEA0D1C, 0x3EEABFDD, 0x3EEB727A,
 0x3EEC24F3, 0x3EECD747, 0x3EED8976, 0x3EEE3B81, 0x3EEEED67, 0x3EEF9F28, 0x3EF050C5, 0x3EF1023C,
 0x3EF1B38E, 0x3EF264BB, 0x3EF315C2, 0x3EF3C6A4, 0x3EF47761, 0x3EF527F8, 0x3EF5D868, 0x3EF688B3,
 0x3EF738D8, 0x3EF7E8D7, 0x3EF898B0, 0x3EF94862, 0x3EF9F7EE, 0x3EFAA753, 0x3EFB5692, 0x3EFC05AA,
 0x3EFCB49B, 0x3EFD6365, 0x3EFE1207, 0x3EFEC083, 0x3EFF6ED8, 0x3F000E82, 0x3F006585, 0x3F00BC74,
 0x3F01134F, 0x3F016A17, 0x3F01C0CA, 0x3F021769, 0x3F026DF4, 0x3F02C46B, 0x3F031ACE, 0x3F03711D,
 0x3F03C757, 0x3F041D7E, 0x3F04738F, 0x3F04C98D, 0x3F051F75, 0x3F05754A, 0x3F05CB0A, 0x3F0620B5,
 0x3F06764B, 0x3F06CBCD, 0x3F07213A, 0x3F077692, 0x3F07CBD5, 0x3F082103, 0x3F08761C, 0x3F08CB20,
 0x3F092010, 0x3F0974E9, 0x3F09C9AE, 0x3F0A1E5E, 0x3F0A72F8, 0x3F0AC77D, 0x3F0B1BEC, 0x3F0B7046,
 0x3F0BC48B, 0x3F0C18BA, 0x3F0C6CD3, 0x3F0CC0D7, 0x3F0D14C5, 0x3F0D689D, 0x3F0DBC5F, 0x3F0E100C,
 0x3F0E63A2, 0x3F0EB723, 0x3F0F0A8E, 0x3F0F5DE2, 0x3F0FB121, 0x3F100449, 0x3F10575B, 0x3F10AA57,
 0x3F10FD3D, 0x3F11500C, 0x3F11A2C5, 0x3F11F567, 0x3F1247F3, 0x3F129A68, 0x3F12ECC7, 0x3F133F0F,
 0x3F139140, 0x3F13E35B, 0x3F14355E, 0x3F14874B, 0x3F14D921, 0x3F152AE0, 0x3F157C88, 0x3F15CE19,
 0x3F161F93, 0x3F1670F6, 0x3F16C241, 0x3F171376, 0x3F176493, 0x3F17B598, 0x3F180687, 0x3F18575D,
 0x3F18A81D, 0x3F18F8C4, 0x3F194955, 0x3F1999CD, 0x3F19EA2E, 0x3F1A3A77, 0x3F1A8AA8, 0x3F1ADAC2,
 0x3F1B2AC3, 0x3F1B7AAD, 0x3F1BCA7F, 0x3F1C1A38, 0x3F1C69DA, 0x3F1CB963, 0x3F1D08D5, 0x3F1D582E,
 0x3F1DA76F, 0x3F1DF697, 0x3F1E45A7, 0x3F1E949F, 0x3F1EE37E, 0x3F1F3245, 0x3F1F80F3, 0x3F1FCF89,
 0x3F201E06, 0x3F206C6A, 0x3F20BAB6, 0x3F2108E9, 0x3F215703, 0x3F21A504, 0x3F21F2EC, 0x3F2240BB,
 0x3F228E71, 0x3F22DC0E, 0x3F232992, 0x3F2376FD, 0x3F23C44F, 0x3F241187, 0x3F245EA6, 0x3F24ABAC,
 0x3F24F898, 0x3F25456B, 0x3F259224, 0x3F25DEC4, 0x3F262B4A, 0x3F2677B7, 0x3F26C40A, 0x3F271043,
 0x3F275C62, 0x3F27A868, 0x3F27F454, 0x3F284026, 0x3F288BDE, 0x3F28D77C, 0x3F292300, 0x3F296E69,
 0x3F29B9B9, 0x3F2A04EF, 0x3F2A500A, 0x3F2A9B0B, 0x3F2AE5F2, 0x3F2B30BE, 0x3F2B7B70, 0x3F2BC608,
 0x3F2C1085, 0x3F2C5AE7, 0x3F2CA52F, 0x3F2CEF5D, 0x3F2D396F, 0x3F2D8367, 0x3F2DCD44, 0x3F2E1707,
 0x3F2E60AE, 0x3F2EAA3B, 0x3F2EF3AD, 0x3F2F3D03, 0x3F2F863F, 0x3F2FCF60, 0x3F301865, 0x3F306150,
 0x3F30AA1F, 0x3F30F2D3, 0x3F313B6B, 0x3F3183E9, 0x3F31CC4B, 0x3F321491, 0x3F325CBC, 0x3F32A4CC,
 0x3F32ECC0, 0x3F333498, 0x3F337C55, 0x3F33C3F6, 0x3F340B7B, 0x3F3452E5, 0x3F349A33, 0x3F34E165,
 0x3F35287B, 0x3F356F75, 0x3F35B653, 0x3F35FD15, 0x3F3643BB, 0x3F368A45, 0x3F36D0B3, 0x3F371704,
 0x3F375D3A, 0x3F37A353, 0x3F37E950, 0x3F382F30, 0x3F3874F4, 0x3F38BA9C, 0x3F390027, 0x3F394595,
 0x3F398AE7, 0x3F39D01D, 0x3F3A1535, 0x3F3A5A31, 0x3F3A9F10, 0x3F3AE3D3, 0x3F3B2879, 0x3F3B6D01,
 0x3F3BB16D, 0x3F3BF5BC, 0x3F3C39EE, 0x3F3C7E03, 0x3F3CC1FB, 0x3F3D05D6, 0x3F3D4993, 0x3F3D8D34,
 0x3F3DD0B7, 0x3F3E141D, 0x3F3E5766, 0x3F3E9A91, 0x3F3EDD9F, 0x3F3F208F, 0x3F3F6362, 0x3F3FA617,
 0x3F3FE8AF, 0x3F402B2A, 0x3F406D86, 0x3F40AFC5, 0x3F40F1E7, 0x3F4133EA, 0x3F4175D0, 0x3F41B798,
 0x3F41F942, 0x3F423ACE, 0x3F427C3C, 0x3F42BD8C, 0x3F42FEBE, 0x3F433FD2, 0x3F4380C8, 0x3F43C1A0,
 0x3F44025A, 0x3F4442F5, 0x3F448372, 0x3F44C3D1, 0x3F450411, 0x3F454433, 0x3F458437, 0x3F45C41C,
 0x3F4603E3, 0x3F46438B, 0x3F468315, 0x3F46C280, 0x3F4701CC, 0x3F4740FA, 0x3F478008, 0x3F47BEF9,
 0x3F47FDCA, 0x3F483C7C, 0x3F487B10, 0x3F48B985, 0x3F48F7DA, 0x3F493611, 0x3F497429, 0x3F49B222,
 0x3F49EFFB, 0x3F4A2DB6, 0x3F4A6B51, 0x3F4AA8CD, 0x3F4AE62A, 0x3F4B2367, 0x3F4B6085, 0x3F4B9D84,
 0x3F4BDA63, 0x3F4C1723, 0x3F4C53C4, 0x3F4C9045, 0x3F4CCCA6, 0x3F4D08E8, 0x3F4D450A, 0x3F4D810D,
 0x3F4DBCF0, 0x3F4DF8B3, 0x3F4E3456, 0x3F4E6FDA, 0x3F4EAB3E, 0x3F4EE681, 0x3F4F21A5, 0x3F4F5CA9,
 0x3F4F978D, 0x3F4FD252, 0x3F500CF6, 0x3F504779, 0x3F5081DD, 0x3F50BC21, 0x3F50F644, 0x3F513047,
 0x3F516A2A, 0x3F51A3ED, 0x3F51DD8F, 0x3F521711, 0x3F525073, 0x3F5289B4, 0x3F52C2D5, 0x3F52FBD5,
 0x3F5334B5, 0x3F536D74, 0x3F53A612, 0x3F53DE90, 0x3F5416ED, 0x3F544F2A, 0x3F548745, 0x3F54BF40,
 0x3F54F71A, 0x3F552ED4, 0x3F55666C, 0x3F559DE3, 0x3F55D53A, 0x3F560C70, 0x3F564384, 0x3F567A78,
 0x3F56B14A, 0x3F56E7FB, 0x3F571E8C, 0x3F5754FB, 0x3F578B48, 0x3F57C175, 0x3F57F780, 0x3F582D6A,
 0x3F586333, 0x3F5898DA, 0x3F58CE60, 0x3F5903C5, 0x3F593908, 0x3F596E29, 0x3F59A329, 0x3F59D807,
 0x3F5A0CC4, 0x3F5A415F, 0x3F5A75D9, 0x3F5AAA31, 0x3F5ADE67, 0x3F5B127B, 0x3F5B466E, 0x3F5B7A3E,
 0x3F5BADED, 0x3F5BE17A, 0x3F5C14E6, 0x3F5C482F, 0x3F5C7B56, 0x3F5CAE5B, 0x3F5CE13E, 0x3F5D13FF,
 0x3F5D469E, 0x3F5D791B, 0x3F5DAB76, 0x3F5DDDAF, 0x3F5E0FC5, 0x3F5E41B9, 0x3F5E738B, 0x3F5EA53A,
 0x3F5ED6C8, 0x3F5F0833, 0x3F5F397B, 0x3F5F6AA1, 0x3F5F9BA5, 0x3F5FCC86, 0x3F5FFD44, 0x3F602DE0,
 0x3F605E5A, 0x3F608EB0, 0x3F60BEE5, 0x3F60EEF6, 0x3F611EE5, 0x3F614EB1, 0x3F617E5B, 0x3F61ADE1,
 0x3F61DD45, 0x3F620C86, 0x3F623BA4, 0x3F626AA0, 0x3F629978, 0x3F62C82D, 0x3F62F6C0, 0x3F63252F,
 0x3F63537B, 0x3F6381A5, 0x3F63AFAB, 0x3F63DD8E, 0x3F640B4E, 0x3F6438EB, 0x3F646665, 0x3F6493BB,
 0x3F64C0EE, 0x3F64EDFE, 0x3F651AEB, 0x3F6547B4, 0x3F65745A, 0x3F65A0DC, 0x3F65CD3B, 0x3F65F977,
 0x3F66258F, 0x3F665184, 0x3F667D55, 0x3F66A903, 0x3F66D48D, 0x3F66FFF3, 0x3F672B36, 0x3F675655,
 0x3F678151, 0x3F67AC29, 0x3F67D6DD, 0x3F68016D, 0x3F682BDA, 0x3F685623, 0x3F688047, 0x3F68AA49,
 0x3F68D426, 0x3F68FDDF, 0x3F692774, 0x3F6950E6, 0x3F697A33, 0x3F69A35D, 0x3F69CC62, 0x3F69F543,
 0x3F6A1E01, 0x3F6A469A, 0x3F6A6F0F, 0x3F6A9760, 0x3F6ABF8C, 0x3F6AE795, 0x3F6B0F79, 0x3F6B3739,
 0x3F6B5ED5, 0x3F6B864C, 0x3F6BADA0, 0x3F6BD4CE, 0x3F6BFBD9, 0x3F6C22BF, 0x3F6C4980, 0x3F6C701E,
 0x3F6C9696, 0x3F6CBCEA, 0x3F6CE31A, 0x3F6D0925, 0x3F6D2F0C, 0x3F6D54CE, 0x3F6D7A6C, 0x3F6D9FE4,
 0x3F6DC539, 0x3F6DEA68, 0x3F6E0F73, 0x3F6E3459, 0x3F6E591A, 0x3F6E7DB7, 0x3F6EA22F, 0x3F6EC682,
 0x3F6EEAB0, 0x3F6F0EBA, 0x3F6F329E, 0x3F6F565E, 0x3F6F79F8, 0x3F6F9D6E, 0x3F6FC0BF, 0x3F6FE3EB,
 0x3F7006F2, 0x3F7029D4, 0x3F704C91, 0x3F706F28, 0x3F70919B, 0x3F70B3E9, 0x3F70D611, 0x3F70F814,
 0x3F7119F3, 0x3F713BAC, 0x3F715D3F, 0x3F717EAE, 0x3F719FF7, 0x3F71C11B, 0x3F71E21A, 0x3F7202F4,
 0x3F7223A8, 0x3F724437, 0x3F7264A0, 0x3F7284E4, 0x3F72A503, 0x3F72C4FC, 0x3F72E4D0, 0x3F73047E,
 0x3F732407, 0x3F73436B, 0x3F7362A8, 0x3F7381C1, 0x3F73A0B4, 0x3F73BF81, 0x3F73DE28, 0x3F73FCAA,
 0x3F741B07, 0x3F74393E, 0x3F74574F, 0x3F74753A, 0x3F749300, 0x3F74B0A0, 0x3F74CE1A, 0x3F74EB6F,
 0x3F75089D, 0x3F7525A6, 0x3F754289, 0x3F755F47, 0x3F757BDE, 0x3F759850, 0x3F75B49C, 0x3F75D0C2,
 0x3F75ECC2, 0x3F76089C, 0x3F762450, 0x3F763FDE, 0x3F765B46, 0x3F767688, 0x3F7691A4, 0x3F76AC9A,
 0x3F76C76B, 0x3F76E215, 0x3F76FC99, 0x3F7716F6, 0x3F77312E, 0x3F774B40, 0x3F77652B, 0x3F777EF1,
 0x3F779890, 0x3F77B209, 0x3F77CB5C, 0x3F77E488, 0x3F77FD8F, 0x3F78166F, 0x3F782F29, 0x3F7847BC,
 0x3F78602A, 0x3F787871, 0x3F789091, 0x3F78A88C, 0x3F78C060, 0x3F78D80E, 0x3F78EF95, 0x3F7906F6,
 0x3F791E30, 0x3F793544, 0x3F794C32, 0x3F7962F9, 0x3F79799A, 0x3F799014, 0x3F79A668, 0x3F79BC95,
 0x3F79D29C, 0x3F79E87C, 0x3F79FE36, 0x3F7A13C9, 0x3F7A2936, 0x3F7A3E7C, 0x3F7A539B, 0x3F7A6894,
 0x3F7A7D66, 0x3F7A9212, 0x3F7AA697, 0x3F7ABAF5, 0x3F7ACF2D, 0x3F7AE33D, 0x3F7AF728, 0x3F7B0AEB,
 0x3F7B1E88, 0x3F7B31FE, 0x3F7B454E, 0x3F7B5876, 0x3F7B6B78, 0x3F7B7E53, 0x3F7B9107, 0x3F7BA395,
 0x3F7BB5FC, 0x3F7BC83B, 0x3F7BDA55, 0x3F7BEC47, 0x3F7BFE12, 0x3F7C0FB7, 0x3F7C2134, 0x3F7C328B,
 0x3F7C43BB, 0x3F7C54C4, 0x3F7C65A6, 0x3F7C7661, 0x3F7C86F5, 0x3F7C9762, 0x3F7CA7A9, 0x3F7CB7C8,
 0x3F7CC7C0, 0x3F7CD792, 0x3F7CE73C, 0x3F7CF6C0, 0x3F7D061C, 0x3F7D1551, 0x3F7D2460, 0x3F7D3347,
 0x3F7D4207, 0x3F7D50A0, 0x3F7D5F13, 0x3F7D6D5E, 0x3F7D7B82, 0x3F7D897E, 0x3F7D9754, 0x3F7DA503,
 0x3F7DB28A, 0x3F7DBFEB, 0x3F7DCD24, 0x3F7DDA36, 0x3F7DE721, 0x3F7DF3E5, 0x3F7E0082, 0x3F7E0CF7,
 0x3F7E1946, 0x3F7E256D, 0x3F7E316D, 0x3F7E3D46, 0x3F7E48F7, 0x3F7E5482, 0x3F7E5FE5, 0x3F7E6B21,
 0x3F7E7635, 0x3F7E8123, 0x3F7E8BE9, 0x3F7E9688, 0x3F7EA100, 0x3F7EAB50, 0x3F7EB579, 0x3F7EBF7B,
 0x3F7EC955, 0x3F7ED309, 0x3F7EDC95, 0x3F7EE5F9, 0x3F7EEF37, 0x3F7EF84D, 0x3F7F013C, 0x3F7F0A03,
 0x3F7F12A3, 0x3F7F1B1C, 0x3F7F236D, 0x3F7F2B97, 0x3F7F339A, 0x3F7F3B75, 0x3F7F4329, 0x3F7F4AB6,
 0x3F7F521B, 0x3F7F5959, 0x3F7F606F, 0x3F7F675F, 0x3F7F6E26, 0x3F7F74C7, 0x3F7F7B40, 0x3F7F8191,
 0x3F7F87BB, 0x3F7F8DBE, 0x3F7F9399, 0x3F7F994D, 0x3F7F9EDA, 0x3F7FA43F, 0x3F7FA97D, 0x3F7FAE93,
 0x3F7FB382, 0x3F7FB849, 0x3F7FBCE9, 0x3F7FC161, 0x3F7FC5B2, 0x3F7FC9DC, 0x3F7FCDDE, 0x3F7FD1B9,
 0x3F7FD56C, 0x3F7FD8F8, 0x3F7FDC5C, 0x3F7FDF99, 0x3F7FE2AF, 0x3F7FE59D, 0x3F7FE863, 0x3F7FEB02,
 0x3F7FED7A, 0x3F7FEFCA, 0x3F7FF1F3, 0x3F7FF3F4, 0x3F7FF5CE, 0x3F7FF780, 0x3F7FF90B, 0x3F7FFA6E,
 0x3F7FFBAA, 0x3F7FFCBE, 0x3F7FFDAB, 0x3F7FFE70, 0x3F7FFF0E, 0x3F7FFF85, 0x3F7FFFD4, 0x3F7FFFFB
};

static const unsigned int sine_short_128[ 128 ] __attribute__(   (  section( ".rodata" ), aligned( 16 )  )   ) = {
 0x3BC90F88, 0x3C96C9B6, 0x3CFB49BA, 0x3D2FE007, 0x3D621469, 0x3D8A200A, 0x3DA3308C, 0x3DBC3AC3,
 0x3DD53DB9, 0x3DEE3876, 0x3E039502, 0x3E1008B7, 0x3E1C76DE, 0x3E28DEFC, 0x3E354098, 0x3E419B37,
 0x3E4DEE60, 0x3E5A3997, 0x3E667C66, 0x3E72B651, 0x3E7EE6E1, 0x3E8586CE, 0x3E8B9507, 0x3E919DDD,
 0x3E97A117, 0x3E9D9E78, 0x3EA395C5, 0x3EA986C4, 0x3EAF713A, 0x3EB554EC, 0x3EBB31A0, 0x3EC1071E,
 0x3EC6D529, 0x3ECC9B8B, 0x3ED25A09, 0x3ED8106B, 0x3EDDBE79, 0x3EE363FA, 0x3EE900B7, 0x3EEE9479,
 0x3EF41F07, 0x3EF9A02D, 0x3EFF17B2, 0x3F0242B1, 0x3F04F484, 0x3F07A136, 0x3F0A48AD, 0x3F0CEAD0,
 0x3F0F8784, 0x3F121EB0, 0x3F14B039, 0x3F173C07, 0x3F19C200, 0x3F1C420C, 0x3F1EBC12, 0x3F212FF9,
 0x3F239DA9, 0x3F26050A, 0x3F286605, 0x3F2AC082, 0x3F2D1469, 0x3F2F61A5, 0x3F31A81D, 0x3F33E7BC,
 0x3F36206C, 0x3F385216, 0x3F3A7CA4, 0x3F3CA003, 0x3F3EBC1B, 0x3F40D0DA, 0x3F42DE29, 0x3F44E3F5,
 0x3F46E22A, 0x3F48D8B3, 0x3F4AC77F, 0x3F4CAE79, 0x3F4E8D90, 0x3F5064AF, 0x3F5233C6, 0x3F53FAC3,
 0x3F55B993, 0x3F577026, 0x3F591E6A, 0x3F5AC450, 0x3F5C61C7, 0x3F5DF6BE, 0x3F5F8327, 0x3F6106F2,
 0x3F628210, 0x3F63F473, 0x3F655E0B, 0x3F66BECC, 0x3F6816A8, 0x3F696591, 0x3F6AAB7B, 0x3F6BE858,
 0x3F6D1C1D, 0x3F6E46BE, 0x3F6F6830, 0x3F708066, 0x3F718F57, 0x3F7294F8, 0x3F73913F, 0x3F748422,
 0x3F756D97, 0x3F764D97, 0x3F772417, 0x3F77F110, 0x3F78B47B, 0x3F796E4E, 0x3F7A1E84, 0x3F7AC516,
 0x3F7B61FC, 0x3F7BF531, 0x3F7C7EB0, 0x3F7CFE73, 0x3F7D7474, 0x3F7DE0B1, 0x3F7E4323, 0x3F7E9BC9,
 0x3F7EEA9D, 0x3F7F2F9D, 0x3F7F6AC7, 0x3F7F9C18, 0x3F7FC38F, 0x3F7FE129, 0x3F7FF4E6, 0x3F7FFEC4
};

static const unsigned int kbd_long_1024[ 1024 ] __attribute__(   (  section( ".rodata" ), aligned( 16 )  )   ) = {
 0x399962F2, 0x39E16FB3, 0x3A0F5326, 0x3A2BA86E, 0x3A46E3A0, 0x3A619971, 0x3A7C1FA8, 0x3A8B5668,
 0x3A98B309, 0x3AA63310, 0x3AB3E07C, 0x3AC1C310, 0x3ACFE100, 0x3ADE3F64, 0x3AECE283, 0x3AFBCE05,
 0x3B05828F, 0x3B0D4550, 0x3B15308D, 0x3B1D456F, 0x3B258506, 0x3B2DF053, 0x3B368843, 0x3B3F4DBA,
 0x3B484190, 0x3B516492, 0x3B5AB788, 0x3B643B33, 0x3B6DF050, 0x3B77D794, 0x3B80F8D9, 0x3B861FAE,
 0x3B8B609E, 0x3B90BBFF, 0x3B963224, 0x3B9BC362, 0x3BA17009, 0x3BA7386C, 0x3BAD1CDC, 0x3BB31DA8,
 0x3BB93B21, 0x3BBF7597, 0x3BC5CD57, 0x3BCC42B1, 0x3BD2D5F4, 0x3BD9876C, 0x3BE05769, 0x3BE74638,
 0x3BEE5427, 0x3BF58182, 0x3BFCCE97, 0x3C021DDA, 0x3C05E492, 0x3C09BB9A, 0x3C0DA319, 0x3C119B35,
 0x3C15A414, 0x3C19BDDC, 0x3C1DE8B4, 0x3C2224C1, 0x3C26722A, 0x3C2AD115, 0x3C2F41A7, 0x3C33C406,
 0x3C385859, 0x3C3CFEC5, 0x3C41B771, 0x3C468280, 0x3C4B601B, 0x3C505065, 0x3C555385, 0x3C5A69A1,
 0x3C5F92DD, 0x3C64CF5F, 0x3C6A1F4D, 0x3C6F82CC, 0x3C74FA01, 0x3C7A8511, 0x3C801211, 0x3C82EBAC,
 0x3C85CF6D, 0x3C88BD66, 0x3C8BB5A8, 0x3C8EB848, 0x3C91C556, 0x3C94DCE6, 0x3C97FF09, 0x3C9B2BD3,
 0x3C9E6354, 0x3CA1A59F, 0x3CA4F2C7, 0x3CA84ADD, 0x3CABADF3, 0x3CAF1C1B, 0x3CB29568, 0x3CB619EA,
 0x3CB9A9B5, 0x3CBD44D8, 0x3CC0EB67, 0x3CC49D73, 0x3CC85B0C, 0x3CCC2446, 0x3CCFF930, 0x3CD3D9DD,
 0x3CD7C65D, 0x3CDBBEC3, 0x3CDFC31E, 0x3CE3D381, 0x3CE7EFFB, 0x3CEC189E, 0x3CF04D7B, 0x3CF48EA3,
 0x3CF8DC25, 0x3CFD3614, 0x3D00CE3F, 0x3D0307BA, 0x3D054784, 0x3D078DA5, 0x3D09DA24, 0x3D0C2D09,
 0x3D0E865E, 0x3D10E628, 0x3D134C71, 0x3D15B940, 0x3D182C9D, 0x3D1AA690, 0x3D1D2720, 0x3D1FAE55,
 0x3D223C37, 0x3D24D0CD, 0x3D276C1E, 0x3D2A0E31, 0x3D2CB70F, 0x3D2F66BF, 0x3D321D47, 0x3D34DAAF,
 0x3D379EFD, 0x3D3A6A3A, 0x3D3D3C6C, 0x3D401599, 0x3D42F5C9, 0x3D45DD03, 0x3D48CB4D, 0x3D4BC0AD,
 0x3D4EBD2B, 0x3D51C0CD, 0x3D54CB99, 0x3D57DD96, 0x3D5AF6C9, 0x3D5E173A, 0x3D613EEF, 0x3D646DED,
 0x3D67A43B, 0x3D6AE1DE, 0x3D6E26DC, 0x3D71733C, 0x3D74C702, 0x3D782235, 0x3D7B84DA, 0x3D7EEEF7,
 0x3D813048, 0x3D82ECD6, 0x3D84AD28, 0x3D867140, 0x3D883921, 0x3D8A04CD, 0x3D8BD447, 0x3D8DA792,
 0x3D8F7EAF, 0x3D9159A1, 0x3D93386A, 0x3D951B0D, 0x3D97018C, 0x3D98EBE8, 0x3D9ADA25, 0x3D9CCC43,
 0x3D9EC246, 0x3DA0BC2F, 0x3DA2B9FF, 0x3DA4BBBA, 0x3DA6C161, 0x3DA8CAF4, 0x3DAAD878, 0x3DACE9EC,
 0x3DAEFF53, 0x3DB118AE, 0x3DB335FF, 0x3DB55747, 0x3DB77C88, 0x3DB9A5C3, 0x3DBBD2FA, 0x3DBE042D,
 0x3DC0395F, 0x3DC27290, 0x3DC4AFC2, 0x3DC6F0F5, 0x3DC9362B, 0x3DCB7F64, 0x3DCDCCA3, 0x3DD01DE7,
 0x3DD27331, 0x3DD4CC82, 0x3DD729DC, 0x3DD98B3E, 0x3DDBF0A9, 0x3DDE5A1F, 0x3DE0C79F, 0x3DE3392A,
 0x3DE5AEC0, 0x3DE82862, 0x3DEAA610, 0x3DED27CB, 0x3DEFAD92, 0x3DF23766, 0x3DF4C546, 0x3DF75734,
 0x3DF9ED2F, 0x3DFC8736, 0x3DFF254A, 0x3E00E3B6, 0x3E0236CC, 0x3E038BE9, 0x3E04E30B, 0x3E063C34,
 0x3E079761, 0x3E08F494, 0x3E0A53CC, 0x3E0BB508, 0x3E0D1849, 0x3E0E7D8D, 0x3E0FE4D5, 0x3E114E20,
 0x3E12B96E, 0x3E1426BE, 0x3E159610, 0x3E170762, 0x3E187AB5, 0x3E19F009, 0x3E1B675B, 0x3E1CE0AC,
 0x3E1E5BFB, 0x3E1FD947, 0x3E215890, 0x3E22D9D5, 0x3E245D14, 0x3E25E24E, 0x3E276981, 0x3E28F2AC,
 0x3E2A7DCE, 0x3E2C0AE8, 0x3E2D99F6, 0x3E2F2AF9, 0x3E30BDF0, 0x3E3252D9, 0x3E33E9B3, 0x3E35827E,
 0x3E371D37, 0x3E38B9DF, 0x3E3A5873, 0x3E3BF8F2, 0x3E3D9B5B, 0x3E3F3FAE, 0x3E40E5E8, 0x3E428E07,
 0x3E44380C, 0x3E45E3F4, 0x3E4791BE, 0x3E494168, 0x3E4AF2F1, 0x3E4CA658, 0x3E4E5B9A, 0x3E5012B6,
 0x3E51CBAB, 0x3E538677, 0x3E554318, 0x3E57018D, 0x3E58C1D3, 0x3E5A83EA, 0x3E5C47CE, 0x3E5E0D7F,
 0x3E5FD4FB, 0x3E619E3F, 0x3E63694A, 0x3E65361A, 0x3E6704AD, 0x3E68D500, 0x3E6AA712, 0x3E6C7AE1,
 0x3E6E506B, 0x3E7027AD, 0x3E7200A6, 0x3E73DB53, 0x3E75B7B2, 0x3E7795C1, 0x3E79757D, 0x3E7B56E5,
 0x3E7D39F6, 0x3E7F1EAD, 0x3E808285, 0x3E817683, 0x3E826B52, 0x3E8360EF, 0x3E84575A, 0x3E854E91,
 0x3E864692, 0x3E873F5D, 0x3E8838F0, 0x3E89334A, 0x3E8A2E6A, 0x3E8B2A4E, 0x3E8C26F5, 0x3E8D245E,
 0x3E8E2287, 0x3E8F216F, 0x3E902114, 0x3E912176, 0x3E922292, 0x3E932468, 0x3E9426F5, 0x3E952A39,
 0x3E962E31, 0x3E9732DD, 0x3E98383B, 0x3E993E4A, 0x3E9A4507, 0x3E9B4C72, 0x3E9C5489, 0x3E9D5D4A,
 0x3E9E66B4, 0x3E9F70C5, 0x3EA07B7C, 0x3EA186D6, 0x3EA292D4, 0x3EA39F72, 0x3EA4ACAF, 0x3EA5BA8A,
 0x3EA6C901, 0x3EA7D812, 0x3EA8E7BB, 0x3EA9F7FC, 0x3EAB08D2, 0x3EAC1A3B, 0x3EAD2C37, 0x3EAE3EC2,
 0x3EAF51DC, 0x3EB06583, 0x3EB179B4, 0x3EB28E6F, 0x3EB3A3B1, 0x3EB4B978, 0x3EB5CFC4, 0x3EB6E691,
 0x3EB7FDDF, 0x3EB915AB, 0x3EBA2DF4, 0x3EBB46B7, 0x3EBC5FF4, 0x3EBD79A8, 0x3EBE93D0, 0x3EBFAE6D,
 0x3EC0C97A, 0x3EC1E4F8, 0x3EC300E3, 0x3EC41D3A, 0x3EC539FB, 0x3EC65724, 0x3EC774B3, 0x3EC892A6,
 0x3EC9B0FB, 0x3ECACFB1, 0x3ECBEEC5, 0x3ECD0E36, 0x3ECE2E01, 0x3ECF4E25, 0x3ED06E9F, 0x3ED18F6D,
 0x3ED2B08F, 0x3ED3D200, 0x3ED4F3C1, 0x3ED615CE, 0x3ED73825, 0x3ED85AC5, 0x3ED97DAC, 0x3EDAA0D7,
 0x3EDBC444, 0x3EDCE7F2, 0x3EDE0BDF, 0x3EDF3008, 0x3EE0546B, 0x3EE17906, 0x3EE29DD8, 0x3EE3C2DD,
 0x3EE4E815, 0x3EE60D7D, 0x3EE73313, 0x3EE858D4, 0x3EE97EBF, 0x3EEAA4D3, 0x3EEBCB0B, 0x3EECF167,
 0x3EEE17E5, 0x3EEF3E82, 0x3EF0653C, 0x3EF18C12, 0x3EF2B300, 0x3EF3DA05, 0x3EF50120, 0x3EF6284C,
 0x3EF74F8A, 0x3EF876D6, 0x3EF99E2E, 0x3EFAC591, 0x3EFBECFC, 0x3EFD146D, 0x3EFE3BE2, 0x3EFF6359,
 0x3F004567, 0x3F00D921, 0x3F016CD9, 0x3F02008E, 0x3F02943E, 0x3F0327E8, 0x3F03BB8C, 0x3F044F29,
 0x3F04E2BD, 0x3F057647, 0x3F0609C6, 0x3F069D3A, 0x3F0730A1, 0x3F07C3FA, 0x3F085744, 0x3F08EA7E,
 0x3F097DA6, 0x3F0A10BD, 0x3F0AA3C1, 0x3F0B36B0, 0x3F0BC98A, 0x3F0C5C4E, 0x3F0CEEFB, 0x3F0D818F,
 0x3F0E1409, 0x3F0EA669, 0x3F0F38AE, 0x3F0FCAD6, 0x3F105CE1, 0x3F10EECC, 0x3F118099, 0x3F121244,
 0x3F12A3CE, 0x3F133535, 0x3F13C678, 0x3F145796, 0x3F14E88F, 0x3F157961, 0x3F160A0B, 0x3F169A8C,
 0x3F172AE3, 0x3F17BB0F, 0x3F184B10, 0x3F18DAE3, 0x3F196A89, 0x3F19FA00, 0x3F1A8947, 0x3F1B185D,
 0x3F1BA741, 0x3F1C35F3, 0x3F1CC470, 0x3F1D52B9, 0x3F1DE0CC, 0x3F1E6EA9, 0x3F1EFC4E, 0x3F1F89BA,
 0x3F2016ED, 0x3F20A3E5, 0x3F2130A1, 0x3F21BD21, 0x3F224964, 0x3F22D569, 0x3F23612E, 0x3F23ECB3,
 0x3F2477F7, 0x3F2502F9, 0x3F258DB8, 0x3F261833, 0x3F26A26A, 0x3F272C5A, 0x3F27B605, 0x3F283F67,
 0x3F28C882, 0x3F295153, 0x3F29D9DA, 0x3F2A6217, 0x3F2AEA07, 0x3F2B71AB, 0x3F2BF901, 0x3F2C8009,
 0x3F2D06C1, 0x3F2D8D2A, 0x3F2E1341, 0x3F2E9907, 0x3F2F1E7A, 0x3F2FA39A, 0x3F302865, 0x3F30ACDC,
 0x3F3130FD, 0x3F31B4C7, 0x3F32383A, 0x3F32BB54, 0x3F333E16, 0x3F33C07D, 0x3F34428B, 0x3F34C43D,
 0x3F354593, 0x3F35C68C, 0x3F364727, 0x3F36C765, 0x3F374743, 0x3F37C6C2, 0x3F3845E0, 0x3F38C49D,
 0x3F3942F9, 0x3F39C0F1, 0x3F3A3E87, 0x3F3ABBB8, 0x3F3B3885, 0x3F3BB4ED, 0x3F3C30EF, 0x3F3CAC8A,
 0x3F3D27BE, 0x3F3DA28A, 0x3F3E1CED, 0x3F3E96E8, 0x3F3F1078, 0x3F3F899E, 0x3F40025A, 0x3F407AA9,
 0x3F40F28D, 0x3F416A03, 0x3F41E10C, 0x3F4257A8, 0x3F42CDD4, 0x3F434392, 0x3F43B8E0, 0x3F442DBE,
 0x3F44A22B, 0x3F451627, 0x3F4589B2, 0x3F45FCCA, 0x3F466F6F, 0x3F46E1A1, 0x3F47535F, 0x3F47C4A9,
 0x3F48357F, 0x3F48A5DF, 0x3F4915C9, 0x3F49853D, 0x3F49F43B, 0x3F4A62C2, 0x3F4AD0D2, 0x3F4B3E69,
 0x3F4BAB88, 0x3F4C182F, 0x3F4C845D, 0x3F4CF011, 0x3F4D5B4B, 0x3F4DC60B, 0x3F4E3051, 0x3F4E9A1C,
 0x3F4F036B, 0x3F4F6C3F, 0x3F4FD497, 0x3F503C72, 0x3F50A3D1, 0x3F510AB3, 0x3F517118, 0x3F51D6FF,
 0x3F523C68, 0x3F52A153, 0x3F5305C0, 0x3F5369AF, 0x3F53CD1E, 0x3F54300E, 0x3F54927F, 0x3F54F471,
 0x3F5555E2, 0x3F55B6D4, 0x3F561745, 0x3F567736, 0x3F56D6A6, 0x3F573595, 0x3F579403, 0x3F57F1F0,
 0x3F584F5C, 0x3F58AC46, 0x3F5908AF, 0x3F596496, 0x3F59BFFB, 0x3F5A1ADE, 0x3F5A753E, 0x3F5ACF1D,
 0x3F5B2879, 0x3F5B8153, 0x3F5BD9AA, 0x3F5C317F, 0x3F5C88D1, 0x3F5CDFA0, 0x3F5D35ED, 0x3F5D8BB7,
 0x3F5DE0FE, 0x3F5E35C2, 0x3F5E8A03, 0x3F5EDDC1, 0x3F5F30FD, 0x3F5F83B5, 0x3F5FD5EB, 0x3F60279E,
 0x3F6078CE, 0x3F60C97B, 0x3F6119A6, 0x3F61694E, 0x3F61B873, 0x3F620715, 0x3F625535, 0x3F62A2D3,
 0x3F62EFEE, 0x3F633C87, 0x3F63889E, 0x3F63D433, 0x3F641F46, 0x3F6469D7, 0x3F64B3E6, 0x3F64FD74,
 0x3F654681, 0x3F658F0C, 0x3F65D716, 0x3F661EA0, 0x3F6665A8, 0x3F66AC30, 0x3F66F238, 0x3F6737BF,
 0x3F677CC7, 0x3F67C14E, 0x3F680556, 0x3F6848DF, 0x3F688BE9, 0x3F68CE74, 0x3F691080, 0x3F69520E,
 0x3F69931D, 0x3F69D3AF, 0x3F6A13C3, 0x3F6A535A, 0x3F6A9274, 0x3F6AD111, 0x3F6B0F31, 0x3F6B4CD5,
 0x3F6B89FE, 0x3F6BC6AB, 0x3F6C02DC, 0x3F6C3E93, 0x3F6C79CF, 0x3F6CB490, 0x3F6CEED8, 0x3F6D28A6,
 0x3F6D61FB, 0x3F6D9AD7, 0x3F6DD33B, 0x3F6E0B26, 0x3F6E429A, 0x3F6E7996, 0x3F6EB01B, 0x3F6EE629,
 0x3F6F1BC2, 0x3F6F50E4, 0x3F6F8591, 0x3F6FB9C9, 0x3F6FED8C, 0x3F7020DB, 0x3F7053B6, 0x3F70861E,
 0x3F70B813, 0x3F70E996, 0x3F711AA6, 0x3F714B45, 0x3F717B73, 0x3F71AB30, 0x3F71DA7C, 0x3F720959,
 0x3F7237C7, 0x3F7265C6, 0x3F729357, 0x3F72C079, 0x3F72ED2F, 0x3F731977, 0x3F734553, 0x3F7370C3,
 0x3F739BC8, 0x3F73C662, 0x3F73F091, 0x3F741A57, 0x3F7443B3, 0x3F746CA6, 0x3F749531, 0x3F74BD55,
 0x3F74E511, 0x3F750C66, 0x3F753355, 0x3F7559DE, 0x3F758003, 0x3F75A5C2, 0x3F75CB1E, 0x3F75F016,
 0x3F7614AB, 0x3F7638DE, 0x3F765CAF, 0x3F76801F, 0x3F76A32E, 0x3F76C5DD, 0x3F76E82C, 0x3F770A1C,
 0x3F772BAE, 0x3F774CE2, 0x3F776DB9, 0x3F778E33, 0x3F77AE51, 0x3F77CE13, 0x3F77ED7A, 0x3F780C87,
 0x3F782B3A, 0x3F784994, 0x3F786795, 0x3F78853E, 0x3F78A290, 0x3F78BF8B, 0x3F78DC2F, 0x3F78F87E,
 0x3F791478, 0x3F79301D, 0x3F794B6F, 0x3F79666D, 0x3F798118, 0x3F799B72, 0x3F79B57A, 0x3F79CF31,
 0x3F79E897, 0x3F7A01AE, 0x3F7A1A76, 0x3F7A32EF, 0x3F7A4B1B, 0x3F7A62F9, 0x3F7A7A8A, 0x3F7A91D0,
 0x3F7AA8CA, 0x3F7ABF79, 0x3F7AD5DE, 0x3F7AEBF9, 0x3F7B01CB, 0x3F7B1754, 0x3F7B2C96, 0x3F7B4190,
 0x3F7B5644, 0x3F7B6AB2, 0x3F7B7EDA, 0x3F7B92BE, 0x3F7BA65D, 0x3F7BB9B8, 0x3F7BCCD0, 0x3F7BDFA6,
 0x3F7BF23A, 0x3F7C048D, 0x3F7C169F, 0x3F7C2871, 0x3F7C3A03, 0x3F7C4B57, 0x3F7C5C6C, 0x3F7C6D43,
 0x3F7C7DDD, 0x3F7C8E3B, 0x3F7C9E5C, 0x3F7CAE43, 0x3F7CBDEE, 0x3F7CCD5F, 0x3F7CDC96, 0x3F7CEB95,
 0x3F7CFA5A, 0x3F7D08E8, 0x3F7D173E, 0x3F7D255E, 0x3F7D3347, 0x3F7D40FA, 0x3F7D4E79, 0x3F7D5BC3,
 0x3F7D68D8, 0x3F7D75BB, 0x3F7D826A, 0x3F7D8EE7, 0x3F7D9B32, 0x3F7DA74C, 0x3F7DB335, 0x3F7DBEEE,
 0x3F7DCA77, 0x3F7DD5D1, 0x3F7DE0FC, 0x3F7DEBFA, 0x3F7DF6C9, 0x3F7E016C, 0x3F7E0BE3, 0x3F7E162D,
 0x3F7E204C, 0x3F7E2A40, 0x3F7E3409, 0x3F7E3DA9, 0x3F7E471F, 0x3F7E506C, 0x3F7E5991, 0x3F7E628E,
 0x3F7E6B63, 0x3F7E7412, 0x3F7E7C9A, 0x3F7E84FC, 0x3F7E8D39, 0x3F7E9550, 0x3F7E9D44, 0x3F7EA513,
 0x3F7EACBE, 0x3F7EB446, 0x3F7EBBAC, 0x3F7EC2EF, 0x3F7ECA11, 0x3F7ED112, 0x3F7ED7F1, 0x3F7EDEB0,
 0x3F7EE550, 0x3F7EEBCF, 0x3F7EF230, 0x3F7EF872, 0x3F7EFE96, 0x3F7F049B, 0x3F7F0A84, 0x3F7F1050,
 0x3F7F15FF, 0x3F7F1B92, 0x3F7F2109, 0x3F7F2665, 0x3F7F2BA6, 0x3F7F30CC, 0x3F7F35D9, 0x3F7F3ACB,
 0x3F7F3FA5, 0x3F7F4465, 0x3F7F490D, 0x3F7F4D9C, 0x3F7F5214, 0x3F7F5674, 0x3F7F5ABD, 0x3F7F5EF0,
 0x3F7F630C, 0x3F7F6712, 0x3F7F6B02, 0x3F7F6EDD, 0x3F7F72A3, 0x3F7F7655, 0x3F7F79F2, 0x3F7F7D7B,
 0x3F7F80F1, 0x3F7F8453, 0x3F7F87A3, 0x3F7F8ADF, 0x3F7F8E0A, 0x3F7F9122, 0x3F7F9428, 0x3F7F971E,
 0x3F7F9A02, 0x3F7F9CD5, 0x3F7F9F98, 0x3F7FA24A, 0x3F7FA4ED, 0x3F7FA780, 0x3F7FAA03, 0x3F7FAC78,
 0x3F7FAEDE, 0x3F7FB135, 0x3F7FB37E, 0x3F7FB5B9, 0x3F7FB7E6, 0x3F7FBA05, 0x3F7FBC18, 0x3F7FBE1D,
 0x3F7FC016, 0x3F7FC202, 0x3F7FC3E2, 0x3F7FC5B6, 0x3F7FC77E, 0x3F7FC93B, 0x3F7FCAEC, 0x3F7FCC93,
 0x3F7FCE2E, 0x3F7FCFBF, 0x3F7FD145, 0x3F7FD2C1, 0x3F7FD434, 0x3F7FD59C, 0x3F7FD6FB, 0x3F7FD850,
 0x3F7FD99C, 0x3F7FDAE0, 0x3F7FDC1A, 0x3F7FDD4C, 0x3F7FDE75, 0x3F7FDF97, 0x3F7FE0B0, 0x3F7FE1C1,
 0x3F7FE2CA, 0x3F7FE3CC, 0x3F7FE4C7, 0x3F7FE5BA, 0x3F7FE6A7, 0x3F7FE78C, 0x3F7FE86B, 0x3F7FE943,
 0x3F7FEA15, 0x3F7FEAE1, 0x3F7FEBA6, 0x3F7FEC65, 0x3F7FED1F, 0x3F7FEDD3, 0x3F7FEE82, 0x3F7FEF2B,
 0x3F7FEFCE, 0x3F7FF06D, 0x3F7FF107, 0x3F7FF19B, 0x3F7FF22B, 0x3F7FF2B7, 0x3F7FF33D, 0x3F7FF3C0,
 0x3F7FF43E, 0x3F7FF4B8, 0x3F7FF52E, 0x3F7FF5A0, 0x3F7FF60E, 0x3F7FF678, 0x3F7FF6DF, 0x3F7FF742,
 0x3F7FF7A1, 0x3F7FF7FE, 0x3F7FF857, 0x3F7FF8AC, 0x3F7FF8FF, 0x3F7FF94F, 0x3F7FF99C, 0x3F7FF9E6,
 0x3F7FFA2D, 0x3F7FFA72, 0x3F7FFAB4, 0x3F7FFAF3, 0x3F7FFB31, 0x3F7FFB6B, 0x3F7FFBA4, 0x3F7FFBDA,
 0x3F7FFC0E, 0x3F7FFC40, 0x3F7FFC70, 0x3F7FFC9E, 0x3F7FFCCA, 0x3F7FFCF5, 0x3F7FFD1D, 0x3F7FFD44,
 0x3F7FFD69, 0x3F7FFD8D, 0x3F7FFDAF, 0x3F7FFDD0, 0x3F7FFDEF, 0x3F7FFE0D, 0x3F7FFE29, 0x3F7FFE44,
 0x3F7FFE5E, 0x3F7FFE77, 0x3F7FFE8E, 0x3F7FFEA5, 0x3F7FFEBA, 0x3F7FFECE, 0x3F7FFEE2, 0x3F7FFEF4,
 0x3F7FFF05, 0x3F7FFF16, 0x3F7FFF26, 0x3F7FFF34, 0x3F7FFF42, 0x3F7FFF50, 0x3F7FFF5C, 0x3F7FFF68,
 0x3F7FFF73, 0x3F7FFF7E, 0x3F7FFF88, 0x3F7FFF91, 0x3F7FFF9A, 0x3F7FFFA3, 0x3F7FFFAA, 0x3F7FFFB2,
 0x3F7FFFB9, 0x3F7FFFBF, 0x3F7FFFC5, 0x3F7FFFCA, 0x3F7FFFD0, 0x3F7FFFD5, 0x3F7FFFD9, 0x3F7FFFDD,
 0x3F7FFFE1, 0x3F7FFFE5, 0x3F7FFFE8, 0x3F7FFFEB, 0x3F7FFFEE, 0x3F7FFFF0, 0x3F7FFFF3, 0x3F7FFFF5,
 0x3F7FFFF7, 0x3F7FFFF8, 0x3F7FFFFA, 0x3F7FFFFB, 0x3F7FFFFC, 0x3F7FFFFD, 0x3F7FFFFE, 0x3F7FFFFF
};

static const unsigned int kbd_short_128[ 128 ] __attribute__(   (  section( ".rodata" ), aligned( 16 )  )   ) = {
 0x3837B147, 0x38F8E089, 0x3971EC82, 0x39CC3228, 0x3A1ECF67, 0x3A69D4E7, 0x3AA5607B, 0x3AE2C6F4,
 0x3B17A699, 0x3B46AA6F, 0x3B7FB8C8, 0x3BA219F1, 0x3BCACAD0, 0x3BFAB72D, 0x3C1958D7, 0x3C39CB44,
 0x3C5F2531, 0x3C84EE44, 0x3C9D3507, 0x3CB8A44E, 0x3CD77A1A, 0x3CF9F48C, 0x3D1028AF, 0x3D2566AF,
 0x3D3CD1EE, 0x3D56870D, 0x3D72A159, 0x3D889D3F, 0x3D99351E, 0x3DAB230E, 0x3DBE7092, 0x3DD325C9,
 0x3DE9494A, 0x3E007004, 0x3E0CF697, 0x3E1A3907, 0x3E2836FF, 0x3E36EF28, 0x3E465F1F, 0x3E568370,
 0x3E675790, 0x3E78D5E2, 0x3E857BDA, 0x3E8EDAA1, 0x3E9882E4, 0x3EA26FBC, 0x3EAC9BCC, 0x3EB70144,
 0x3EC199EA, 0x3ECC5F23, 0x3ED749FE, 0x3EE25340, 0x3EED736E, 0x3EF8A2DE, 0x3F01ECE0, 0x3F078819,
 0x3F0D1F24, 0x3F12AE10, 0x3F1830F6, 0x3F1DA402, 0x3F23037A, 0x3F284BC5, 0x3F2D7972, 0x3F32893B,
 0x3F377811, 0x3F3C431C, 0x3F40E7C3, 0x3F4563AF, 0x3F49B4CF, 0x3F4DD95A, 0x3F51CFD3, 0x3F559709,
 0x3F592E18, 0x3F5C9469, 0x3F5FC9B2, 0x3F62CDF2, 0x3F65A171, 0x3F6844BD, 0x3F6AB8A3, 0x3F6CFE30,
 0x3F6F16A6, 0x3F71037D, 0x3F72C659, 0x3F746104, 0x3F75D56A, 0x3F772592, 0x3F785396, 0x3F7961A0,
 0x3F7A51DE, 0x3F7B2683, 0x3F7BE1BE, 0x3F7C85B2, 0x3F7D1477, 0x3F7D9013, 0x3F7DFA73, 0x3F7E5570,
 0x3F7EA2C3, 0x3F7EE40C, 0x3F7F1ACA, 0x3F7F485D, 0x3F7F6E07, 0x3F7F8CEB, 0x3F7FA60D, 0x3F7FBA54,
 0x3F7FCA8C, 0x3F7FD766, 0x3F7FE17C, 0x3F7FE953, 0x3F7FEF5A, 0x3F7FF3EE, 0x3F7FF75F, 0x3F7FF9EC,
 0x3F7FFBC9, 0x3F7FFD21, 0x3F7FFE15, 0x3F7FFEBF, 0x3F7FFF33, 0x3F7FFF80, 0x3F7FFFB3, 0x3F7FFFD3,
 0x3F7FFFE7, 0x3F7FFFF3, 0x3F7FFFF9, 0x3F7FFFFD, 0x3F7FFFFF, 0x3F800000, 0x3F800000, 0x3F800000
};

unsigned const int _aac_iq_table[ IQ_TABLE_SIZE ] __attribute__(   (  section( ".rodata" )  )   ) = {
 0x00000000, 0x3F800000, 0x40214518, 0x408A74BA, 0x40CB2FF5, 0x4108CC4F, 0x412E718E, 0x41563F90,
 0x41800000, 0x4195C41B, 0x41AC5AD3, 0x41C3B5D3, 0x41DBC8FF, 0x41F489EF, 0x4206F7CD, 0x4213F904,
 0x42214518, 0x422ED8DF, 0x423CB181, 0x424ACC6C, 0x42592746, 0x4267BFE8, 0x42769458, 0x4282D161,
 0x428A74BA, 0x4292336D, 0x429A0CBF, 0x42A20000, 0x42AA0C8A, 0x42B231C3, 0x42BA6F17, 0x42C2C3FE,
 0x42CB2FF5, 0x42D3B280, 0x42DC4B2A, 0x42E4F982, 0x42EDBD20, 0x42F6959C, 0x42FF8298, 0x430441DB,
 0x4308CC4F, 0x430D607D, 0x4311FE3D, 0x4316A568, 0x431B55D8, 0x43200F69, 0x4324D1F9, 0x43299D65,
 0x432E718E, 0x43334E55, 0x4338339A, 0x433D2142, 0x43421730, 0x43471549, 0x434C1B72, 0x43512992,
 0x43563F90, 0x435B5D54, 0x436082C7, 0x4365AFD1, 0x436AE45E, 0x43702057, 0x437563A8, 0x437AAE3C,
 0x43800000, 0x4382AC70, 0x43855C65, 0x43880FD6, 0x438AC6BA, 0x438D8108, 0x43903EB7, 0x4392FFC0,
 0x4395C41B, 0x43988BBF, 0x439B56A5, 0x439E24C5, 0x43A0F618, 0x43A3CA96, 0x43A6A239, 0x43A97CFA,
 0x43AC5AD3, 0x43AF3BBB, 0x43B21FAD, 0x43B506A4, 0x43B7F097, 0x43BADD82, 0x43BDCD5E, 0x43C0C025,
 0x43C3B5D3, 0x43C6AE60, 0x43C9A9C8, 0x43CCA806, 0x43CFA913, 0x43D2ACEA, 0x43D5B388, 0x43D8BCE5,
 0x43DBC8FF, 0x43DED7CE, 0x43E1E950, 0x43E4FD7F, 0x43E81456, 0x43EB2DD2, 0x43EE49ED, 0x43F168A3,
 0x43F489EF, 0x43F7ADCF, 0x43FAD43C, 0x43FDFD34, 0x44009459, 0x44022B59, 0x4403C399, 0x44055D15,
 0x4406F7CD, 0x440893BE, 0x440A30E6, 0x440BCF45, 0x440D6ED7, 0x440F0F9C, 0x4410B190, 0x441254B4,
 0x4413F904, 0x44159E80, 0x44174526, 0x4418ECF3, 0x441A95E7, 0x441C4000, 0x441DEB3C, 0x441F979A,
 0x44214518, 0x4422F3B4, 0x4424A36E, 0x44265444, 0x44280634, 0x4429B93D, 0x442B6D5D, 0x442D2294,
 0x442ED8DF, 0x4430903D, 0x443248AE, 0x4434022F, 0x4435BCBF, 0x4437785E, 0x44393509, 0x443AF2C0,
 0x443CB181, 0x443E714C, 0x4440321E, 0x4441F3F6, 0x4443B6D5, 0x44457AB7, 0x44473F9C, 0x44490584,
 0x444ACC6C, 0x444C9454, 0x444E5D3A, 0x4450271E, 0x4451F1FF, 0x4453BDDA, 0x44558AB0, 0x4457587F,
 0x44592746, 0x445AF704, 0x445CC7B8, 0x445E9961, 0x44606BFE, 0x44623F8F, 0x44641411, 0x4465E984,
 0x4467BFE8, 0x4469973A, 0x446B6F7B, 0x446D48AA, 0x446F22C4, 0x4470FDCA, 0x4472D9BB, 0x4474B695,
 0x44769458, 0x44787302, 0x447A5294, 0x447C330C, 0x447E1469, 0x447FF6AB, 0x4480ECE8, 0x4481DEEC,
 0x4482D161, 0x4483C446, 0x4484B79C, 0x4485AB61, 0x44869F96, 0x44879439, 0x4488894B, 0x44897ECC,
 0x448A74BA, 0x448B6B15, 0x448C61DE, 0x448D5913, 0x448E50B4, 0x448F48C2, 0x4490413A, 0x44913A1E,
 0x4492336D, 0x44932D27, 0x4494274A, 0x449521D7, 0x44961CCE, 0x4497182D, 0x449813F6, 0x44991027,
 0x449A0CBF, 0x449B09C0, 0x449C0728, 0x449D04F7, 0x449E032C, 0x449F01C9, 0x44A000CB, 0x44A10033,
 0x44A20000, 0x44A30033, 0x44A400CA, 0x44A501C6, 0x44A60326, 0x44A704EA, 0x44A80711, 0x44A9099C,
 0x44AA0C8A, 0x44AB0FDB, 0x44AC138E, 0x44AD17A3, 0x44AE1C1A, 0x44AF20F2, 0x44B0262C, 0x44B12BC7,
 0x44B231C3, 0x44B3381F, 0x44B43EDB, 0x44B545F7, 0x44B64D72, 0x44B7554D, 0x44B85D87, 0x44B96620,
 0x44BA6F17, 0x44BB786D, 0x44BC8221, 0x44BD8C32, 0x44BE96A1, 0x44BFA16D, 0x44C0AC96, 0x44C1B81C,
 0x44C2C3FE, 0x44C3D03D, 0x44C4DCD8, 0x44C5E9CE, 0x44C6F720, 0x44C804CD, 0x44C912D5, 0x44CA2138,
 0x44CB2FF5, 0x44CC3F0D, 0x44CD4E7F, 0x44CE5E4A, 0x44CF6E70, 0x44D07EEE, 0x44D18FC6, 0x44D2A0F7,
 0x44D3B280, 0x44D4C462, 0x44D5D69C, 0x44D6E92E, 0x44D7FC18, 0x44D90F5A, 0x44DA22F2, 0x44DB36E3,
 0x44DC4B2A, 0x44DD5FC7, 0x44DE74BC, 0x44DF8A06, 0x44E09FA7, 0x44E1B59D, 0x44E2CBE9, 0x44E3E28B,
 0x44E4F982, 0x44E610CE, 0x44E7286F, 0x44E84065, 0x44E958AF, 0x44EA714D, 0x44EB8A3F, 0x44ECA386,
 0x44EDBD20, 0x44EED70D, 0x44EFF14E, 0x44F10BE1, 0x44F226C8, 0x44F34202, 0x44F45D8E, 0x44F5796C,
 0x44F6959C, 0x44F7B21F, 0x44F8CEF3, 0x44F9EC19, 0x44FB0990, 0x44FC2759, 0x44FD4572, 0x44FE63DD,
 0x44FF8298, 0x450050D2, 0x4500E080, 0x45017056, 0x45020054, 0x4502907A, 0x450320C8, 0x4503B13E,
 0x450441DB, 0x4504D2A0, 0x4505638C, 0x4505F4A0, 0x450685DB, 0x4507173D, 0x4507A8C7, 0x45083A77,
 0x4508CC4F, 0x45095E4D, 0x4509F072, 0x450A82BE, 0x450B1531, 0x450BA7CA, 0x450C3A8A, 0x450CCD70,
 0x450D607D, 0x450DF3B0, 0x450E8709, 0x450F1A88, 0x450FAE2D, 0x451041F8, 0x4510D5EA, 0x45116A00,
 0x4511FE3D, 0x4512929F, 0x45132727, 0x4513BBD5, 0x451450A8, 0x4514E5A0, 0x45157ABD, 0x45161000,
 0x4516A568, 0x45173AF5, 0x4517D0A7, 0x4518667E, 0x4518FC7A, 0x4519929A, 0x451A28DF, 0x451ABF49,
 0x451B55D8, 0x451BEC8B, 0x451C8363, 0x451D1A5E, 0x451DB17F, 0x451E48C3, 0x451EE02C, 0x451F77B8,
 0x45200F69, 0x4520A73E, 0x45213F37, 0x4521D753, 0x45226F93, 0x452307F7, 0x4523A07F, 0x4524392A,
 0x4524D1F9, 0x45256AEB, 0x45260400, 0x45269D39, 0x45273695, 0x4527D015, 0x452869B7, 0x4529037D,
 0x45299D65, 0x452A3771, 0x452AD19F, 0x452B6BF0, 0x452C0664, 0x452CA0FB, 0x452D3BB4, 0x452DD690,
 0x452E718E, 0x452F0CAF, 0x452FA7F2, 0x45304358, 0x4530DEE0, 0x45317A8A, 0x45321656, 0x4532B244,
 0x45334E55, 0x4533EA87, 0x453486DB, 0x45352352, 0x4535BFE9, 0x45365CA3, 0x4536F97F, 0x4537967C,
 0x4538339A, 0x4538D0DB, 0x45396E3C, 0x453A0BBF, 0x453AA964, 0x453B472A, 0x453BE511, 0x453C8319,
 0x453D2142, 0x453DBF8D, 0x453E5DF8, 0x453EFC85, 0x453F9B32, 0x45403A01, 0x4540D8F0, 0x45417800,
 0x45421730, 0x4542B682, 0x454355F3, 0x4543F586, 0x45449539, 0x4545350C, 0x4545D500, 0x45467514,
 0x45471549, 0x4547B59E, 0x45485613, 0x4548F6A8, 0x4549975D, 0x454A3832, 0x454AD928, 0x454B7A3D,
 0x454C1B72, 0x454CBCC7, 0x454D5E3C, 0x454DFFD1, 0x454EA185, 0x454F4359, 0x454FE54C, 0x4550875F,
 0x45512992, 0x4551CBE4, 0x45526E56, 0x455310E7, 0x4553B397, 0x45545666, 0x4554F955, 0x45559C63,
 0x45563F90, 0x4556E2DC, 0x45578647, 0x455829D2, 0x4558CD7B, 0x45597143, 0x455A152A, 0x455AB930,
 0x455B5D54, 0x455C0197, 0x455CA5F9, 0x455D4A7A, 0x455DEF19, 0x455E93D7, 0x455F38B3, 0x455FDDAE,
 0x456082C7, 0x456127FE, 0x4561CD54, 0x456272C8, 0x4563185A, 0x4563BE0B, 0x456463DA, 0x456509C6,
 0x4565AFD1, 0x456655FA, 0x4566FC41, 0x4567A2A6, 0x45684929, 0x4568EFC9, 0x45699688, 0x456A3D64,
 0x456AE45E, 0x456B8B76, 0x456C32AB, 0x456CD9FE, 0x456D816E, 0x456E28FC, 0x456ED0A8, 0x456F7871,
 0x45702057, 0x4570C85B, 0x4571707C, 0x457218BA, 0x4572C116, 0x4573698F, 0x45741225, 0x4574BAD8,
 0x457563A8, 0x45760C95, 0x4576B59F, 0x45775EC7, 0x4578080B, 0x4578B16C, 0x45795AEA, 0x457A0485,
 0x457AAE3C, 0x457B5810, 0x457C0201, 0x457CAC0F, 0x457D5639, 0x457E0080, 0x457EAAE4, 0x457F5564,
 0x45800000, 0x4580555C, 0x4580AAC7, 0x45810040, 0x458155C7, 0x4581AB5C, 0x458200FF, 0x458256B1,
 0x4582AC70, 0x4583023E, 0x45835819, 0x4583AE03, 0x458403FB, 0x45845A00, 0x4584B014, 0x45850636,
 0x45855C65, 0x4585B2A3, 0x458608EE, 0x45865F48, 0x4586B5AF, 0x45870C24, 0x458762A7, 0x4587B938,
 0x45880FD6, 0x45886683, 0x4588BD3D, 0x45891405, 0x45896ADA, 0x4589C1BE, 0x458A18AF, 0x458A6FAD,
 0x458AC6BA, 0x458B1DD4, 0x458B74FC, 0x458BCC31, 0x458C2374, 0x458C7AC5, 0x458CD223, 0x458D298F,
 0x458D8108, 0x458DD88F, 0x458E3023, 0x458E87C5, 0x458EDF74, 0x458F3731, 0x458F8EFB, 0x458FE6D2,
 0x45903EB7, 0x459096AA, 0x4590EEAA, 0x459146B7, 0x45919ED1, 0x4591F6F9, 0x45924F2E, 0x4592A771,
 0x4592FFC0, 0x4593581D, 0x4593B088, 0x459408FF, 0x45946184, 0x4594BA16, 0x459512B5, 0x45956B61,
 0x4595C41B, 0x45961CE2, 0x459675B5, 0x4596CE96, 0x45972784, 0x4597807F, 0x4597D987, 0x4598329D,
 0x45988BBF, 0x4598E4EE, 0x45993E2A, 0x45999774, 0x4599F0CA, 0x459A4A2D, 0x459AA39D, 0x459AFD1B,
 0x459B56A5, 0x459BB03C, 0x459C09DF, 0x459C6390, 0x459CBD4E, 0x459D1718, 0x459D70F0, 0x459DCAD4,
 0x459E24C5, 0x459E7EC2, 0x459ED8CD, 0x459F32E4, 0x459F8D08, 0x459FE739, 0x45A04176, 0x45A09BC1,
 0x45A0F618, 0x45A1507B, 0x45A1AAEB, 0x45A20568, 0x45A25FF2, 0x45A2BA88, 0x45A3152B, 0x45A36FDA,
 0x45A3CA96, 0x45A4255F, 0x45A48034, 0x45A4DB15, 0x45A53604, 0x45A590FE, 0x45A5EC06, 0x45A64719,
 0x45A6A239, 0x45A6FD66, 0x45A7589F, 0x45A7B3E5, 0x45A80F37, 0x45A86A95, 0x45A8C600, 0x45A92177,
 0x45A97CFA, 0x45A9D88A, 0x45AA3427, 0x45AA8FCF, 0x45AAEB84, 0x45AB4745, 0x45ABA313, 0x45ABFEED,
 0x45AC5AD3, 0x45ACB6C5, 0x45AD12C3, 0x45AD6ECE, 0x45ADCAE5, 0x45AE2708, 0x45AE8338, 0x45AEDF73,
 0x45AF3BBB, 0x45AF980F, 0x45AFF46F, 0x45B050DB, 0x45B0AD53, 0x45B109D8, 0x45B16668, 0x45B1C305,
 0x45B21FAD, 0x45B27C62, 0x45B2D923, 0x45B335F0, 0x45B392C8, 0x45B3EFAD, 0x45B44C9E, 0x45B4A99B,
 0x45B506A4, 0x45B563B8, 0x45B5C0D9, 0x45B61E05, 0x45B67B3E, 0x45B6D882, 0x45B735D3, 0x45B7932F,
 0x45B7F097, 0x45B84E0B, 0x45B8AB8B, 0x45B90916, 0x45B966AE, 0x45B9C451, 0x45BA2200, 0x45BA7FBB,
 0x45BADD82, 0x45BB3B54, 0x45BB9932, 0x45BBF71C, 0x45BC5512, 0x45BCB313, 0x45BD1121, 0x45BD6F39,
 0x45BDCD5E, 0x45BE2B8E, 0x45BE89CA, 0x45BEE811, 0x45BF4665, 0x45BFA4C3, 0x45C0032E, 0x45C061A4,
 0x45C0C025, 0x45C11EB3, 0x45C17D4B, 0x45C1DBF0, 0x45C23AA0, 0x45C2995B, 0x45C2F822, 0x45C356F5,
 0x45C3B5D3, 0x45C414BC, 0x45C473B1, 0x45C4D2B2, 0x45C531BE, 0x45C590D5, 0x45C5EFF8, 0x45C64F27,
 0x45C6AE60, 0x45C70DA5, 0x45C76CF6, 0x45C7CC52, 0x45C82BB9, 0x45C88B2C, 0x45C8EAAA, 0x45C94A34,
 0x45C9A9C8, 0x45CA0968, 0x45CA6914, 0x45CAC8CB, 0x45CB288D, 0x45CB885A, 0x45CBE833, 0x45CC4816,
 0x45CCA806, 0x45CD0800, 0x45CD6806, 0x45CDC816, 0x45CE2833, 0x45CE885A, 0x45CEE88C, 0x45CF48CA,
 0x45CFA913, 0x45D00967, 0x45D069C6, 0x45D0CA30, 0x45D12AA6, 0x45D18B26, 0x45D1EBB2, 0x45D24C49,
 0x45D2ACEA, 0x45D30D97, 0x45D36E4F, 0x45D3CF13, 0x45D42FE1, 0x45D490BA, 0x45D4F19E, 0x45D5528D,
 0x45D5B388, 0x45D6148D, 0x45D6759D, 0x45D6D6B9, 0x45D737DF, 0x45D79910, 0x45D7FA4C, 0x45D85B93,
 0x45D8BCE5, 0x45D91E42, 0x45D97FAA, 0x45D9E11D, 0x45DA429B, 0x45DAA423, 0x45DB05B7, 0x45DB6755,
 0x45DBC8FF, 0x45DC2AB3, 0x45DC8C72, 0x45DCEE3B, 0x45DD5010, 0x45DDB1EF, 0x45DE13DA, 0x45DE75CF,
 0x45DED7CE, 0x45DF39D9, 0x45DF9BEE, 0x45DFFE0E, 0x45E06039, 0x45E0C26F, 0x45E124AF, 0x45E186FA,
 0x45E1E950, 0x45E24BB1, 0x45E2AE1C, 0x45E31092, 0x45E37312, 0x45E3D59D, 0x45E43833, 0x45E49AD4,
 0x45E4FD7F, 0x45E56035, 0x45E5C2F5, 0x45E625C0, 0x45E68896, 0x45E6EB76, 0x45E74E61, 0x45E7B156,
 0x45E81456, 0x45E87761, 0x45E8DA76, 0x45E93D96, 0x45E9A0C0, 0x45EA03F4, 0x45EA6734, 0x45EACA7D,
 0x45EB2DD2, 0x45EB9130, 0x45EBF49A, 0x45EC580D, 0x45ECBB8B, 0x45ED1F14, 0x45ED82A7, 0x45EDE645,
 0x45EE49ED, 0x45EEAD9F, 0x45EF115C, 0x45EF7523, 0x45EFD8F4, 0x45F03CD0, 0x45F0A0B7, 0x45F104A7,
 0x45F168A3, 0x45F1CCA8, 0x45F230B8, 0x45F294D2, 0x45F2F8F6, 0x45F35D25, 0x45F3C15E, 0x45F425A2,
 0x45F489EF, 0x45F4EE47, 0x45F552AA, 0x45F5B716, 0x45F61B8D, 0x45F6800E, 0x45F6E499, 0x45F7492F,
 0x45F7ADCF, 0x45F81279, 0x45F8772D, 0x45F8DBEB, 0x45F940B4, 0x45F9A587, 0x45FA0A64, 0x45FA6F4B,
 0x45FAD43C, 0x45FB3938, 0x45FB9E3D, 0x45FC034D, 0x45FC6867, 0x45FCCD8B, 0x45FD32B9, 0x45FD97F2,
 0x45FDFD34, 0x45FE6281, 0x45FEC7D7, 0x45FF2D38, 0x45FF92A3, 0x45FFF818, 0x46002ECB, 0x46006190,
 0x46009459, 0x4600C728, 0x4600F9FB, 0x46012CD4, 0x46015FB1, 0x46019294, 0x4601C57B, 0x4601F868,
 0x46022B59, 0x46025E50, 0x4602914B, 0x4602C44C, 0x4602F751, 0x46032A5C, 0x46035D6B, 0x4603907F,
 0x4603C399, 0x4603F6B7, 0x460429DA, 0x46045D02, 0x4604902F, 0x4604C361, 0x4604F698, 0x460529D4,
 0x46055D15, 0x4605905B, 0x4605C3A5, 0x4605F6F5, 0x46062A4A, 0x46065DA3, 0x46069101, 0x4606C465,
 0x4606F7CD, 0x46072B3A, 0x46075EAC, 0x46079222, 0x4607C59E, 0x4607F91F, 0x46082CA4, 0x4608602F,
 0x460893BE, 0x4608C752, 0x4608FAEB, 0x46092E89, 0x4609622B, 0x460995D3, 0x4609C97F, 0x4609FD30,
 0x460A30E6, 0x460A64A1, 0x460A9861, 0x460ACC26, 0x460AFFEF, 0x460B33BD, 0x460B6790, 0x460B9B68,
 0x460BCF45, 0x460C0326, 0x460C370D, 0x460C6AF8, 0x460C9EE8, 0x460CD2DC, 0x460D06D6, 0x460D3AD4,
 0x460D6ED7, 0x460DA2DF, 0x460DD6EC, 0x460E0AFD, 0x460E3F13, 0x460E732E, 0x460EA74E, 0x460EDB72,
 0x460F0F9C, 0x460F43CA, 0x460F77FC, 0x460FAC34, 0x460FE070, 0x461014B1, 0x461048F7, 0x46107D41,
 0x4610B190, 0x4610E5E4, 0x46111A3D, 0x46114E9A, 0x461182FC, 0x4611B763, 0x4611EBCF, 0x4612203F,
 0x461254B4, 0x4612892E, 0x4612BDAC, 0x4612F22F, 0x461326B7, 0x46135B43, 0x46138FD4, 0x4613C46A,
 0x4613F904, 0x46142DA4, 0x46146247, 0x461496F0, 0x4614CB9D, 0x4615004F, 0x46153505, 0x461569C1,
 0x46159E80, 0x4615D345, 0x4616080E, 0x46163CDC, 0x461671AE, 0x4616A685, 0x4616DB61, 0x46171041,
 0x46174526, 0x46177A0F, 0x4617AEFD, 0x4617E3F0, 0x461818E8, 0x46184DE4, 0x461882E4, 0x4618B7E9,
 0x4618ECF3, 0x46192202, 0x46195715, 0x46198C2C, 0x4619C149, 0x4619F669, 0x461A2B8F, 0x461A60B9,
 0x461A95E7, 0x461ACB1A, 0x461B0052, 0x461B358E, 0x461B6ACF, 0x461BA014, 0x461BD55E, 0x461C0AAD,
 0x461C4000, 0x461C7558, 0x461CAAB4, 0x461CE014, 0x461D157A, 0x461D4AE3, 0x461D8052, 0x461DB5C5,
 0x461DEB3C, 0x461E20B8, 0x461E5638, 0x461E8BBD, 0x461EC147, 0x461EF6D5, 0x461F2C67, 0x461F61FE,
 0x461F979A, 0x461FCD3A, 0x462002DE, 0x46203887, 0x46206E35, 0x4620A3E7, 0x4620D99D, 0x46210F58,
 0x46214518, 0x46217ADC, 0x4621B0A4, 0x4621E671, 0x46221C42, 0x46225218, 0x462287F3, 0x4622BDD1,
 0x4622F3B4, 0x4623299C, 0x46235F88, 0x46239579, 0x4623CB6E, 0x46240167, 0x46243765, 0x46246D68,
 0x4624A36E, 0x4624D97A, 0x46250F89, 0x4625459D, 0x46257BB6, 0x4625B1D3, 0x4625E7F4, 0x46261E1A,
 0x46265444, 0x46268A73, 0x4626C0A6, 0x4626F6DD, 0x46272D19, 0x46276359, 0x4627999E, 0x4627CFE7,
 0x46280634, 0x46283C86, 0x462872DC, 0x4628A936, 0x4628DF95, 0x462915F9, 0x46294C60, 0x462982CC,
 0x4629B93D, 0x4629EFB2, 0x462A262B, 0x462A5CA8, 0x462A932A, 0x462AC9B0, 0x462B003B, 0x462B36CA,
 0x462B6D5D, 0x462BA3F5, 0x462BDA91, 0x462C1131, 0x462C47D6, 0x462C7E7F, 0x462CB52C, 0x462CEBDE,
 0x462D2294, 0x462D594E, 0x462D900C, 0x462DC6CF, 0x462DFD97, 0x462E3462, 0x462E6B32, 0x462EA206,
 0x462ED8DF, 0x462F0FBB, 0x462F469D, 0x462F7D82, 0x462FB46C, 0x462FEB5A, 0x4630224C, 0x46305942,
 0x4630903D, 0x4630C73C, 0x4630FE40, 0x46313547, 0x46316C53, 0x4631A363, 0x4631DA78, 0x46321191,
 0x463248AE, 0x46327FCF, 0x4632B6F4, 0x4632EE1E, 0x4633254C, 0x46335C7E, 0x463393B5, 0x4633CAF0,
 0x4634022F, 0x46343972, 0x463470B9, 0x4634A805, 0x4634DF55, 0x463516A9, 0x46354E02, 0x4635855E,
 0x4635BCBF, 0x4635F424, 0x46362B8E, 0x463662FB, 0x46369A6D, 0x4636D1E3, 0x4637095D, 0x463740DB,
 0x4637785E, 0x4637AFE5, 0x4637E770, 0x46381EFF, 0x46385692, 0x46388E2A, 0x4638C5C5, 0x4638FD65,
 0x46393509, 0x46396CB2, 0x4639A45E, 0x4639DC0F, 0x463A13C3, 0x463A4B7C, 0x463A8339, 0x463ABAFB,
 0x463AF2C0, 0x463B2A8A, 0x463B6258, 0x463B9A29, 0x463BD200, 0x463C09DA, 0x463C41B8, 0x463C799B,
 0x463CB181, 0x463CE96C, 0x463D215B, 0x463D594E, 0x463D9145, 0x463DC941, 0x463E0140, 0x463E3944,
 0x463E714C, 0x463EA957, 0x463EE167, 0x463F197C, 0x463F5194, 0x463F89B0, 0x463FC1D1, 0x463FF9F5,
 0x4640321E, 0x46406A4A, 0x4640A27B, 0x4640DAB0, 0x464112E9, 0x46414B26, 0x46418368, 0x4641BBAD,
 0x4641F3F6, 0x46422C44, 0x46426496, 0x46429CEB, 0x4642D545, 0x46430DA3, 0x46434605, 0x46437E6B,
 0x4643B6D5, 0x4643EF43, 0x464427B5, 0x4644602B, 0x464498A5, 0x4644D124, 0x464509A6, 0x4645422D,
 0x46457AB7, 0x4645B346, 0x4645EBD8, 0x4646246F, 0x46465D09, 0x464695A8, 0x4646CE4B, 0x464706F2,
 0x46473F9C, 0x4647784B, 0x4647B0FE, 0x4647E9B5, 0x46482270, 0x46485B2F, 0x464893F2, 0x4648CCB9,
 0x46490584, 0x46493E53, 0x46497726, 0x4649AFFD, 0x4649E8D8, 0x464A21B7, 0x464A5A9A, 0x464A9381,
 0x464ACC6C, 0x464B055B, 0x464B3E4E, 0x464B7745, 0x464BB040, 0x464BE93F, 0x464C2242, 0x464C5B49,
 0x464C9454, 0x464CCD63, 0x464D0676, 0x464D3F8D, 0x464D78A8, 0x464DB1C6, 0x464DEAE9, 0x464E2410,
 0x464E5D3A, 0x464E9669, 0x464ECF9C, 0x464F08D2, 0x464F420D, 0x464F7B4B, 0x464FB48E, 0x464FEDD4,
 0x4650271E, 0x4650606D, 0x465099BF, 0x4650D315, 0x46510C6F, 0x465145CD, 0x46517F2F, 0x4651B895,
 0x4651F1FF, 0x46522B6C, 0x465264DE, 0x46529E54, 0x4652D7CD, 0x4653114A, 0x46534ACC, 0x46538451,
 0x4653BDDA, 0x4653F767, 0x465430F8, 0x46546A8D, 0x4654A426, 0x4654DDC3, 0x46551763, 0x46555108,
 0x46558AB0, 0x4655C45C, 0x4655FE0C, 0x465637C0, 0x46567178, 0x4656AB34, 0x4656E4F4, 0x46571EB7,
 0x4657587F, 0x4657924A, 0x4657CC19, 0x465805EC, 0x46583FC3, 0x4658799E, 0x4658B37D, 0x4658ED5F,
 0x46592746, 0x46596130, 0x46599B1E, 0x4659D510, 0x465A0F06, 0x465A4900, 0x465A82FD, 0x465ABCFF,
 0x465AF704, 0x465B310D, 0x465B6B1A, 0x465BA52B, 0x465BDF3F, 0x465C1958, 0x465C5374, 0x465C8D94,
 0x465CC7B8, 0x465D01E0, 0x465D3C0B, 0x465D763B, 0x465DB06E, 0x465DEAA5, 0x465E24E0, 0x465E5F1F,
 0x465E9961, 0x465ED3A8, 0x465F0DF2, 0x465F4840, 0x465F8291, 0x465FBCE7, 0x465FF740, 0x4660319D,
 0x46606BFE, 0x4660A663, 0x4660E0CC, 0x46611B38, 0x466155A8, 0x4661901C, 0x4661CA94, 0x4662050F,
 0x46623F8F, 0x46627A12, 0x4662B499, 0x4662EF23, 0x466329B2, 0x46636444, 0x46639EDA, 0x4663D973,
 0x46641411, 0x46644EB2, 0x46648957, 0x4664C400, 0x4664FEAD, 0x4665395D, 0x46657411, 0x4665AEC9,
 0x4665E984, 0x46662444, 0x46665F07, 0x466699CE, 0x4666D498, 0x46670F66, 0x46674A38, 0x4667850E,
 0x4667BFE8, 0x4667FAC5, 0x466835A6, 0x4668708B, 0x4668AB73, 0x4668E660, 0x4669214F, 0x46695C43,
 0x4669973A, 0x4669D236, 0x466A0D34, 0x466A4837, 0x466A833D, 0x466ABE47, 0x466AF955, 0x466B3466,
 0x466B6F7B, 0x466BAA94, 0x466BE5B1, 0x466C20D1, 0x466C5BF5, 0x466C971C, 0x466CD248, 0x466D0D77,
 0x466D48AA, 0x466D83E0, 0x466DBF1A, 0x466DFA58, 0x466E3599, 0x466E70DE, 0x466EAC27, 0x466EE774,
 0x466F22C4, 0x466F5E18, 0x466F9970, 0x466FD4CB, 0x4670102A, 0x46704B8C, 0x467086F3, 0x4670C25C,
 0x4670FDCA, 0x4671393B, 0x467174B0, 0x4671B029, 0x4671EBA5, 0x46722725, 0x467262A8, 0x46729E30,
 0x4672D9BB, 0x46731549, 0x467350DB, 0x46738C71, 0x4673C80A, 0x467403A8, 0x46743F48, 0x46747AED,
 0x4674B695, 0x4674F240, 0x46752DF0, 0x467569A3, 0x4675A559, 0x4675E113, 0x46761CD1, 0x46765893,
 0x46769458, 0x4676D020, 0x46770BED, 0x467747BC, 0x46778390, 0x4677BF67, 0x4677FB42, 0x46783720,
 0x46787302, 0x4678AEE8, 0x4678EAD1, 0x467926BE, 0x467962AE, 0x46799EA2, 0x4679DA9A, 0x467A1695,
 0x467A5294, 0x467A8E96, 0x467ACA9C, 0x467B06A6, 0x467B42B3, 0x467B7EC4, 0x467BBAD8, 0x467BF6F0,
 0x467C330C, 0x467C6F2B, 0x467CAB4E, 0x467CE774, 0x467D239E, 0x467D5FCB, 0x467D9BFC, 0x467DD831,
 0x467E1469, 0x467E50A5, 0x467E8CE4, 0x467EC927, 0x467F056D, 0x467F41B7, 0x467F7E05, 0x467FBA56,
 0x467FF6AB, 0x46801981, 0x468037AF, 0x468055DF, 0x46807410, 0x46809244, 0x4680B079, 0x4680CEAF,
 0x4680ECE8, 0x46810B22, 0x4681295E, 0x4681479C, 0x468165DC, 0x4681841D, 0x4681A260, 0x4681C0A5,
 0x4681DEEC, 0x4681FD34, 0x46821B7E, 0x468239CA, 0x46825818, 0x46827668, 0x468294B9, 0x4682B30C,
 0x4682D161, 0x4682EFB7, 0x46830E0F, 0x46832C6A, 0x46834AC5, 0x46836923, 0x46838782, 0x4683A5E3,
 0x4683C446, 0x4683E2AB, 0x46840111, 0x46841F79, 0x46843DE3, 0x46845C4E, 0x46847ABC, 0x4684992B,
 0x4684B79C, 0x4684D60E, 0x4684F483, 0x468512F9, 0x46853170, 0x46854FEA, 0x46856E65, 0x46858CE2,
 0x4685AB61, 0x4685C9E1, 0x4685E864, 0x468606E8, 0x4686256D, 0x468643F5, 0x4686627E, 0x46868109,
 0x46869F96, 0x4686BE24, 0x4686DCB4, 0x4686FB46, 0x468719DA, 0x4687386F, 0x46875706, 0x4687759F,
 0x46879439, 0x4687B2D5, 0x4687D173, 0x4687F013, 0x46880EB4, 0x46882D58, 0x46884BFC, 0x46886AA3,
 0x4688894B, 0x4688A7F5, 0x4688C6A1, 0x4688E54F, 0x468903FE, 0x468922AF, 0x46894161, 0x46896016,
 0x46897ECC, 0x46899D83, 0x4689BC3D, 0x4689DAF8, 0x4689F9B5, 0x468A1874, 0x468A3734, 0x468A55F6,
 0x468A74BA, 0x468A937F, 0x468AB246, 0x468AD10F, 0x468AEFDA, 0x468B0EA6, 0x468B2D74, 0x468B4C44,
 0x468B6B15, 0x468B89E8, 0x468BA8BD, 0x468BC794, 0x468BE66C, 0x468C0546, 0x468C2421, 0x468C42FF,
 0x468C61DE, 0x468C80BE, 0x468C9FA1, 0x468CBE85, 0x468CDD6B, 0x468CFC52, 0x468D1B3B, 0x468D3A26,
 0x468D5913, 0x468D7801, 0x468D96F1, 0x468DB5E3, 0x468DD4D6, 0x468DF3CB, 0x468E12C2, 0x468E31BA,
 0x468E50B4, 0x468E6FB0, 0x468E8EAD, 0x468EADAD, 0x468ECCAD, 0x468EEBB0, 0x468F0AB4, 0x468F29BA,
 0x468F48C2, 0x468F67CB, 0x468F86D6, 0x468FA5E2, 0x468FC4F1, 0x468FE401, 0x46900312, 0x46902225,
 0x4690413A, 0x46906051, 0x46907F69, 0x46909E83, 0x4690BD9F, 0x4690DCBC, 0x4690FBDB, 0x46911AFC,
 0x46913A1E, 0x46915942, 0x46917868, 0x46919790, 0x4691B6B9, 0x4691D5E3, 0x4691F510, 0x4692143E,
 0x4692336D, 0x4692529F, 0x469271D2, 0x46929106, 0x4692B03D, 0x4692CF75, 0x4692EEAE, 0x46930DEA,
 0x46932D27, 0x46934C65, 0x46936BA6, 0x46938AE8, 0x4693AA2B, 0x4693C970, 0x4693E8B7, 0x46940800,
 0x4694274A, 0x46944696, 0x469465E3, 0x46948533, 0x4694A483, 0x4694C3D6, 0x4694E32A, 0x46950280,
 0x469521D7, 0x46954130, 0x4695608B, 0x46957FE7, 0x46959F45, 0x4695BEA5, 0x4695DE06, 0x4695FD69,
 0x46961CCE, 0x46963C34, 0x46965B9C, 0x46967B05, 0x46969A71, 0x4696B9DD, 0x4696D94C, 0x4696F8BC,
 0x4697182D, 0x469737A1, 0x46975716, 0x4697768C, 0x46979605, 0x4697B57E, 0x4697D4FA, 0x4697F477,
 0x469813F6, 0x46983376, 0x469852F8, 0x4698727C, 0x46989201, 0x4698B188, 0x4698D111, 0x4698F09B,
 0x46991027, 0x46992FB4, 0x46994F43, 0x46996ED4, 0x46998E66, 0x4699ADFA, 0x4699CD8F, 0x4699ED27,
 0x469A0CBF, 0x469A2C5A, 0x469A4BF6, 0x469A6B93, 0x469A8B33, 0x469AAAD4, 0x469ACA76, 0x469AEA1A,
 0x469B09C0, 0x469B2967, 0x469B4910, 0x469B68BB, 0x469B8867, 0x469BA815, 0x469BC7C4, 0x469BE775,
 0x469C0728, 0x469C26DC, 0x469C4692, 0x469C6649, 0x469C8602, 0x469CA5BD, 0x469CC579, 0x469CE537,
 0x469D04F7, 0x469D24B8, 0x469D447B, 0x469D643F, 0x469D8405, 0x469DA3CC, 0x469DC395, 0x469DE360,
 0x469E032C, 0x469E22FA, 0x469E42CA, 0x469E629B, 0x469E826E, 0x469EA242, 0x469EC218, 0x469EE1EF,
 0x469F01C9, 0x469F21A3, 0x469F4180, 0x469F615D, 0x469F813D, 0x469FA11E, 0x469FC101, 0x469FE0E5,
 0x46A000CB, 0x46A020B2, 0x46A0409B, 0x46A06086, 0x46A08072, 0x46A0A060, 0x46A0C04F, 0x46A0E040,
 0x46A10033, 0x46A12027, 0x46A1401C, 0x46A16014, 0x46A1800D, 0x46A1A007, 0x46A1C003, 0x46A1E001,
 0x46A20000, 0x46A22001, 0x46A24003, 0x46A26007, 0x46A2800D, 0x46A2A014, 0x46A2C01C, 0x46A2E027,
 0x46A30033, 0x46A32040, 0x46A3404F, 0x46A3605F, 0x46A38072, 0x46A3A085, 0x46A3C09B, 0x46A3E0B1,
 0x46A400CA, 0x46A420E4, 0x46A440FF, 0x46A4611D, 0x46A4813B, 0x46A4A15C, 0x46A4C17D, 0x46A4E1A1,
 0x46A501C6, 0x46A521EC, 0x46A54214, 0x46A5623E, 0x46A58269, 0x46A5A296, 0x46A5C2C4, 0x46A5E2F4,
 0x46A60326, 0x46A62359, 0x46A6438D, 0x46A663C4, 0x46A683FB, 0x46A6A435, 0x46A6C46F, 0x46A6E4AC,
 0x46A704EA, 0x46A72529, 0x46A7456A, 0x46A765AD, 0x46A785F1, 0x46A7A637, 0x46A7C67E, 0x46A7E6C7,
 0x46A80711, 0x46A8275D, 0x46A847AB, 0x46A867FA, 0x46A8884A, 0x46A8A89D, 0x46A8C8F0, 0x46A8E945,
 0x46A9099C, 0x46A929F5, 0x46A94A4E, 0x46A96AAA, 0x46A98B07, 0x46A9AB65, 0x46A9CBC5, 0x46A9EC27,
 0x46AA0C8A, 0x46AA2CEF, 0x46AA4D55, 0x46AA6DBD, 0x46AA8E26, 0x46AAAE91, 0x46AACEFD, 0x46AAEF6B,
 0x46AB0FDB, 0x46AB304C, 0x46AB50BE, 0x46AB7132, 0x46AB91A8, 0x46ABB21F, 0x46ABD298, 0x46ABF312,
 0x46AC138E, 0x46AC340B, 0x46AC548A, 0x46AC750A, 0x46AC958C, 0x46ACB60F, 0x46ACD694, 0x46ACF71B,
 0x46AD17A3, 0x46AD382C, 0x46AD58B7, 0x46AD7944, 0x46AD99D2, 0x46ADBA62, 0x46ADDAF3, 0x46ADFB86,
 0x46AE1C1A, 0x46AE3CB0, 0x46AE5D47, 0x46AE7DE0, 0x46AE9E7A, 0x46AEBF16, 0x46AEDFB3, 0x46AF0052,
 0x46AF20F2, 0x46AF4194, 0x46AF6238, 0x46AF82DD, 0x46AFA383, 0x46AFC42B, 0x46AFE4D5, 0x46B00580,
 0x46B0262C, 0x46B046DA, 0x46B0678A, 0x46B0883B, 0x46B0A8EE, 0x46B0C9A2, 0x46B0EA57, 0x46B10B0E,
 0x46B12BC7, 0x46B14C81, 0x46B16D3D, 0x46B18DFA, 0x46B1AEB9, 0x46B1CF79, 0x46B1F03B, 0x46B210FE,
 0x46B231C3, 0x46B25289, 0x46B27351, 0x46B2941A, 0x46B2B4E5, 0x46B2D5B1, 0x46B2F67F, 0x46B3174E,
 0x46B3381F, 0x46B358F1, 0x46B379C5, 0x46B39A9A, 0x46B3BB71, 0x46B3DC49, 0x46B3FD23, 0x46B41DFE,
 0x46B43EDB, 0x46B45FB9, 0x46B48099, 0x46B4A17A, 0x46B4C25D, 0x46B4E341, 0x46B50427, 0x46B5250E,
 0x46B545F7, 0x46B566E1, 0x46B587CD, 0x46B5A8BA, 0x46B5C9A9, 0x46B5EA99, 0x46B60B8A, 0x46B62C7E,
 0x46B64D72, 0x46B66E68, 0x46B68F60, 0x46B6B059, 0x46B6D154, 0x46B6F250, 0x46B7134E, 0x46B7344D,
 0x46B7554D, 0x46B7764F, 0x46B79753, 0x46B7B858, 0x46B7D95E, 0x46B7FA66, 0x46B81B70, 0x46B83C7B,
 0x46B85D87, 0x46B87E95, 0x46B89FA5, 0x46B8C0B5, 0x46B8E1C8, 0x46B902DC, 0x46B923F1, 0x46B94508,
 0x46B96620, 0x46B9873A, 0x46B9A855, 0x46B9C972, 0x46B9EA90, 0x46BA0BB0, 0x46BA2CD1, 0x46BA4DF3,
 0x46BA6F17, 0x46BA903D, 0x46BAB164, 0x46BAD28C, 0x46BAF3B6, 0x46BB14E2, 0x46BB360F, 0x46BB573D,
 0x46BB786D, 0x46BB999E, 0x46BBBAD1, 0x46BBDC05, 0x46BBFD3B, 0x46BC1E72, 0x46BC3FAB, 0x46BC60E5,
 0x46BC8221, 0x46BCA35E, 0x46BCC49C, 0x46BCE5DC, 0x46BD071E, 0x46BD2861, 0x46BD49A5, 0x46BD6AEB,
 0x46BD8C32, 0x46BDAD7B, 0x46BDCEC5, 0x46BDF011, 0x46BE115E, 0x46BE32AC, 0x46BE53FC, 0x46BE754E,
 0x46BE96A1, 0x46BEB7F5, 0x46BED94B, 0x46BEFAA3, 0x46BF1BFB, 0x46BF3D56, 0x46BF5EB1, 0x46BF800E,
 0x46BFA16D, 0x46BFC2CD, 0x46BFE42F, 0x46C00592, 0x46C026F6, 0x46C0485C, 0x46C069C3, 0x46C08B2C,
 0x46C0AC96, 0x46C0CE02, 0x46C0EF6F, 0x46C110DE, 0x46C1324E, 0x46C153BF, 0x46C17532, 0x46C196A6,
 0x46C1B81C, 0x46C1D993, 0x46C1FB0C, 0x46C21C86, 0x46C23E02, 0x46C25F7F, 0x46C280FD, 0x46C2A27D,
 0x46C2C3FE, 0x46C2E581, 0x46C30705, 0x46C3288B, 0x46C34A12, 0x46C36B9B, 0x46C38D25, 0x46C3AEB0,
 0x46C3D03D, 0x46C3F1CB, 0x46C4135B, 0x46C434EC, 0x46C4567F, 0x46C47813, 0x46C499A8, 0x46C4BB3F,
 0x46C4DCD8, 0x46C4FE71, 0x46C5200D, 0x46C541A9, 0x46C56347, 0x46C584E7, 0x46C5A688, 0x46C5C82A,
 0x46C5E9CE, 0x46C60B73, 0x46C62D1A, 0x46C64EC2, 0x46C6706B, 0x46C69216, 0x46C6B3C3, 0x46C6D570,
 0x46C6F720, 0x46C718D0, 0x46C73A82, 0x46C75C36, 0x46C77DEB, 0x46C79FA1, 0x46C7C159, 0x46C7E312,
 0x46C804CD, 0x46C82689, 0x46C84846, 0x46C86A05, 0x46C88BC5, 0x46C8AD87, 0x46C8CF4A, 0x46C8F10F,
 0x46C912D5, 0x46C9349C, 0x46C95665, 0x46C9782F, 0x46C999FB, 0x46C9BBC8, 0x46C9DD97, 0x46C9FF66,
 0x46CA2138, 0x46CA430A, 0x46CA64DF, 0x46CA86B4, 0x46CAA88B, 0x46CACA64, 0x46CAEC3D, 0x46CB0E19,
 0x46CB2FF5, 0x46CB51D3, 0x46CB73B3, 0x46CB9593, 0x46CBB776, 0x46CBD959, 0x46CBFB3F, 0x46CC1D25,
 0x46CC3F0D, 0x46CC60F6, 0x46CC82E1, 0x46CCA4CD, 0x46CCC6BB, 0x46CCE8A9, 0x46CD0A9A, 0x46CD2C8C,
 0x46CD4E7F, 0x46CD7073, 0x46CD9269, 0x46CDB461, 0x46CDD659, 0x46CDF853, 0x46CE1A4F, 0x46CE3C4C,
 0x46CE5E4A, 0x46CE804A, 0x46CEA24B, 0x46CEC44E, 0x46CEE652, 0x46CF0857, 0x46CF2A5E, 0x46CF4C66,
 0x46CF6E70, 0x46CF907B, 0x46CFB287, 0x46CFD495, 0x46CFF6A4, 0x46D018B4, 0x46D03AC6, 0x46D05CD9,
 0x46D07EEE, 0x46D0A104, 0x46D0C31C, 0x46D0E535, 0x46D1074F, 0x46D1296B, 0x46D14B88, 0x46D16DA6,
 0x46D18FC6, 0x46D1B1E7, 0x46D1D40A, 0x46D1F62E, 0x46D21853, 0x46D23A7A, 0x46D25CA2, 0x46D27ECC,
 0x46D2A0F7, 0x46D2C323, 0x46D2E551, 0x46D30780, 0x46D329B0, 0x46D34BE2, 0x46D36E15, 0x46D3904A,
 0x46D3B280, 0x46D3D4B7, 0x46D3F6F0, 0x46D4192A, 0x46D43B66, 0x46D45DA3, 0x46D47FE1, 0x46D4A221,
 0x46D4C462, 0x46D4E6A4, 0x46D508E8, 0x46D52B2D, 0x46D54D74, 0x46D56FBC, 0x46D59205, 0x46D5B450,
 0x46D5D69C, 0x46D5F8E9, 0x46D61B38, 0x46D63D88, 0x46D65FDA, 0x46D6822D, 0x46D6A481, 0x46D6C6D7,
 0x46D6E92E, 0x46D70B87, 0x46D72DE0, 0x46D7503C, 0x46D77298, 0x46D794F6, 0x46D7B755, 0x46D7D9B6,
 0x46D7FC18, 0x46D81E7B, 0x46D840E0, 0x46D86346, 0x46D885AE, 0x46D8A817, 0x46D8CA81, 0x46D8ECED,
 0x46D90F5A, 0x46D931C8, 0x46D95438, 0x46D976A9, 0x46D9991B, 0x46D9BB8F, 0x46D9DE04, 0x46DA007B,
 0x46DA22F2, 0x46DA456C, 0x46DA67E6, 0x46DA8A62, 0x46DAACE0, 0x46DACF5E, 0x46DAF1DE, 0x46DB1460,
 0x46DB36E3, 0x46DB5967, 0x46DB7BEC, 0x46DB9E73, 0x46DBC0FB, 0x46DBE385, 0x46DC0610, 0x46DC289C,
 0x46DC4B2A, 0x46DC6DB9, 0x46DC9049, 0x46DCB2DB, 0x46DCD56E, 0x46DCF802, 0x46DD1A98, 0x46DD3D2F,
 0x46DD5FC7, 0x46DD8261, 0x46DDA4FC, 0x46DDC799, 0x46DDEA37, 0x46DE0CD6, 0x46DE2F76, 0x46DE5218,
 0x46DE74BC, 0x46DE9760, 0x46DEBA06, 0x46DEDCAD, 0x46DEFF56, 0x46DF2200, 0x46DF44AB, 0x46DF6758,
 0x46DF8A06, 0x46DFACB5, 0x46DFCF66, 0x46DFF218, 0x46E014CC, 0x46E03780, 0x46E05A36, 0x46E07CEE,
 0x46E09FA7, 0x46E0C261, 0x46E0E51C, 0x46E107D9, 0x46E12A97, 0x46E14D57, 0x46E17018, 0x46E192DA,
 0x46E1B59D, 0x46E1D862, 0x46E1FB28, 0x46E21DF0, 0x46E240B9, 0x46E26383, 0x46E2864E, 0x46E2A91B,
 0x46E2CBE9, 0x46E2EEB9, 0x46E3118A, 0x46E3345C, 0x46E35730, 0x46E37A05, 0x46E39CDB, 0x46E3BFB2,
 0x46E3E28B, 0x46E40565, 0x46E42841, 0x46E44B1E, 0x46E46DFC, 0x46E490DC, 0x46E4B3BC, 0x46E4D69F,
 0x46E4F982, 0x46E51C67, 0x46E53F4D, 0x46E56235, 0x46E5851E, 0x46E5A808, 0x46E5CAF3, 0x46E5EDE0,
 0x46E610CE, 0x46E633BE, 0x46E656AE, 0x46E679A1, 0x46E69C94, 0x46E6BF89, 0x46E6E27F, 0x46E70576,
 0x46E7286F, 0x46E74B69, 0x46E76E65, 0x46E79161, 0x46E7B45F, 0x46E7D75F, 0x46E7FA5F, 0x46E81D61,
 0x46E84065, 0x46E86369, 0x46E8866F, 0x46E8A976, 0x46E8CC7F, 0x46E8EF89, 0x46E91294, 0x46E935A1,
 0x46E958AF, 0x46E97BBE, 0x46E99ECE, 0x46E9C1E0, 0x46E9E4F3, 0x46EA0808, 0x46EA2B1D, 0x46EA4E35,
 0x46EA714D, 0x46EA9467, 0x46EAB782, 0x46EADA9E, 0x46EAFDBC, 0x46EB20DB, 0x46EB43FB, 0x46EB671C,
 0x46EB8A3F, 0x46EBAD64, 0x46EBD089, 0x46EBF3B0, 0x46EC16D8, 0x46EC3A01, 0x46EC5D2C, 0x46EC8058,
 0x46ECA386, 0x46ECC6B4, 0x46ECE9E4, 0x46ED0D16, 0x46ED3048, 0x46ED537C, 0x46ED76B1, 0x46ED99E8,
 0x46EDBD20, 0x46EDE059, 0x46EE0393, 0x46EE26CF, 0x46EE4A0C, 0x46EE6D4A, 0x46EE908A, 0x46EEB3CB,
 0x46EED70D, 0x46EEFA50, 0x46EF1D95, 0x46EF40DB, 0x46EF6423, 0x46EF876C, 0x46EFAAB6, 0x46EFCE01,
 0x46EFF14E, 0x46F0149C, 0x46F037EB, 0x46F05B3B, 0x46F07E8D, 0x46F0A1E0, 0x46F0C535, 0x46F0E88A,
 0x46F10BE1, 0x46F12F3A, 0x46F15293, 0x46F175EE, 0x46F1994A, 0x46F1BCA8, 0x46F1E007, 0x46F20367,
 0x46F226C8, 0x46F24A2B, 0x46F26D8F, 0x46F290F4, 0x46F2B45B, 0x46F2D7C2, 0x46F2FB2B, 0x46F31E96,
 0x46F34202, 0x46F3656F, 0x46F388DD, 0x46F3AC4C, 0x46F3CFBD, 0x46F3F32F, 0x46F416A3, 0x46F43A18,
 0x46F45D8E, 0x46F48105, 0x46F4A47D, 0x46F4C7F7, 0x46F4EB72, 0x46F50EEF, 0x46F5326D, 0x46F555EC,
 0x46F5796C, 0x46F59CED, 0x46F5C070, 0x46F5E3F4, 0x46F6077A, 0x46F62B01, 0x46F64E89, 0x46F67212,
 0x46F6959C, 0x46F6B928, 0x46F6DCB5, 0x46F70044, 0x46F723D3, 0x46F74764, 0x46F76AF7, 0x46F78E8A,
 0x46F7B21F, 0x46F7D5B5, 0x46F7F94C, 0x46F81CE5, 0x46F8407F, 0x46F8641A, 0x46F887B6, 0x46F8AB54,
 0x46F8CEF3, 0x46F8F293, 0x46F91635, 0x46F939D8, 0x46F95D7C, 0x46F98121, 0x46F9A4C8, 0x46F9C870,
 0x46F9EC19, 0x46FA0FC3, 0x46FA336F, 0x46FA571C, 0x46FA7ACA, 0x46FA9E7A, 0x46FAC22B, 0x46FAE5DD,
 0x46FB0990, 0x46FB2D45, 0x46FB50FB, 0x46FB74B2, 0x46FB986A, 0x46FBBC24, 0x46FBDFDF, 0x46FC039B,
 0x46FC2759, 0x46FC4B17, 0x46FC6ED8, 0x46FC9299, 0x46FCB65B, 0x46FCDA1F, 0x46FCFDE4, 0x46FD21AB,
 0x46FD4572, 0x46FD693B, 0x46FD8D05, 0x46FDB0D1, 0x46FDD49D, 0x46FDF86B, 0x46FE1C3B, 0x46FE400B,
 0x46FE63DD, 0x46FE87B0, 0x46FEAB84, 0x46FECF5A, 0x46FEF330, 0x46FF1708, 0x46FF3AE2, 0x46FF5EBC,
 0x46FF8298, 0x46FFA675, 0x46FFCA53, 0x46FFEE33, 0x4700090A, 0x47001AFB, 0x47002CED, 0x47003EDF,
 0x470050D2, 0x470062C5, 0x470074BA, 0x470086AE, 0x470098A4, 0x4700AA9A, 0x4700BC91, 0x4700CE88,
 0x4700E080, 0x4700F278, 0x47010472, 0x4701166B, 0x47012866, 0x47013A61, 0x47014C5D, 0x47015E59,
 0x47017056, 0x47018254, 0x47019452, 0x4701A651, 0x4701B850, 0x4701CA50, 0x4701DC51, 0x4701EE52,
 0x47020054, 0x47021257, 0x4702245A, 0x4702365E, 0x47024862, 0x47025A67, 0x47026C6D, 0x47027E73,
 0x4702907A, 0x4702A282, 0x4702B48A, 0x4702C693, 0x4702D89C, 0x4702EAA6, 0x4702FCB1, 0x47030EBC,
 0x470320C8, 0x470332D5, 0x470344E2, 0x470356EF, 0x470368FE, 0x47037B0D, 0x47038D1C, 0x47039F2D,
 0x4703B13E, 0x4703C34F, 0x4703D561, 0x4703E774, 0x4703F987, 0x47040B9B, 0x47041DB0, 0x47042FC5,
 0x470441DB, 0x470453F1, 0x47046608, 0x47047820, 0x47048A38, 0x47049C51, 0x4704AE6B, 0x4704C085,
 0x4704D2A0, 0x4704E4BB, 0x4704F6D7, 0x470508F4, 0x47051B11, 0x47052D2F, 0x47053F4D, 0x4705516C,
 0x4705638C, 0x470575AC, 0x470587CD, 0x470599EF, 0x4705AC11, 0x4705BE34, 0x4705D057, 0x4705E27B,
 0x4705F4A0, 0x470606C5, 0x470618EB, 0x47062B11, 0x47063D38, 0x47064F60, 0x47066188, 0x470673B1,
 0x470685DB, 0x47069805, 0x4706AA30, 0x4706BC5B, 0x4706CE87, 0x4706E0B4, 0x4706F2E1, 0x4707050F,
 0x4707173D, 0x4707296C, 0x47073B9C, 0x47074DCC, 0x47075FFD, 0x4707722F, 0x47078461, 0x47079693,
 0x4707A8C7, 0x4707BAFB, 0x4707CD2F, 0x4707DF64, 0x4707F19A, 0x470803D0, 0x47081607, 0x4708283F,
 0x47083A77, 0x47084CB0, 0x47085EE9, 0x47087123, 0x4708835E, 0x47089599, 0x4708A7D5, 0x4708BA12,
 0x4708CC4F, 0x4708DE8C, 0x4708F0CB, 0x4709030A, 0x47091549, 0x47092789, 0x470939CA, 0x47094C0B,
 0x47095E4D, 0x47097090, 0x470982D3, 0x47099517, 0x4709A75B, 0x4709B9A0, 0x4709CBE5, 0x4709DE2C,
 0x4709F072, 0x470A02BA, 0x470A1502, 0x470A274A, 0x470A3994, 0x470A4BDD, 0x470A5E28, 0x470A7073,
 0x470A82BE, 0x470A950B, 0x470AA757, 0x470AB9A5, 0x470ACBF3, 0x470ADE42, 0x470AF091, 0x470B02E1,
 0x470B1531, 0x470B2782, 0x470B39D4, 0x470B4C26, 0x470B5E79, 0x470B70CC, 0x470B8320, 0x470B9575,
 0x470BA7CA, 0x470BBA20, 0x470BCC77, 0x470BDECE, 0x470BF125, 0x470C037E, 0x470C15D7, 0x470C2830,
 0x470C3A8A, 0x470C4CE5, 0x470C5F40, 0x470C719C, 0x470C83F8, 0x470C9656, 0x470CA8B3, 0x470CBB12,
 0x470CCD70, 0x470CDFD0, 0x470CF230, 0x470D0491, 0x470D16F2, 0x470D2954, 0x470D3BB6, 0x470D4E19,
 0x470D607D, 0x470D72E1, 0x470D8546, 0x470D97AC, 0x470DAA12, 0x470DBC78, 0x470DCEE0, 0x470DE147,
 0x470DF3B0, 0x470E0619, 0x470E1883, 0x470E2AED, 0x470E3D58, 0x470E4FC3, 0x470E622F, 0x470E749C,
 0x470E8709, 0x470E9977, 0x470EABE5, 0x470EBE54, 0x470ED0C4, 0x470EE334, 0x470EF5A5, 0x470F0816,
 0x470F1A88, 0x470F2CFB, 0x470F3F6E, 0x470F51E2, 0x470F6456, 0x470F76CB, 0x470F8940, 0x470F9BB7,
 0x470FAE2D, 0x470FC0A5, 0x470FD31D, 0x470FE595, 0x470FF80E, 0x47100A88, 0x47101D02, 0x47102F7D,
 0x471041F8, 0x47105475, 0x471066F1, 0x4710796E, 0x47108BEC, 0x47109E6B, 0x4710B0EA, 0x4710C369,
 0x4710D5EA, 0x4710E86A, 0x4710FAEC, 0x47110D6E, 0x47111FF0, 0x47113273, 0x471144F7, 0x4711577B,
 0x47116A00, 0x47117C86, 0x47118F0C, 0x4711A193, 0x4711B41A, 0x4711C6A2, 0x4711D92A, 0x4711EBB3,
 0x4711FE3D, 0x471210C7, 0x47122352, 0x471235DE, 0x47124869, 0x47125AF6, 0x47126D83, 0x47128011,
 0x4712929F, 0x4712A52E, 0x4712B7BE, 0x4712CA4E, 0x4712DCDF, 0x4712EF70, 0x47130202, 0x47131494,
 0x47132727, 0x471339BB, 0x47134C4F, 0x47135EE4, 0x47137179, 0x4713840F, 0x471396A6, 0x4713A93D,
 0x4713BBD5, 0x4713CE6D, 0x4713E106, 0x4713F39F, 0x47140639, 0x471418D4, 0x47142B6F, 0x47143E0B,
 0x471450A8, 0x47146344, 0x471475E2, 0x47148880, 0x47149B1F, 0x4714ADBE, 0x4714C05E, 0x4714D2FF,
 0x4714E5A0, 0x4714F841, 0x47150AE4, 0x47151D86, 0x4715302A, 0x471542CE, 0x47155572, 0x47156818,
 0x47157ABD, 0x47158D64, 0x4715A00A, 0x4715B2B2, 0x4715C55A, 0x4715D803, 0x4715EAAC, 0x4715FD56,
 0x47161000, 0x471622AB, 0x47163556, 0x47164803, 0x47165AAF, 0x47166D5D, 0x4716800A, 0x471692B9,
 0x4716A568, 0x4716B817, 0x4716CAC8, 0x4716DD78, 0x4716F02A, 0x471702DC, 0x4717158E, 0x47172841,
 0x47173AF5, 0x47174DA9, 0x4717605E, 0x47177313, 0x471785C9, 0x47179880, 0x4717AB37, 0x4717BDEF,
 0x4717D0A7, 0x4717E360, 0x4717F619, 0x471808D3, 0x47181B8E, 0x47182E49, 0x47184105, 0x471853C1,
 0x4718667E, 0x4718793B, 0x47188BF9, 0x47189EB8, 0x4718B177, 0x4718C437, 0x4718D6F7, 0x4718E9B8,
 0x4718FC7A, 0x47190F3C, 0x471921FE, 0x471934C1, 0x47194785, 0x47195A4A, 0x47196D0F, 0x47197FD4,
 0x4719929A, 0x4719A561, 0x4719B828, 0x4719CAF0, 0x4719DDB8, 0x4719F081, 0x471A034B, 0x471A1615,
 0x471A28DF, 0x471A3BAB, 0x471A4E77, 0x471A6143, 0x471A7410, 0x471A86DD, 0x471A99AC, 0x471AAC7A,
 0x471ABF49, 0x471AD219, 0x471AE4EA, 0x471AF7BB, 0x471B0A8C, 0x471B1D5E, 0x471B3031, 0x471B4304,
 0x471B55D8, 0x471B68AC, 0x471B7B81, 0x471B8E57, 0x471BA12D, 0x471BB404, 0x471BC6DB, 0x471BD9B3,
 0x471BEC8B, 0x471BFF64, 0x471C123E, 0x471C2518, 0x471C37F2, 0x471C4ACD, 0x471C5DA9, 0x471C7086,
 0x471C8363, 0x471C9640, 0x471CA91E, 0x471CBBFD, 0x471CCEDC, 0x471CE1BC, 0x471CF49C, 0x471D077D,
 0x471D1A5E, 0x471D2D40, 0x471D4023, 0x471D5306, 0x471D65EA, 0x471D78CE, 0x471D8BB3, 0x471D9E99,
 0x471DB17F, 0x471DC465, 0x471DD74C, 0x471DEA34, 0x471DFD1C, 0x471E1005, 0x471E22EF, 0x471E35D9,
 0x471E48C3, 0x471E5BAE, 0x471E6E9A, 0x471E8186, 0x471E9473, 0x471EA760, 0x471EBA4E, 0x471ECD3D,
 0x471EE02C, 0x471EF31B, 0x471F060B, 0x471F18FC, 0x471F2BEE, 0x471F3EDF, 0x471F51D2, 0x471F64C5,
 0x471F77B8, 0x471F8AAD, 0x471F9DA1, 0x471FB096, 0x471FC38C, 0x471FD683, 0x471FE97A, 0x471FFC71,
 0x47200F69, 0x47202262, 0x4720355B, 0x47204855, 0x47205B4F, 0x47206E4A, 0x47208145, 0x47209441,
 0x4720A73E, 0x4720BA3B, 0x4720CD39, 0x4720E037, 0x4720F336, 0x47210635, 0x47211935, 0x47212C36,
 0x47213F37, 0x47215238, 0x4721653A, 0x4721783D, 0x47218B40, 0x47219E44, 0x4721B149, 0x4721C44E,
 0x4721D753, 0x4721EA59, 0x4721FD60, 0x47221067, 0x4722236F, 0x47223677, 0x47224980, 0x47225C89,
 0x47226F93, 0x4722829E, 0x472295A9, 0x4722A8B5, 0x4722BBC1, 0x4722CECE, 0x4722E1DB, 0x4722F4E9,
 0x472307F7, 0x47231B06, 0x47232E16, 0x47234126, 0x47235437, 0x47236748, 0x47237A5A, 0x47238D6C,
 0x4723A07F, 0x4723B392, 0x4723C6A6, 0x4723D9BB, 0x4723ECD0, 0x4723FFE6, 0x472412FC, 0x47242613,
 0x4724392A, 0x47244C42, 0x47245F5A, 0x47247273, 0x4724858D, 0x472498A7, 0x4724ABC2, 0x4724BEDD,
 0x4724D1F9, 0x4724E515, 0x4724F832, 0x47250B4F, 0x47251E6D, 0x4725318C, 0x472544AB, 0x472557CB,
 0x47256AEB, 0x47257E0C, 0x4725912D, 0x4725A44F, 0x4725B771, 0x4725CA94, 0x4725DDB8, 0x4725F0DC,
 0x47260400, 0x47261726, 0x47262A4B, 0x47263D72, 0x47265098, 0x472663C0, 0x472676E8, 0x47268A10,
 0x47269D39, 0x4726B063, 0x4726C38D, 0x4726D6B8, 0x4726E9E3, 0x4726FD0F, 0x4727103B, 0x47272368,
 0x47273695, 0x472749C3, 0x47275CF2, 0x47277021, 0x47278351, 0x47279681, 0x4727A9B2, 0x4727BCE3,
 0x4727D015, 0x4727E347, 0x4727F67A, 0x472809AE, 0x47281CE2, 0x47283016, 0x4728434B, 0x47285681,
 0x472869B7, 0x47287CEE, 0x47289025, 0x4728A35D, 0x4728B696, 0x4728C9CF, 0x4728DD08, 0x4728F042,
 0x4729037D, 0x472916B8, 0x472929F4, 0x47293D30, 0x4729506D, 0x472963AA, 0x472976E8, 0x47298A26,
 0x47299D65, 0x4729B0A5, 0x4729C3E5, 0x4729D725, 0x4729EA67, 0x4729FDA8, 0x472A10EB, 0x472A242D,
 0x472A3771, 0x472A4AB5, 0x472A5DF9, 0x472A713E, 0x472A8484, 0x472A97CA, 0x472AAB10, 0x472ABE57,
 0x472AD19F, 0x472AE4E7, 0x472AF830, 0x472B0B79, 0x472B1EC3, 0x472B320E, 0x472B4559, 0x472B58A4,
 0x472B6BF0, 0x472B7F3D, 0x472B928A, 0x472BA5D8, 0x472BB926, 0x472BCC75, 0x472BDFC4, 0x472BF314,
 0x472C0664, 0x472C19B5, 0x472C2D07, 0x472C4059, 0x472C53AB, 0x472C66FE, 0x472C7A52, 0x472C8DA6,
 0x472CA0FB, 0x472CB450, 0x472CC7A6, 0x472CDAFC, 0x472CEE53, 0x472D01AB, 0x472D1502, 0x472D285B,
 0x472D3BB4, 0x472D4F0E, 0x472D6268, 0x472D75C2, 0x472D891E, 0x472D9C79, 0x472DAFD6, 0x472DC333,
 0x472DD690, 0x472DE9EE, 0x472DFD4C, 0x472E10AB, 0x472E240B, 0x472E376B, 0x472E4ACB, 0x472E5E2D,
 0x472E718E, 0x472E84F0, 0x472E9853, 0x472EABB7, 0x472EBF1A, 0x472ED27F, 0x472EE5E4, 0x472EF949,
 0x472F0CAF, 0x472F2016, 0x472F337D, 0x472F46E4, 0x472F5A4C, 0x472F6DB5, 0x472F811E, 0x472F9488,
 0x472FA7F2, 0x472FBB5D, 0x472FCEC8, 0x472FE234, 0x472FF5A1, 0x4730090E, 0x47301C7B, 0x47302FE9,
 0x47304358, 0x473056C7, 0x47306A37, 0x47307DA7, 0x47309118, 0x4730A489, 0x4730B7FB, 0x4730CB6D,
 0x4730DEE0, 0x4730F253, 0x473105C7, 0x4731193B, 0x47312CB0, 0x47314026, 0x4731539C, 0x47316713,
 0x47317A8A, 0x47318E01, 0x4731A17A, 0x4731B4F2, 0x4731C86C, 0x4731DBE5, 0x4731EF60, 0x473202DB,
 0x47321656, 0x473229D2, 0x47323D4E, 0x473250CB, 0x47326449, 0x473277C7, 0x47328B46, 0x47329EC5,
 0x4732B244, 0x4732C5C5, 0x4732D945, 0x4732ECC6, 0x47330048, 0x473313CB, 0x4733274D, 0x47333AD1,
 0x47334E55, 0x473361D9, 0x4733755E, 0x473388E4, 0x47339C6A, 0x4733AFF0, 0x4733C377, 0x4733D6FF,
 0x4733EA87, 0x4733FE10, 0x47341199, 0x47342523, 0x473438AD, 0x47344C38, 0x47345FC3, 0x4734734F,
 0x473486DB, 0x47349A68, 0x4734ADF6, 0x4734C184, 0x4734D512, 0x4734E8A1, 0x4734FC31, 0x47350FC1,
 0x47352352, 0x473536E3, 0x47354A74, 0x47355E07, 0x47357199, 0x4735852D, 0x473598C0, 0x4735AC55,
 0x4735BFE9, 0x4735D37F, 0x4735E715, 0x4735FAAB, 0x47360E42, 0x473621DA, 0x47363572, 0x4736490A,
 0x47365CA3, 0x4736703D, 0x473683D7, 0x47369772, 0x4736AB0D, 0x4736BEA8, 0x4736D245, 0x4736E5E1,
 0x4736F97F, 0x47370D1C, 0x473720BB, 0x4737345A, 0x473747F9, 0x47375B99, 0x47376F39, 0x473782DA,
 0x4737967C, 0x4737AA1E, 0x4737BDC0, 0x4737D163, 0x4737E507, 0x4737F8AB, 0x47380C50, 0x47381FF5,
 0x4738339A, 0x47384741, 0x47385AE7, 0x47386E8F, 0x47388236, 0x473895DF, 0x4738A987, 0x4738BD31,
 0x4738D0DB, 0x4738E485, 0x4738F830, 0x47390BDB, 0x47391F87, 0x47393334, 0x473946E1, 0x47395A8E,
 0x47396E3C, 0x473981EB, 0x4739959A, 0x4739A94A, 0x4739BCFA, 0x4739D0AA, 0x4739E45B, 0x4739F80D,
 0x473A0BBF, 0x473A1F72, 0x473A3325, 0x473A46D9, 0x473A5A8D, 0x473A6E42, 0x473A81F8, 0x473A95AD,
 0x473AA964, 0x473ABD1B, 0x473AD0D2, 0x473AE48A, 0x473AF843, 0x473B0BFC, 0x473B1FB5, 0x473B336F,
 0x473B472A, 0x473B5AE5, 0x473B6EA0, 0x473B825C, 0x473B9619, 0x473BA9D6, 0x473BBD94, 0x473BD152,
 0x473BE511, 0x473BF8D0, 0x473C0C90, 0x473C2050, 0x473C3411, 0x473C47D2, 0x473C5B94, 0x473C6F56,
 0x473C8319, 0x473C96DC, 0x473CAAA0, 0x473CBE65, 0x473CD229, 0x473CE5EF, 0x473CF9B5, 0x473D0D7B,
 0x473D2142, 0x473D350A, 0x473D48D2, 0x473D5C9A, 0x473D7063, 0x473D842D, 0x473D97F7, 0x473DABC2,
 0x473DBF8D, 0x473DD358, 0x473DE725, 0x473DFAF1, 0x473E0EBE, 0x473E228C, 0x473E365A, 0x473E4A29,
 0x473E5DF8, 0x473E71C8, 0x473E8598, 0x473E9969, 0x473EAD3A, 0x473EC10C, 0x473ED4DF, 0x473EE8B1,
 0x473EFC85, 0x473F1059, 0x473F242D, 0x473F3802, 0x473F4BD7, 0x473F5FAD, 0x473F7384, 0x473F875B,
 0x473F9B32, 0x473FAF0A, 0x473FC2E3, 0x473FD6BC, 0x473FEA95, 0x473FFE6F, 0x4740124A, 0x47402625,
 0x47403A01, 0x47404DDD, 0x474061B9, 0x47407596, 0x47408974, 0x47409D52, 0x4740B131, 0x4740C510,
 0x4740D8F0, 0x4740ECD0, 0x474100B1, 0x47411492, 0x47412874, 0x47413C56, 0x47415039, 0x4741641C,
 0x47417800, 0x47418BE4, 0x47419FC9, 0x4741B3AE, 0x4741C794, 0x4741DB7A, 0x4741EF61, 0x47420348,
 0x47421730, 0x47422B19, 0x47423F02, 0x474252EB, 0x474266D5, 0x47427ABF, 0x47428EAA, 0x4742A296,
 0x4742B682, 0x4742CA6E, 0x4742DE5B, 0x4742F248, 0x47430636, 0x47431A25, 0x47432E14, 0x47434203,
 0x474355F3, 0x474369E4, 0x47437DD5, 0x474391C7, 0x4743A5B9, 0x4743B9AB, 0x4743CD9E, 0x4743E192,
 0x4743F586, 0x4744097B, 0x47441D70, 0x47443165, 0x4744455B, 0x47445952, 0x47446D49, 0x47448141,
 0x47449539, 0x4744A932, 0x4744BD2B, 0x4744D124, 0x4744E51F, 0x4744F919, 0x47450D14, 0x47452110,
 0x4745350C, 0x47454909, 0x47455D06, 0x47457104, 0x47458502, 0x47459901, 0x4745AD00, 0x4745C100,
 0x4745D500, 0x4745E901, 0x4745FD02, 0x47461104, 0x47462506, 0x47463909, 0x47464D0C, 0x47466110,
 0x47467514, 0x47468919, 0x47469D1F, 0x4746B124, 0x4746C52B, 0x4746D932, 0x4746ED39, 0x47470141,
 0x47471549, 0x47472952, 0x47473D5B, 0x47475165, 0x4747656F, 0x4747797A, 0x47478D86, 0x4747A191,
 0x4747B59E, 0x4747C9AB, 0x4747DDB8, 0x4747F1C6, 0x474805D4, 0x474819E3, 0x47482DF2, 0x47484202,
 0x47485613, 0x47486A24, 0x47487E35, 0x47489247, 0x4748A659, 0x4748BA6C, 0x4748CE80, 0x4748E293,
 0x4748F6A8, 0x47490ABD, 0x47491ED2, 0x474932E8, 0x474946FE, 0x47495B15, 0x47496F2D, 0x47498345,
 0x4749975D, 0x4749AB76, 0x4749BF8F, 0x4749D3A9, 0x4749E7C4, 0x4749FBDF, 0x474A0FFA, 0x474A2416,
 0x474A3832, 0x474A4C4F, 0x474A606D, 0x474A748B, 0x474A88A9, 0x474A9CC8, 0x474AB0E7, 0x474AC507,
 0x474AD928, 0x474AED49, 0x474B016A, 0x474B158C, 0x474B29AE, 0x474B3DD1, 0x474B51F5, 0x474B6619,
 0x474B7A3D, 0x474B8E62, 0x474BA287, 0x474BB6AD, 0x474BCAD4, 0x474BDEFA, 0x474BF322, 0x474C074A,
 0x474C1B72, 0x474C2F9B, 0x474C43C4, 0x474C57EE, 0x474C6C19, 0x474C8044, 0x474C946F, 0x474CA89B,
 0x474CBCC7, 0x474CD0F4, 0x474CE521, 0x474CF94F, 0x474D0D7E, 0x474D21AC, 0x474D35DC, 0x474D4A0C,
 0x474D5E3C, 0x474D726D, 0x474D869E, 0x474D9AD0, 0x474DAF02, 0x474DC335, 0x474DD768, 0x474DEB9C,
 0x474DFFD1, 0x474E1405, 0x474E283B, 0x474E3C70, 0x474E50A7, 0x474E64DD, 0x474E7915, 0x474E8D4D,
 0x474EA185, 0x474EB5BE, 0x474EC9F7, 0x474EDE31, 0x474EF26B, 0x474F06A6, 0x474F1AE1, 0x474F2F1D,
 0x474F4359, 0x474F5795, 0x474F6BD3, 0x474F8010, 0x474F944F, 0x474FA88D, 0x474FBCCC, 0x474FD10C,
 0x474FE54C, 0x474FF98D, 0x47500DCE, 0x47502210, 0x47503652, 0x47504A95, 0x47505ED8, 0x4750731B,
 0x4750875F, 0x47509BA4, 0x4750AFE9, 0x4750C42F, 0x4750D875, 0x4750ECBB, 0x47510102, 0x4751154A,
 0x47512992, 0x47513DDB, 0x47515224, 0x4751666D, 0x47517AB7, 0x47518F02, 0x4751A34D, 0x4751B798,
 0x4751CBE4, 0x4751E031, 0x4751F47E, 0x475208CB, 0x47521D19, 0x47523167, 0x475245B6, 0x47525A06,
 0x47526E56, 0x475282A6, 0x475296F7, 0x4752AB48, 0x4752BF9A, 0x4752D3ED, 0x4752E83F, 0x4752FC93,
 0x475310E7, 0x4753253B, 0x47533990, 0x47534DE5, 0x4753623B, 0x47537691, 0x47538AE8, 0x47539F3F,
 0x4753B397, 0x4753C7EF, 0x4753DC48, 0x4753F0A1, 0x475404FB, 0x47541955, 0x47542DB0, 0x4754420B,
 0x47545666, 0x47546AC2, 0x47547F1F, 0x4754937C, 0x4754A7DA, 0x4754BC38, 0x4754D096, 0x4754E4F6,
 0x4754F955, 0x47550DB5, 0x47552216, 0x47553677, 0x47554AD8, 0x47555F3A, 0x4755739D, 0x47558800,
 0x47559C63, 0x4755B0C7, 0x4755C52B, 0x4755D990, 0x4755EDF6, 0x4756025C, 0x475616C2, 0x47562B29,
 0x47563F90, 0x475653F8, 0x47566860, 0x47567CC9, 0x47569132, 0x4756A59C, 0x4756BA06, 0x4756CE71,
 0x4756E2DC, 0x4756F748, 0x47570BB4, 0x47572021, 0x4757348E, 0x475748FC, 0x47575D6A, 0x475771D8,
 0x47578647, 0x47579AB7, 0x4757AF27, 0x4757C398, 0x4757D809, 0x4757EC7A, 0x475800EC, 0x4758155F,
 0x475829D2, 0x47583E45, 0x475852B9, 0x4758672D, 0x47587BA2, 0x47589018, 0x4758A48E, 0x4758B904,
 0x4758CD7B, 0x4758E1F2, 0x4758F66A, 0x47590AE2, 0x47591F5B, 0x475933D4, 0x4759484E, 0x47595CC8,
 0x47597143, 0x475985BE, 0x47599A3A, 0x4759AEB6, 0x4759C333, 0x4759D7B0, 0x4759EC2D, 0x475A00AB,
 0x475A152A, 0x475A29A9, 0x475A3E28, 0x475A52A8, 0x475A6729, 0x475A7BAA, 0x475A902B, 0x475AA4AD,
 0x475AB930, 0x475ACDB2, 0x475AE236, 0x475AF6BA, 0x475B0B3E, 0x475B1FC3, 0x475B3448, 0x475B48CE,
 0x475B5D54, 0x475B71DB, 0x475B8662, 0x475B9AEA, 0x475BAF72, 0x475BC3FB, 0x475BD884, 0x475BED0D,
 0x475C0197, 0x475C1622, 0x475C2AAD, 0x475C3F38, 0x475C53C4, 0x475C6851, 0x475C7CDE, 0x475C916B,
 0x475CA5F9, 0x475CBA88, 0x475CCF17, 0x475CE3A6, 0x475CF836, 0x475D0CC6, 0x475D2157, 0x475D35E8,
 0x475D4A7A, 0x475D5F0C, 0x475D739F, 0x475D8832, 0x475D9CC6, 0x475DB15A, 0x475DC5EE, 0x475DDA83,
 0x475DEF19, 0x475E03AF, 0x475E1846, 0x475E2CDD, 0x475E4174, 0x475E560C, 0x475E6AA4, 0x475E7F3D,
 0x475E93D7, 0x475EA871, 0x475EBD0B, 0x475ED1A6, 0x475EE641, 0x475EFADD, 0x475F0F79, 0x475F2416,
 0x475F38B3, 0x475F4D51, 0x475F61EF, 0x475F768D, 0x475F8B2D, 0x475F9FCC, 0x475FB46C, 0x475FC90D,
 0x475FDDAE, 0x475FF24F, 0x476006F1, 0x47601B93, 0x47603036, 0x476044DA, 0x4760597E, 0x47606E22,
 0x476082C7, 0x4760976C, 0x4760AC12, 0x4760C0B8, 0x4760D55F, 0x4760EA06, 0x4760FEAE, 0x47611356,
 0x476127FE, 0x47613CA7, 0x47615151, 0x476165FB, 0x47617AA5, 0x47618F50, 0x4761A3FC, 0x4761B8A8,
 0x4761CD54, 0x4761E201, 0x4761F6AE, 0x47620B5C, 0x4762200A, 0x476234B9, 0x47624968, 0x47625E18,
 0x476272C8, 0x47628779, 0x47629C2A, 0x4762B0DB, 0x4762C58E, 0x4762DA40, 0x4762EEF3, 0x476303A7,
 0x4763185A, 0x47632D0F, 0x476341C4, 0x47635679, 0x47636B2F, 0x47637FE5, 0x4763949C, 0x4763A953,
 0x4763BE0B, 0x4763D2C3, 0x4763E77C, 0x4763FC35, 0x476410EF, 0x476425A9, 0x47643A63, 0x47644F1E,
 0x476463DA, 0x47647896, 0x47648D52, 0x4764A20F, 0x4764B6CC, 0x4764CB8A, 0x4764E048, 0x4764F507,
 0x476509C6, 0x47651E86, 0x47653346, 0x47654807, 0x47655CC8, 0x4765718A, 0x4765864C, 0x47659B0E,
 0x4765AFD1, 0x4765C495, 0x4765D959, 0x4765EE1D, 0x476602E2, 0x476617A7, 0x47662C6D, 0x47664134,
 0x476655FA, 0x47666AC2, 0x47667F89, 0x47669451, 0x4766A91A, 0x4766BDE3, 0x4766D2AD, 0x4766E777,
 0x4766FC41, 0x4767110C, 0x476725D8, 0x47673AA3, 0x47674F70, 0x4767643D, 0x4767790A, 0x47678DD8,
 0x4767A2A6, 0x4767B775, 0x4767CC44, 0x4767E114, 0x4767F5E4, 0x47680AB4, 0x47681F85, 0x47683457,
 0x47684929, 0x47685DFB, 0x476872CE, 0x476887A1, 0x47689C75, 0x4768B14A, 0x4768C61E, 0x4768DAF4,
 0x4768EFC9, 0x476904A0, 0x47691976, 0x47692E4D, 0x47694325, 0x476957FD, 0x47696CD5, 0x476981AE,
 0x47699688, 0x4769AB62, 0x4769C03C, 0x4769D517, 0x4769E9F2, 0x4769FECE, 0x476A13AA, 0x476A2887,
 0x476A3D64, 0x476A5242, 0x476A6720, 0x476A7BFE, 0x476A90DD, 0x476AA5BD, 0x476ABA9D, 0x476ACF7D,
 0x476AE45E, 0x476AF93F, 0x476B0E21, 0x476B2303, 0x476B37E6, 0x476B4CC9, 0x476B61AD, 0x476B7691,
 0x476B8B76, 0x476BA05B, 0x476BB540, 0x476BCA26, 0x476BDF0D, 0x476BF3F3, 0x476C08DB, 0x476C1DC3,
 0x476C32AB, 0x476C4794, 0x476C5C7D, 0x476C7166, 0x476C8651, 0x476C9B3B, 0x476CB026, 0x476CC512,
 0x476CD9FE, 0x476CEEEA, 0x476D03D7, 0x476D18C4, 0x476D2DB2, 0x476D42A1, 0x476D578F, 0x476D6C7F,
 0x476D816E, 0x476D965E, 0x476DAB4F, 0x476DC040, 0x476DD532, 0x476DEA24, 0x476DFF16, 0x476E1409,
 0x476E28FC, 0x476E3DF0, 0x476E52E4, 0x476E67D9, 0x476E7CCE, 0x476E91C4, 0x476EA6BA, 0x476EBBB1,
 0x476ED0A8, 0x476EE59F, 0x476EFA97, 0x476F0F90, 0x476F2489, 0x476F3982, 0x476F4E7C, 0x476F6376,
 0x476F7871, 0x476F8D6C, 0x476FA268, 0x476FB764, 0x476FCC60, 0x476FE15D, 0x476FF65B, 0x47700B59,
 0x47702057, 0x47703556, 0x47704A55, 0x47705F55, 0x47707455, 0x47708956, 0x47709E57, 0x4770B359,
 0x4770C85B, 0x4770DD5D, 0x4770F260, 0x47710764, 0x47711C68, 0x4771316C, 0x47714671, 0x47715B76,
 0x4771707C, 0x47718582, 0x47719A89, 0x4771AF90, 0x4771C497, 0x4771D99F, 0x4771EEA8, 0x477203B1,
 0x477218BA, 0x47722DC4, 0x477242CE, 0x477257D9, 0x47726CE4, 0x477281F0, 0x477296FC, 0x4772AC09,
 0x4772C116, 0x4772D623, 0x4772EB31, 0x47730040, 0x4773154F, 0x47732A5E, 0x47733F6E, 0x4773547E,
 0x4773698F, 0x47737EA0, 0x477393B1, 0x4773A8C3, 0x4773BDD6, 0x4773D2E9, 0x4773E7FC, 0x4773FD10,
 0x47741225, 0x47742739, 0x47743C4F, 0x47745164, 0x4774667A, 0x47747B91, 0x477490A8, 0x4774A5C0,
 0x4774BAD8, 0x4774CFF0, 0x4774E509, 0x4774FA22, 0x47750F3C, 0x47752456, 0x47753971, 0x47754E8C,
 0x477563A8, 0x477578C4, 0x47758DE0, 0x4775A2FD, 0x4775B81B, 0x4775CD39, 0x4775E257, 0x4775F776,
 0x47760C95, 0x477621B5, 0x477636D5, 0x47764BF6, 0x47766117, 0x47767638, 0x47768B5A, 0x4776A07C,
 0x4776B59F, 0x4776CAC3, 0x4776DFE6, 0x4776F50B, 0x47770A2F, 0x47771F54, 0x4777347A, 0x477749A0,
 0x47775EC7, 0x477773EE, 0x47778915, 0x47779E3D, 0x4777B365, 0x4777C88E, 0x4777DDB7, 0x4777F2E1,
 0x4778080B, 0x47781D35, 0x47783260, 0x4778478C, 0x47785CB8, 0x477871E4, 0x47788711, 0x47789C3E,
 0x4778B16C, 0x4778C69A, 0x4778DBC9, 0x4778F0F8, 0x47790627, 0x47791B57, 0x47793088, 0x477945B8,
 0x47795AEA, 0x4779701C, 0x4779854E, 0x47799A80, 0x4779AFB4, 0x4779C4E7, 0x4779DA1B, 0x4779EF50,
 0x477A0485, 0x477A19BA, 0x477A2EF0, 0x477A4426, 0x477A595D, 0x477A6E94, 0x477A83CB, 0x477A9904,
 0x477AAE3C, 0x477AC375, 0x477AD8AE, 0x477AEDE8, 0x477B0323, 0x477B185D, 0x477B2D99, 0x477B42D4,
 0x477B5810, 0x477B6D4D, 0x477B828A, 0x477B97C7, 0x477BAD05, 0x477BC244, 0x477BD782, 0x477BECC2,
 0x477C0201, 0x477C1741, 0x477C2C82, 0x477C41C3, 0x477C5705, 0x477C6C47, 0x477C8189, 0x477C96CC,
 0x477CAC0F, 0x477CC153, 0x477CD697, 0x477CEBDC, 0x477D0121, 0x477D1666, 0x477D2BAC, 0x477D40F2,
 0x477D5639, 0x477D6B81, 0x477D80C8, 0x477D9611, 0x477DAB59, 0x477DC0A2, 0x477DD5EC, 0x477DEB36,
 0x477E0080, 0x477E15CB, 0x477E2B16, 0x477E4062, 0x477E55AE, 0x477E6AFB, 0x477E8048, 0x477E9596,
 0x477EAAE4, 0x477EC032, 0x477ED581, 0x477EEAD0, 0x477F0020, 0x477F1570, 0x477F2AC1, 0x477F4012,
 0x477F5564, 0x477F6AB6, 0x477F8008, 0x477F955B, 0x477FAAAE, 0x477FC002, 0x477FD556, 0x477FEAAB,
 0x47800000, 0x47800AAB, 0x47801556, 0x47802001, 0x47802AAC, 0x47803558, 0x47804004, 0x47804AB0,
 0x4780555C, 0x47806009, 0x47806AB6, 0x47807563, 0x47808010, 0x47808ABD, 0x4780956B, 0x4780A019,
 0x4780AAC7, 0x4780B575, 0x4780C024, 0x4780CAD3, 0x4780D582, 0x4780E031, 0x4780EAE0, 0x4780F590,
 0x47810040, 0x47810AF0, 0x478115A0, 0x47812051, 0x47812B02, 0x478135B3, 0x47814064, 0x47814B15,
 0x478155C7, 0x47816079, 0x47816B2B, 0x478175DD, 0x47818090, 0x47818B42, 0x478195F5, 0x4781A0A9,
 0x4781AB5C, 0x4781B610, 0x4781C0C4, 0x4781CB78, 0x4781D62C, 0x4781E0E0, 0x4781EB95, 0x4781F64A,
 0x478200FF, 0x47820BB5, 0x4782166A, 0x47822120, 0x47822BD6, 0x4782368D, 0x47824143, 0x47824BFA,
 0x478256B1, 0x47826168, 0x47826C1F, 0x478276D7, 0x4782818F, 0x47828C47, 0x478296FF, 0x4782A1B8,
 0x4782AC70, 0x4782B729, 0x4782C1E2, 0x4782CC9C, 0x4782D755, 0x4782E20F, 0x4782ECC9, 0x4782F783,
 0x4783023E, 0x47830CF8, 0x478317B3, 0x4783226E, 0x47832D2A, 0x478337E5, 0x478342A1, 0x47834D5D,
 0x47835819, 0x478362D6, 0x47836D92, 0x4783784F, 0x4783830C, 0x47838DCA, 0x47839887, 0x4783A345,
 0x4783AE03, 0x4783B8C1, 0x4783C380, 0x4783CE3E, 0x4783D8FD, 0x4783E3BC, 0x4783EE7B, 0x4783F93B,
 0x478403FB, 0x47840EBB, 0x4784197B, 0x4784243B, 0x47842EFC, 0x478439BD, 0x4784447E, 0x47844F3F,
 0x47845A00, 0x478464C2, 0x47846F84, 0x47847A46, 0x47848508, 0x47848FCB, 0x47849A8E, 0x4784A551,
 0x4784B014, 0x4784BAD8, 0x4784C59B, 0x4784D05F, 0x4784DB23, 0x4784E5E7, 0x4784F0AC, 0x4784FB71,
 0x47850636, 0x478510FB, 0x47851BC0, 0x47852686, 0x4785314C, 0x47853C12, 0x478546D8, 0x4785519F,
 0x47855C65, 0x4785672C, 0x478571F3, 0x47857CBB, 0x47858782, 0x4785924A, 0x47859D12, 0x4785A7DA,
 0x4785B2A3, 0x4785BD6B, 0x4785C834, 0x4785D2FD, 0x4785DDC7, 0x4785E890, 0x4785F35A, 0x4785FE24,
 0x478608EE, 0x478613B9, 0x47861E83, 0x4786294E, 0x47863419, 0x47863EE4, 0x478649B0, 0x4786547C,
 0x47865F48, 0x47866A14, 0x478674E0, 0x47867FAD, 0x47868A79, 0x47869546, 0x4786A014, 0x4786AAE1,
 0x4786B5AF, 0x4786C07D, 0x4786CB4B, 0x4786D619, 0x4786E0E8, 0x4786EBB6, 0x4786F685, 0x47870155,
 0x47870C24, 0x478716F4, 0x478721C3, 0x47872C93, 0x47873764, 0x47874234, 0x47874D05, 0x478757D6,
 0x478762A7, 0x47876D78, 0x4787784A, 0x4787831B, 0x47878DED, 0x478798C0, 0x4787A392, 0x4787AE65,
 0x4787B938, 0x4787C40B, 0x4787CEDE, 0x4787D9B1, 0x4787E485, 0x4787EF59, 0x4787FA2D, 0x47880502,
 0x47880FD6, 0x47881AAB, 0x47882580, 0x47883055, 0x47883B2B, 0x47884600, 0x478850D6, 0x47885BAC,
 0x47886683, 0x47887159, 0x47887C30, 0x47888707, 0x478891DE, 0x47889CB5, 0x4788A78D, 0x4788B265,
 0x4788BD3D, 0x4788C815, 0x4788D2ED, 0x4788DDC6, 0x4788E89F, 0x4788F378, 0x4788FE51, 0x4789092B,
 0x47891405, 0x47891EDF, 0x478929B9, 0x47893493, 0x47893F6E, 0x47894A48, 0x47895523, 0x47895FFF,
 0x47896ADA, 0x478975B6, 0x47898092, 0x47898B6E, 0x4789964A, 0x4789A127, 0x4789AC03, 0x4789B6E0,
 0x4789C1BE, 0x4789CC9B, 0x4789D779, 0x4789E256, 0x4789ED34, 0x4789F813, 0x478A02F1, 0x478A0DD0,
 0x478A18AF, 0x478A238E, 0x478A2E6D, 0x478A394D, 0x478A442C, 0x478A4F0C, 0x478A59EC, 0x478A64CD,
 0x478A6FAD, 0x478A7A8E, 0x478A856F, 0x478A9050, 0x478A9B32, 0x478AA614, 0x478AB0F5, 0x478ABBD8,
 0x478AC6BA, 0x478AD19C, 0x478ADC7F, 0x478AE762, 0x478AF245, 0x478AFD29, 0x478B080C, 0x478B12F0,
 0x478B1DD4, 0x478B28B8, 0x478B339D, 0x478B3E81, 0x478B4966, 0x478B544B, 0x478B5F30, 0x478B6A16,
 0x478B74FC, 0x478B7FE2, 0x478B8AC8, 0x478B95AE, 0x478BA095, 0x478BAB7B, 0x478BB662, 0x478BC14A,
 0x478BCC31, 0x478BD719, 0x478BE201, 0x478BECE9, 0x478BF7D1, 0x478C02B9, 0x478C0DA2, 0x478C188B,
 0x478C2374, 0x478C2E5D, 0x478C3947, 0x478C4431, 0x478C4F1B, 0x478C5A05, 0x478C64EF, 0x478C6FDA,
 0x478C7AC5, 0x478C85B0, 0x478C909B, 0x478C9B86, 0x478CA672, 0x478CB15E, 0x478CBC4A, 0x478CC736,
 0x478CD223, 0x478CDD10, 0x478CE7FC, 0x478CF2EA, 0x478CFDD7, 0x478D08C5, 0x478D13B2, 0x478D1EA0,
 0x478D298F, 0x478D347D, 0x478D3F6C, 0x478D4A5A, 0x478D5549, 0x478D6039, 0x478D6B28, 0x478D7618,
 0x478D8108, 0x478D8BF8, 0x478D96E8, 0x478DA1D9, 0x478DACC9, 0x478DB7BA, 0x478DC2AC, 0x478DCD9D,
 0x478DD88F, 0x478DE380, 0x478DEE72, 0x478DF965, 0x478E0457, 0x478E0F4A, 0x478E1A3C, 0x478E2530,
 0x478E3023, 0x478E3B16, 0x478E460A, 0x478E50FE, 0x478E5BF2, 0x478E66E6, 0x478E71DB, 0x478E7CD0,
 0x478E87C5, 0x478E92BA, 0x478E9DAF, 0x478EA8A5, 0x478EB39B, 0x478EBE91, 0x478EC987, 0x478ED47D,
 0x478EDF74, 0x478EEA6B, 0x478EF562, 0x478F0059, 0x478F0B51, 0x478F1648, 0x478F2140, 0x478F2C38,
 0x478F3731, 0x478F4229, 0x478F4D22, 0x478F581B, 0x478F6314, 0x478F6E0D, 0x478F7907, 0x478F8401,
 0x478F8EFB, 0x478F99F5, 0x478FA4EF, 0x478FAFEA, 0x478FBAE5, 0x478FC5E0, 0x478FD0DB, 0x478FDBD7,
 0x478FE6D2, 0x478FF1CE, 0x478FFCCA, 0x479007C7, 0x479012C3, 0x47901DC0, 0x479028BD, 0x479033BA,
 0x47903EB7, 0x479049B5, 0x479054B3, 0x47905FB1, 0x47906AAF, 0x479075AD, 0x479080AC, 0x47908BAB,
 0x479096AA, 0x4790A1A9, 0x4790ACA8, 0x4790B7A8, 0x4790C2A8, 0x4790CDA8, 0x4790D8A8, 0x4790E3A9,
 0x4790EEAA, 0x4790F9AA, 0x479104AC, 0x47910FAD, 0x47911AAE, 0x479125B0, 0x479130B2, 0x47913BB4,
 0x479146B7, 0x479151B9, 0x47915CBC, 0x479167BF, 0x479172C2, 0x47917DC6, 0x479188C9, 0x479193CD,
 0x47919ED1, 0x4791A9D5, 0x4791B4DA, 0x4791BFDF, 0x4791CAE3, 0x4791D5E9, 0x4791E0EE, 0x4791EBF3,
 0x4791F6F9, 0x479201FF, 0x47920D05, 0x4792180B, 0x47922312, 0x47922E19, 0x47923920, 0x47924427,
 0x47924F2E, 0x47925A36, 0x4792653E, 0x47927046, 0x47927B4E, 0x47928656, 0x4792915F, 0x47929C68,
 0x4792A771, 0x4792B27A, 0x4792BD83, 0x4792C88D, 0x4792D397, 0x4792DEA1, 0x4792E9AB, 0x4792F4B6,
 0x4792FFC0, 0x47930ACB, 0x479315D6, 0x479320E2, 0x47932BED, 0x479336F9, 0x47934205, 0x47934D11,
 0x4793581D, 0x4793632A, 0x47936E37, 0x47937944, 0x47938451, 0x47938F5E, 0x47939A6C, 0x4793A57A,
 0x4793B088, 0x4793BB96, 0x4793C6A4, 0x4793D1B3, 0x4793DCC2, 0x4793E7D1, 0x4793F2E0, 0x4793FDF0,
 0x479408FF, 0x4794140F, 0x47941F1F, 0x47942A2F, 0x47943540, 0x47944051, 0x47944B62, 0x47945673,
 0x47946184, 0x47946C96, 0x479477A7, 0x479482B9, 0x47948DCB, 0x479498DE, 0x4794A3F0, 0x4794AF03,
 0x4794BA16, 0x4794C529, 0x4794D03C, 0x4794DB50, 0x4794E664, 0x4794F178, 0x4794FC8C, 0x479507A0,
 0x479512B5, 0x47951DCA, 0x479528DF, 0x479533F4, 0x47953F0A, 0x47954A1F, 0x47955535, 0x4795604B,
 0x47956B61, 0x47957678, 0x4795818F, 0x47958CA5, 0x479597BC, 0x4795A2D4, 0x4795ADEB, 0x4795B903,
 0x4795C41B, 0x4795CF33, 0x4795DA4B, 0x4795E564, 0x4795F07D, 0x4795FB95, 0x479606AF, 0x479611C8,
 0x47961CE2, 0x479627FB, 0x47963315, 0x47963E2F, 0x4796494A, 0x47965464, 0x47965F7F, 0x47966A9A,
 0x479675B5, 0x479680D1, 0x47968BEC, 0x47969708, 0x4796A224, 0x4796AD40, 0x4796B85D, 0x4796C379,
 0x4796CE96, 0x4796D9B3, 0x4796E4D0, 0x4796EFEE, 0x4796FB0C, 0x47970629, 0x47971147, 0x47971C66,
 0x47972784, 0x479732A3, 0x47973DC2, 0x479748E1, 0x47975400, 0x47975F20, 0x47976A3F, 0x4797755F,
 0x4797807F, 0x47978BA0, 0x479796C0, 0x4797A1E1, 0x4797AD02, 0x4797B823, 0x4797C344, 0x4797CE66,
 0x4797D987, 0x4797E4A9, 0x4797EFCB, 0x4797FAEE, 0x47980610, 0x47981133, 0x47981C56, 0x47982779,
 0x4798329D, 0x47983DC0, 0x479848E4, 0x47985408, 0x47985F2C, 0x47986A50, 0x47987575, 0x4798809A,
 0x47988BBF, 0x479896E4, 0x4798A209, 0x4798AD2F, 0x4798B855, 0x4798C37B, 0x4798CEA1, 0x4798D9C7,
 0x4798E4EE, 0x4798F015, 0x4798FB3C, 0x47990663, 0x4799118B, 0x47991CB2, 0x479927DA, 0x47993302,
 0x47993E2A, 0x47994953, 0x4799547C, 0x47995FA4, 0x47996ACD, 0x479975F7, 0x47998120, 0x47998C4A,
 0x47999774, 0x4799A29E, 0x4799ADC8, 0x4799B8F3, 0x4799C41D, 0x4799CF48, 0x4799DA73, 0x4799E59E,
 0x4799F0CA, 0x4799FBF6, 0x479A0722, 0x479A124E, 0x479A1D7A, 0x479A28A6, 0x479A33D3, 0x479A3F00,
 0x479A4A2D, 0x479A555B, 0x479A6088, 0x479A6BB6, 0x479A76E4, 0x479A8212, 0x479A8D40, 0x479A986F,
 0x479AA39D, 0x479AAECC, 0x479AB9FB, 0x479AC52B, 0x479AD05A, 0x479ADB8A, 0x479AE6BA, 0x479AF1EA,
 0x479AFD1B, 0x479B084B, 0x479B137C, 0x479B1EAD, 0x479B29DE, 0x479B350F, 0x479B4041, 0x479B4B73,
 0x479B56A5, 0x479B61D7, 0x479B6D09, 0x479B783C, 0x479B836E, 0x479B8EA1, 0x479B99D5, 0x479BA508,
 0x479BB03C, 0x479BBB6F, 0x479BC6A3, 0x479BD1D7, 0x479BDD0C, 0x479BE840, 0x479BF375, 0x479BFEAA,
 0x479C09DF, 0x479C1515, 0x479C204A, 0x479C2B80, 0x479C36B6, 0x479C41EC, 0x479C4D23, 0x479C5859,
 0x479C6390, 0x479C6EC7, 0x479C79FE, 0x479C8536, 0x479C906D, 0x479C9BA5, 0x479CA6DD, 0x479CB215,
 0x479CBD4E, 0x479CC886, 0x479CD3BF, 0x479CDEF8, 0x479CEA31, 0x479CF56B, 0x479D00A4, 0x479D0BDE,
 0x479D1718, 0x479D2252, 0x479D2D8D, 0x479D38C7, 0x479D4402, 0x479D4F3D, 0x479D5A79, 0x479D65B4,
 0x479D70F0, 0x479D7C2B, 0x479D8767, 0x479D92A4, 0x479D9DE0, 0x479DA91D, 0x479DB459, 0x479DBF96,
 0x479DCAD4, 0x479DD611, 0x479DE14F, 0x479DEC8D, 0x479DF7CB, 0x479E0309, 0x479E0E47, 0x479E1986,
 0x479E24C5, 0x479E3004, 0x479E3B43, 0x479E4682, 0x479E51C2, 0x479E5D02, 0x479E6842, 0x479E7382,
 0x479E7EC2, 0x479E8A03, 0x479E9544, 0x479EA085, 0x479EABC6, 0x479EB707, 0x479EC249, 0x479ECD8B,
 0x479ED8CD, 0x479EE40F, 0x479EEF51, 0x479EFA94, 0x479F05D7, 0x479F111A, 0x479F1C5D, 0x479F27A1,
 0x479F32E4, 0x479F3E28, 0x479F496C, 0x479F54B0, 0x479F5FF5, 0x479F6B39, 0x479F767E, 0x479F81C3,
 0x479F8D08, 0x479F984E, 0x479FA393, 0x479FAED9, 0x479FBA1F, 0x479FC565, 0x479FD0AC, 0x479FDBF2,
 0x479FE739, 0x479FF280, 0x479FFDC7, 0x47A0090E, 0x47A01456, 0x47A01F9E, 0x47A02AE6, 0x47A0362E,
 0x47A04176, 0x47A04CBF, 0x47A05808, 0x47A06351, 0x47A06E9A, 0x47A079E3, 0x47A0852D, 0x47A09077,
 0x47A09BC1, 0x47A0A70B, 0x47A0B255, 0x47A0BDA0, 0x47A0C8EA, 0x47A0D435, 0x47A0DF81, 0x47A0EACC,
 0x47A0F618, 0x47A10163, 0x47A10CAF, 0x47A117FB, 0x47A12348, 0x47A12E94, 0x47A139E1, 0x47A1452E,
 0x47A1507B, 0x47A15BC8, 0x47A16716, 0x47A17264, 0x47A17DB2, 0x47A18900, 0x47A1944E, 0x47A19F9D,
 0x47A1AAEB, 0x47A1B63A, 0x47A1C189, 0x47A1CCD9, 0x47A1D828, 0x47A1E378, 0x47A1EEC8, 0x47A1FA18,
 0x47A20568, 0x47A210B9, 0x47A21C09, 0x47A2275A, 0x47A232AB, 0x47A23DFD, 0x47A2494E, 0x47A254A0,
 0x47A25FF2, 0x47A26B44, 0x47A27696, 0x47A281E9, 0x47A28D3B, 0x47A2988E, 0x47A2A3E1, 0x47A2AF35,
 0x47A2BA88, 0x47A2C5DC, 0x47A2D12F, 0x47A2DC84, 0x47A2E7D8, 0x47A2F32C, 0x47A2FE81, 0x47A309D6,
 0x47A3152B, 0x47A32080, 0x47A32BD5, 0x47A3372B, 0x47A34281, 0x47A34DD7, 0x47A3592D, 0x47A36484,
 0x47A36FDA, 0x47A37B31, 0x47A38688, 0x47A391DF, 0x47A39D37, 0x47A3A88E, 0x47A3B3E6, 0x47A3BF3E,
 0x47A3CA96, 0x47A3D5EF, 0x47A3E147, 0x47A3ECA0, 0x47A3F7F9, 0x47A40352, 0x47A40EAB, 0x47A41A05,
 0x47A4255F, 0x47A430B9, 0x47A43C13, 0x47A4476D, 0x47A452C8, 0x47A45E22, 0x47A4697D, 0x47A474D8,
 0x47A48034, 0x47A48B8F, 0x47A496EB, 0x47A4A247, 0x47A4ADA3, 0x47A4B8FF, 0x47A4C45C, 0x47A4CFB9,
 0x47A4DB15, 0x47A4E673, 0x47A4F1D0, 0x47A4FD2D, 0x47A5088B, 0x47A513E9, 0x47A51F47, 0x47A52AA5,
 0x47A53604, 0x47A54162, 0x47A54CC1, 0x47A55820, 0x47A5637F, 0x47A56EDF, 0x47A57A3E, 0x47A5859E,
 0x47A590FE, 0x47A59C5F, 0x47A5A7BF, 0x47A5B320, 0x47A5BE80, 0x47A5C9E1, 0x47A5D543, 0x47A5E0A4,
 0x47A5EC06, 0x47A5F767, 0x47A602C9, 0x47A60E2B, 0x47A6198E, 0x47A624F0, 0x47A63053, 0x47A63BB6,
 0x47A64719, 0x47A6527D, 0x47A65DE0, 0x47A66944, 0x47A674A8, 0x47A6800C, 0x47A68B70, 0x47A696D5,
 0x47A6A239, 0x47A6AD9E, 0x47A6B903, 0x47A6C469, 0x47A6CFCE, 0x47A6DB34, 0x47A6E69A, 0x47A6F200,
 0x47A6FD66, 0x47A708CD, 0x47A71433, 0x47A71F9A, 0x47A72B01, 0x47A73668, 0x47A741D0, 0x47A74D37,
 0x47A7589F, 0x47A76407, 0x47A76F6F, 0x47A77AD8, 0x47A78640, 0x47A791A9, 0x47A79D12, 0x47A7A87B,
 0x47A7B3E5, 0x47A7BF4E, 0x47A7CAB8, 0x47A7D622, 0x47A7E18C, 0x47A7ECF6, 0x47A7F861, 0x47A803CC,
 0x47A80F37, 0x47A81AA2, 0x47A8260D, 0x47A83179, 0x47A83CE4, 0x47A84850, 0x47A853BC, 0x47A85F29,
 0x47A86A95, 0x47A87602, 0x47A8816F, 0x47A88CDC, 0x47A89849, 0x47A8A3B6, 0x47A8AF24, 0x47A8BA92,
 0x47A8C600, 0x47A8D16E, 0x47A8DCDC, 0x47A8E84B, 0x47A8F3BA, 0x47A8FF29, 0x47A90A98, 0x47A91607,
 0x47A92177, 0x47A92CE7, 0x47A93857, 0x47A943C7, 0x47A94F37, 0x47A95AA8, 0x47A96618, 0x47A97189,
 0x47A97CFA, 0x47A9886C, 0x47A993DD, 0x47A99F4F, 0x47A9AAC1, 0x47A9B633, 0x47A9C1A5, 0x47A9CD18,
 0x47A9D88A, 0x47A9E3FD, 0x47A9EF70, 0x47A9FAE3, 0x47AA0657, 0x47AA11CB, 0x47AA1D3E, 0x47AA28B2,
 0x47AA3427, 0x47AA3F9B, 0x47AA4B10, 0x47AA5684, 0x47AA61F9, 0x47AA6D6F, 0x47AA78E4, 0x47AA8459,
 0x47AA8FCF, 0x47AA9B45, 0x47AAA6BB, 0x47AAB232, 0x47AABDA8, 0x47AAC91F, 0x47AAD496, 0x47AAE00D,
 0x47AAEB84, 0x47AAF6FC, 0x47AB0273, 0x47AB0DEB, 0x47AB1963, 0x47AB24DB, 0x47AB3054, 0x47AB3BCC,
 0x47AB4745, 0x47AB52BE, 0x47AB5E37, 0x47AB69B1, 0x47AB752A, 0x47AB80A4, 0x47AB8C1E, 0x47AB9798,
 0x47ABA313, 0x47ABAE8D, 0x47ABBA08, 0x47ABC583, 0x47ABD0FE, 0x47ABDC79, 0x47ABE7F5, 0x47ABF371,
 0x47ABFEED, 0x47AC0A69, 0x47AC15E5, 0x47AC2161, 0x47AC2CDE, 0x47AC385B, 0x47AC43D8, 0x47AC4F55,
 0x47AC5AD3, 0x47AC6650, 0x47AC71CE, 0x47AC7D4C, 0x47AC88CA, 0x47AC9449, 0x47AC9FC7, 0x47ACAB46,
 0x47ACB6C5, 0x47ACC244, 0x47ACCDC3, 0x47ACD943, 0x47ACE4C3, 0x47ACF043, 0x47ACFBC3, 0x47AD0743,
 0x47AD12C3, 0x47AD1E44, 0x47AD29C5, 0x47AD3546, 0x47AD40C7, 0x47AD4C49, 0x47AD57CA, 0x47AD634C,
 0x47AD6ECE, 0x47AD7A50, 0x47AD85D3, 0x47AD9155, 0x47AD9CD8, 0x47ADA85B, 0x47ADB3DE, 0x47ADBF62,
 0x47ADCAE5, 0x47ADD669, 0x47ADE1ED, 0x47ADED71, 0x47ADF8F5, 0x47AE047A, 0x47AE0FFE, 0x47AE1B83,
 0x47AE2708, 0x47AE328E, 0x47AE3E13, 0x47AE4999, 0x47AE551F, 0x47AE60A5, 0x47AE6C2B, 0x47AE77B1,
 0x47AE8338, 0x47AE8EBF, 0x47AE9A45, 0x47AEA5CD, 0x47AEB154, 0x47AEBCDC, 0x47AEC863, 0x47AED3EB,
 0x47AEDF73, 0x47AEEAFC, 0x47AEF684, 0x47AF020D, 0x47AF0D96, 0x47AF191F, 0x47AF24A8, 0x47AF3031,
 0x47AF3BBB, 0x47AF4745, 0x47AF52CF, 0x47AF5E59, 0x47AF69E3, 0x47AF756E, 0x47AF80F9, 0x47AF8C84,
 0x47AF980F, 0x47AFA39A, 0x47AFAF26, 0x47AFBAB2, 0x47AFC63D, 0x47AFD1CA, 0x47AFDD56, 0x47AFE8E2,
 0x47AFF46F, 0x47AFFFFC, 0x47B00B89, 0x47B01716, 0x47B022A4, 0x47B02E31, 0x47B039BF, 0x47B0454D,
 0x47B050DB, 0x47B05C6A, 0x47B067F8, 0x47B07387, 0x47B07F16, 0x47B08AA5, 0x47B09634, 0x47B0A1C4,
 0x47B0AD53, 0x47B0B8E3, 0x47B0C473, 0x47B0D004, 0x47B0DB94, 0x47B0E725, 0x47B0F2B6, 0x47B0FE47,
 0x47B109D8, 0x47B11569, 0x47B120FB, 0x47B12C8D, 0x47B1381F, 0x47B143B1, 0x47B14F43, 0x47B15AD6,
 0x47B16668, 0x47B171FB, 0x47B17D8E, 0x47B18922, 0x47B194B5, 0x47B1A049, 0x47B1ABDD, 0x47B1B771,
 0x47B1C305, 0x47B1CE99, 0x47B1DA2E, 0x47B1E5C3, 0x47B1F158, 0x47B1FCED, 0x47B20882, 0x47B21418,
 0x47B21FAD, 0x47B22B43, 0x47B236DA, 0x47B24270, 0x47B24E06, 0x47B2599D, 0x47B26534, 0x47B270CB,
 0x47B27C62, 0x47B287FA, 0x47B29391, 0x47B29F29, 0x47B2AAC1, 0x47B2B659, 0x47B2C1F2, 0x47B2CD8A,
 0x47B2D923, 0x47B2E4BC, 0x47B2F055, 0x47B2FBEE, 0x47B30788, 0x47B31321, 0x47B31EBB, 0x47B32A55,
 0x47B335F0, 0x47B3418A, 0x47B34D25, 0x47B358C0, 0x47B3645B, 0x47B36FF6, 0x47B37B91, 0x47B3872D,
 0x47B392C8, 0x47B39E64, 0x47B3AA01, 0x47B3B59D, 0x47B3C139, 0x47B3CCD6, 0x47B3D873, 0x47B3E410,
 0x47B3EFAD, 0x47B3FB4B, 0x47B406E8, 0x47B41286, 0x47B41E24, 0x47B429C2, 0x47B43561, 0x47B440FF,
 0x47B44C9E, 0x47B4583D, 0x47B463DC, 0x47B46F7B, 0x47B47B1B, 0x47B486BB, 0x47B4925A, 0x47B49DFB,
 0x47B4A99B, 0x47B4B53B, 0x47B4C0DC, 0x47B4CC7D, 0x47B4D81E, 0x47B4E3BF, 0x47B4EF60, 0x47B4FB02,
 0x47B506A4, 0x47B51245, 0x47B51DE8, 0x47B5298A, 0x47B5352C, 0x47B540CF, 0x47B54C72, 0x47B55815,
 0x47B563B8, 0x47B56F5C, 0x47B57AFF, 0x47B586A3, 0x47B59247, 0x47B59DEB, 0x47B5A990, 0x47B5B534,
 0x47B5C0D9, 0x47B5CC7E, 0x47B5D823, 0x47B5E3C8, 0x47B5EF6E, 0x47B5FB13, 0x47B606B9, 0x47B6125F,
 0x47B61E05, 0x47B629AC, 0x47B63552, 0x47B640F9, 0x47B64CA0, 0x47B65847, 0x47B663EF, 0x47B66F96,
 0x47B67B3E, 0x47B686E6, 0x47B6928E, 0x47B69E36, 0x47B6A9DF, 0x47B6B587, 0x47B6C130, 0x47B6CCD9,
 0x47B6D882, 0x47B6E42C, 0x47B6EFD5, 0x47B6FB7F, 0x47B70729, 0x47B712D3, 0x47B71E7E, 0x47B72A28,
 0x47B735D3, 0x47B7417E, 0x47B74D29, 0x47B758D4, 0x47B7647F, 0x47B7702B, 0x47B77BD7, 0x47B78783,
 0x47B7932F, 0x47B79EDB, 0x47B7AA88, 0x47B7B635, 0x47B7C1E1, 0x47B7CD8F, 0x47B7D93C, 0x47B7E4E9,
 0x47B7F097, 0x47B7FC45, 0x47B807F3, 0x47B813A1, 0x47B81F4F, 0x47B82AFE, 0x47B836AD, 0x47B8425C,
 0x47B84E0B, 0x47B859BA, 0x47B8656A, 0x47B87119, 0x47B87CC9, 0x47B88879, 0x47B8942A, 0x47B89FDA,
 0x47B8AB8B, 0x47B8B73C, 0x47B8C2ED, 0x47B8CE9E, 0x47B8DA4F, 0x47B8E601, 0x47B8F1B2, 0x47B8FD64,
 0x47B90916, 0x47B914C9, 0x47B9207B, 0x47B92C2E, 0x47B937E1, 0x47B94394, 0x47B94F47, 0x47B95AFA,
 0x47B966AE, 0x47B97262, 0x47B97E16, 0x47B989CA, 0x47B9957E, 0x47B9A132, 0x47B9ACE7, 0x47B9B89C,
 0x47B9C451, 0x47B9D006, 0x47B9DBBC, 0x47B9E771, 0x47B9F327, 0x47B9FEDD, 0x47BA0A93, 0x47BA164A,
 0x47BA2200, 0x47BA2DB7, 0x47BA396E, 0x47BA4525, 0x47BA50DC, 0x47BA5C94, 0x47BA684B, 0x47BA7403,
 0x47BA7FBB, 0x47BA8B73, 0x47BA972C, 0x47BAA2E4, 0x47BAAE9D, 0x47BABA56, 0x47BAC60F, 0x47BAD1C8,
 0x47BADD82, 0x47BAE93B, 0x47BAF4F5, 0x47BB00AF, 0x47BB0C69, 0x47BB1824, 0x47BB23DE, 0x47BB2F99,
 0x47BB3B54, 0x47BB470F, 0x47BB52CB, 0x47BB5E86, 0x47BB6A42, 0x47BB75FE, 0x47BB81BA, 0x47BB8D76,
 0x47BB9932, 0x47BBA4EF, 0x47BBB0AC, 0x47BBBC69, 0x47BBC826, 0x47BBD3E3, 0x47BBDFA1, 0x47BBEB5E,
 0x47BBF71C, 0x47BC02DA, 0x47BC0E99, 0x47BC1A57, 0x47BC2616, 0x47BC31D4, 0x47BC3D93, 0x47BC4953,
 0x47BC5512, 0x47BC60D2, 0x47BC6C91, 0x47BC7851, 0x47BC8411, 0x47BC8FD2, 0x47BC9B92, 0x47BCA753,
 0x47BCB313, 0x47BCBED4, 0x47BCCA96, 0x47BCD657, 0x47BCE219, 0x47BCEDDA, 0x47BCF99C, 0x47BD055E,
 0x47BD1121, 0x47BD1CE3, 0x47BD28A6, 0x47BD3468, 0x47BD402B, 0x47BD4BEF, 0x47BD57B2, 0x47BD6376,
 0x47BD6F39, 0x47BD7AFD, 0x47BD86C1, 0x47BD9286, 0x47BD9E4A, 0x47BDAA0F, 0x47BDB5D4, 0x47BDC199,
 0x47BDCD5E, 0x47BDD923, 0x47BDE4E9, 0x47BDF0AF, 0x47BDFC74, 0x47BE083B, 0x47BE1401, 0x47BE1FC7,
 0x47BE2B8E, 0x47BE3755, 0x47BE431C, 0x47BE4EE3, 0x47BE5AAB, 0x47BE6672, 0x47BE723A, 0x47BE7E02,
 0x47BE89CA, 0x47BE9592, 0x47BEA15B, 0x47BEAD23, 0x47BEB8EC, 0x47BEC4B5, 0x47BED07E, 0x47BEDC48,
 0x47BEE811, 0x47BEF3DB, 0x47BEFFA5, 0x47BF0B6F, 0x47BF173A, 0x47BF2304, 0x47BF2ECF, 0x47BF3A9A,
 0x47BF4665, 0x47BF5230, 0x47BF5DFB, 0x47BF69C7, 0x47BF7593, 0x47BF815E, 0x47BF8D2B, 0x47BF98F7,
 0x47BFA4C3, 0x47BFB090, 0x47BFBC5D, 0x47BFC82A, 0x47BFD3F7, 0x47BFDFC5, 0x47BFEB92, 0x47BFF760,
 0x47C0032E, 0x47C00EFC, 0x47C01ACA, 0x47C02699, 0x47C03267, 0x47C03E36, 0x47C04A05, 0x47C055D4,
 0x47C061A4, 0x47C06D73, 0x47C07943, 0x47C08513, 0x47C090E3, 0x47C09CB3, 0x47C0A884, 0x47C0B455,
 0x47C0C025, 0x47C0CBF6, 0x47C0D7C8, 0x47C0E399, 0x47C0EF6B, 0x47C0FB3C, 0x47C1070E, 0x47C112E0,
 0x47C11EB3, 0x47C12A85, 0x47C13658, 0x47C1422B, 0x47C14DFE, 0x47C159D1, 0x47C165A4, 0x47C17178,
 0x47C17D4B, 0x47C1891F, 0x47C194F3, 0x47C1A0C8, 0x47C1AC9C, 0x47C1B871, 0x47C1C446, 0x47C1D01B,
 0x47C1DBF0, 0x47C1E7C5, 0x47C1F39B, 0x47C1FF70, 0x47C20B46, 0x47C2171C, 0x47C222F3, 0x47C22EC9,
 0x47C23AA0, 0x47C24677, 0x47C2524E, 0x47C25E25, 0x47C269FC, 0x47C275D4, 0x47C281AB, 0x47C28D83,
 0x47C2995B, 0x47C2A533, 0x47C2B10C, 0x47C2BCE5, 0x47C2C8BD, 0x47C2D496, 0x47C2E06F, 0x47C2EC49,
 0x47C2F822, 0x47C303FC, 0x47C30FD6, 0x47C31BB0, 0x47C3278A, 0x47C33364, 0x47C33F3F, 0x47C34B1A,
 0x47C356F5, 0x47C362D0, 0x47C36EAB, 0x47C37A87, 0x47C38662, 0x47C3923E, 0x47C39E1A, 0x47C3A9F6,
 0x47C3B5D3, 0x47C3C1AF, 0x47C3CD8C, 0x47C3D969, 0x47C3E546, 0x47C3F123, 0x47C3FD01, 0x47C408DF,
 0x47C414BC, 0x47C4209A, 0x47C42C79, 0x47C43857, 0x47C44435, 0x47C45014, 0x47C45BF3, 0x47C467D2,
 0x47C473B1, 0x47C47F91, 0x47C48B70, 0x47C49750, 0x47C4A330, 0x47C4AF10, 0x47C4BAF1, 0x47C4C6D1,
 0x47C4D2B2, 0x47C4DE93, 0x47C4EA74, 0x47C4F655, 0x47C50236, 0x47C50E18, 0x47C519FA, 0x47C525DC,
 0x47C531BE, 0x47C53DA0, 0x47C54983, 0x47C55565, 0x47C56148, 0x47C56D2B, 0x47C5790E, 0x47C584F2,
 0x47C590D5, 0x47C59CB9, 0x47C5A89D, 0x47C5B481, 0x47C5C065, 0x47C5CC4A, 0x47C5D82E, 0x47C5E413,
 0x47C5EFF8, 0x47C5FBDD, 0x47C607C3, 0x47C613A8, 0x47C61F8E, 0x47C62B74, 0x47C6375A, 0x47C64340,
 0x47C64F27, 0x47C65B0D, 0x47C666F4, 0x47C672DB, 0x47C67EC2, 0x47C68AA9, 0x47C69691, 0x47C6A278,
 0x47C6AE60, 0x47C6BA48, 0x47C6C631, 0x47C6D219, 0x47C6DE01, 0x47C6E9EA, 0x47C6F5D3, 0x47C701BC,
 0x47C70DA5, 0x47C7198F, 0x47C72579, 0x47C73162, 0x47C73D4C, 0x47C74936, 0x47C75521, 0x47C7610B,
 0x47C76CF6, 0x47C778E1, 0x47C784CC, 0x47C790B7, 0x47C79CA3, 0x47C7A88E, 0x47C7B47A, 0x47C7C066,
 0x47C7CC52, 0x47C7D83E, 0x47C7E42B, 0x47C7F017, 0x47C7FC04, 0x47C807F1, 0x47C813DE, 0x47C81FCC,
 0x47C82BB9, 0x47C837A7, 0x47C84395, 0x47C84F83, 0x47C85B71, 0x47C86760, 0x47C8734E, 0x47C87F3D,
 0x47C88B2C, 0x47C8971B, 0x47C8A30B, 0x47C8AEFA, 0x47C8BAEA, 0x47C8C6DA, 0x47C8D2CA, 0x47C8DEBA,
 0x47C8EAAA, 0x47C8F69B, 0x47C9028B, 0x47C90E7C, 0x47C91A6D, 0x47C9265F, 0x47C93250, 0x47C93E42,
 0x47C94A34, 0x47C95626, 0x47C96218, 0x47C96E0A, 0x47C979FD, 0x47C985EF, 0x47C991E2, 0x47C99DD5,
 0x47C9A9C8, 0x47C9B5BC, 0x47C9C1AF, 0x47C9CDA3, 0x47C9D997, 0x47C9E58B, 0x47C9F17F, 0x47C9FD74,
 0x47CA0968, 0x47CA155D, 0x47CA2152, 0x47CA2D47, 0x47CA393D, 0x47CA4532, 0x47CA5128, 0x47CA5D1E,
 0x47CA6914, 0x47CA750A, 0x47CA8101, 0x47CA8CF7, 0x47CA98EE, 0x47CAA4E5, 0x47CAB0DC, 0x47CABCD3,
 0x47CAC8CB, 0x47CAD4C2, 0x47CAE0BA, 0x47CAECB2, 0x47CAF8AA, 0x47CB04A3, 0x47CB109B, 0x47CB1C94,
 0x47CB288D, 0x47CB3486, 0x47CB407F, 0x47CB4C78, 0x47CB5872, 0x47CB646C, 0x47CB7066, 0x47CB7C60,
 0x47CB885A, 0x47CB9454, 0x47CBA04F, 0x47CBAC4A, 0x47CBB845, 0x47CBC440, 0x47CBD03B, 0x47CBDC37,
 0x47CBE833, 0x47CBF42E, 0x47CC002B, 0x47CC0C27, 0x47CC1823, 0x47CC2420, 0x47CC301C, 0x47CC3C19,
 0x47CC4816, 0x47CC5414, 0x47CC6011, 0x47CC6C0F, 0x47CC780D, 0x47CC840B, 0x47CC9009, 0x47CC9C07,
 0x47CCA806, 0x47CCB404, 0x47CCC003, 0x47CCCC02, 0x47CCD801, 0x47CCE401, 0x47CCF000, 0x47CCFC00,
 0x47CD0800, 0x47CD1400, 0x47CD2000, 0x47CD2C01, 0x47CD3801, 0x47CD4402, 0x47CD5003, 0x47CD5C04,
 0x47CD6806, 0x47CD7407, 0x47CD8009, 0x47CD8C0B, 0x47CD980D, 0x47CDA40F, 0x47CDB011, 0x47CDBC14,
 0x47CDC816, 0x47CDD419, 0x47CDE01C, 0x47CDEC20, 0x47CDF823, 0x47CE0427, 0x47CE102A, 0x47CE1C2E,
 0x47CE2833, 0x47CE3437, 0x47CE403B, 0x47CE4C40, 0x47CE5845, 0x47CE644A, 0x47CE704F, 0x47CE7C54,
 0x47CE885A, 0x47CE945F, 0x47CEA065, 0x47CEAC6B, 0x47CEB872, 0x47CEC478, 0x47CED07F, 0x47CEDC85,
 0x47CEE88C, 0x47CEF493, 0x47CF009B, 0x47CF0CA2, 0x47CF18AA, 0x47CF24B1, 0x47CF30B9, 0x47CF3CC2,
 0x47CF48CA, 0x47CF54D2, 0x47CF60DB, 0x47CF6CE4, 0x47CF78ED, 0x47CF84F6, 0x47CF90FF, 0x47CF9D09,
 0x47CFA913, 0x47CFB51D, 0x47CFC127, 0x47CFCD31, 0x47CFD93B, 0x47CFE546, 0x47CFF151, 0x47CFFD5C,
 0x47D00967, 0x47D01572, 0x47D0217D, 0x47D02D89, 0x47D03995, 0x47D045A1, 0x47D051AD, 0x47D05DB9,
 0x47D069C6, 0x47D075D3, 0x47D081DF, 0x47D08DEC, 0x47D099FA, 0x47D0A607, 0x47D0B215, 0x47D0BE22,
 0x47D0CA30, 0x47D0D63E, 0x47D0E24C, 0x47D0EE5B, 0x47D0FA69, 0x47D10678, 0x47D11287, 0x47D11E96,
 0x47D12AA6, 0x47D136B5, 0x47D142C5, 0x47D14ED4, 0x47D15AE4, 0x47D166F5, 0x47D17305, 0x47D17F15,
 0x47D18B26, 0x47D19737, 0x47D1A348, 0x47D1AF59, 0x47D1BB6B, 0x47D1C77C, 0x47D1D38E, 0x47D1DFA0,
 0x47D1EBB2, 0x47D1F7C4, 0x47D203D6, 0x47D20FE9, 0x47D21BFC, 0x47D2280F, 0x47D23422, 0x47D24035,
 0x47D24C49, 0x47D2585C, 0x47D26470, 0x47D27084, 0x47D27C98, 0x47D288AC, 0x47D294C1, 0x47D2A0D6,
 0x47D2ACEA, 0x47D2B8FF, 0x47D2C515, 0x47D2D12A, 0x47D2DD40, 0x47D2E955, 0x47D2F56B, 0x47D30181,
 0x47D30D97, 0x47D319AE, 0x47D325C4, 0x47D331DB, 0x47D33DF2, 0x47D34A09, 0x47D35620, 0x47D36238,
 0x47D36E4F, 0x47D37A67, 0x47D3867F, 0x47D39297, 0x47D39EB0, 0x47D3AAC8, 0x47D3B6E1, 0x47D3C2FA,
 0x47D3CF13, 0x47D3DB2C, 0x47D3E745, 0x47D3F35F, 0x47D3FF78, 0x47D40B92, 0x47D417AC, 0x47D423C6,
 0x47D42FE1, 0x47D43BFB, 0x47D44816, 0x47D45431, 0x47D4604C, 0x47D46C67, 0x47D47883, 0x47D4849E,
 0x47D490BA, 0x47D49CD6, 0x47D4A8F2, 0x47D4B50E, 0x47D4C12B, 0x47D4CD47, 0x47D4D964, 0x47D4E581,
 0x47D4F19E, 0x47D4FDBB, 0x47D509D9, 0x47D515F7, 0x47D52214, 0x47D52E32, 0x47D53A51, 0x47D5466F,
 0x47D5528D, 0x47D55EAC, 0x47D56ACB, 0x47D576EA, 0x47D58309, 0x47D58F29, 0x47D59B48, 0x47D5A768,
 0x47D5B388, 0x47D5BFA8, 0x47D5CBC8, 0x47D5D7E8, 0x47D5E409, 0x47D5F02A, 0x47D5FC4B, 0x47D6086C,
 0x47D6148D, 0x47D620AE, 0x47D62CD0, 0x47D638F2, 0x47D64514, 0x47D65136, 0x47D65D58, 0x47D6697B,
 0x47D6759D, 0x47D681C0, 0x47D68DE3, 0x47D69A06, 0x47D6A62A, 0x47D6B24D, 0x47D6BE71, 0x47D6CA95,
 0x47D6D6B9, 0x47D6E2DD, 0x47D6EF01, 0x47D6FB26, 0x47D7074A, 0x47D7136F, 0x47D71F94, 0x47D72BB9,
 0x47D737DF, 0x47D74404, 0x47D7502A, 0x47D75C50, 0x47D76876, 0x47D7749C, 0x47D780C3, 0x47D78CE9,
 0x47D79910, 0x47D7A537, 0x47D7B15E, 0x47D7BD85, 0x47D7C9AD, 0x47D7D5D4, 0x47D7E1FC, 0x47D7EE24,
 0x47D7FA4C, 0x47D80674, 0x47D8129D, 0x47D81EC6, 0x47D82AEE, 0x47D83717, 0x47D84341, 0x47D84F6A,
 0x47D85B93, 0x47D867BD, 0x47D873E7, 0x47D88011, 0x47D88C3B, 0x47D89865, 0x47D8A490, 0x47D8B0BB,
 0x47D8BCE5, 0x47D8C910, 0x47D8D53C, 0x47D8E167, 0x47D8ED93, 0x47D8F9BE, 0x47D905EA, 0x47D91216,
 0x47D91E42, 0x47D92A6F, 0x47D9369B, 0x47D942C8, 0x47D94EF5, 0x47D95B22, 0x47D9674F, 0x47D9737D,
 0x47D97FAA, 0x47D98BD8, 0x47D99806, 0x47D9A434, 0x47D9B062, 0x47D9BC91, 0x47D9C8BF, 0x47D9D4EE,
 0x47D9E11D, 0x47D9ED4C, 0x47D9F97C, 0x47DA05AB, 0x47DA11DB, 0x47DA1E0A, 0x47DA2A3A, 0x47DA366B,
 0x47DA429B, 0x47DA4ECB, 0x47DA5AFC, 0x47DA672D, 0x47DA735E, 0x47DA7F8F, 0x47DA8BC0, 0x47DA97F2,
 0x47DAA423, 0x47DAB055, 0x47DABC87, 0x47DAC8BA, 0x47DAD4EC, 0x47DAE11E, 0x47DAED51, 0x47DAF984,
 0x47DB05B7, 0x47DB11EA, 0x47DB1E1E, 0x47DB2A51, 0x47DB3685, 0x47DB42B9, 0x47DB4EED, 0x47DB5B21,
 0x47DB6755, 0x47DB738A, 0x47DB7FBF, 0x47DB8BF4, 0x47DB9829, 0x47DBA45E, 0x47DBB093, 0x47DBBCC9,
 0x47DBC8FF, 0x47DBD534, 0x47DBE16B, 0x47DBEDA1, 0x47DBF9D7, 0x47DC060E, 0x47DC1245, 0x47DC1E7C,
 0x47DC2AB3, 0x47DC36EA, 0x47DC4321, 0x47DC4F59, 0x47DC5B91, 0x47DC67C9, 0x47DC7401, 0x47DC8039,
 0x47DC8C72, 0x47DC98AA, 0x47DCA4E3, 0x47DCB11C, 0x47DCBD55, 0x47DCC98E, 0x47DCD5C8, 0x47DCE202,
 0x47DCEE3B, 0x47DCFA75, 0x47DD06B0, 0x47DD12EA, 0x47DD1F24, 0x47DD2B5F, 0x47DD379A, 0x47DD43D5,
 0x47DD5010, 0x47DD5C4B, 0x47DD6887, 0x47DD74C2, 0x47DD80FE, 0x47DD8D3A, 0x47DD9977, 0x47DDA5B3,
 0x47DDB1EF, 0x47DDBE2C, 0x47DDCA69, 0x47DDD6A6, 0x47DDE2E3, 0x47DDEF20, 0x47DDFB5E, 0x47DE079C,
 0x47DE13DA, 0x47DE2018, 0x47DE2C56, 0x47DE3894, 0x47DE44D3, 0x47DE5111, 0x47DE5D50, 0x47DE698F,
 0x47DE75CF, 0x47DE820E, 0x47DE8E4E, 0x47DE9A8D, 0x47DEA6CD, 0x47DEB30D, 0x47DEBF4D, 0x47DECB8E,
 0x47DED7CE, 0x47DEE40F, 0x47DEF050, 0x47DEFC91, 0x47DF08D2, 0x47DF1514, 0x47DF2155, 0x47DF2D97,
 0x47DF39D9, 0x47DF461B, 0x47DF525D, 0x47DF5EA0, 0x47DF6AE2, 0x47DF7725, 0x47DF8368, 0x47DF8FAB,
 0x47DF9BEE, 0x47DFA832, 0x47DFB475, 0x47DFC0B9, 0x47DFCCFD, 0x47DFD941, 0x47DFE585, 0x47DFF1CA,
 0x47DFFE0E, 0x47E00A53, 0x47E01698, 0x47E022DD, 0x47E02F23, 0x47E03B68, 0x47E047AE, 0x47E053F3,
 0x47E06039, 0x47E06C7F, 0x47E078C6, 0x47E0850C, 0x47E09153, 0x47E09D9A, 0x47E0A9E1, 0x47E0B628,
 0x47E0C26F, 0x47E0CEB6, 0x47E0DAFE, 0x47E0E746, 0x47E0F38E, 0x47E0FFD6, 0x47E10C1E, 0x47E11867,
 0x47E124AF, 0x47E130F8, 0x47E13D41, 0x47E1498A, 0x47E155D3, 0x47E1621D, 0x47E16E67, 0x47E17AB0,
 0x47E186FA, 0x47E19344, 0x47E19F8F, 0x47E1ABD9, 0x47E1B824, 0x47E1C46F, 0x47E1D0BA, 0x47E1DD05,
 0x47E1E950, 0x47E1F59C, 0x47E201E7, 0x47E20E33, 0x47E21A7F, 0x47E226CB, 0x47E23317, 0x47E23F64,
 0x47E24BB1, 0x47E257FD, 0x47E2644A, 0x47E27098, 0x47E27CE5, 0x47E28932, 0x47E29580, 0x47E2A1CE,
 0x47E2AE1C, 0x47E2BA6A, 0x47E2C6B8, 0x47E2D307, 0x47E2DF55, 0x47E2EBA4, 0x47E2F7F3, 0x47E30442,
 0x47E31092, 0x47E31CE1, 0x47E32931, 0x47E33581, 0x47E341D1, 0x47E34E21, 0x47E35A71, 0x47E366C2,
 0x47E37312, 0x47E37F63, 0x47E38BB4, 0x47E39805, 0x47E3A456, 0x47E3B0A8, 0x47E3BCFA, 0x47E3C94B,
 0x47E3D59D, 0x47E3E1F0, 0x47E3EE42, 0x47E3FA94, 0x47E406E7, 0x47E4133A, 0x47E41F8D, 0x47E42BE0,
 0x47E43833, 0x47E44487, 0x47E450DA, 0x47E45D2E, 0x47E46982, 0x47E475D6, 0x47E4822B, 0x47E48E7F,
 0x47E49AD4, 0x47E4A729, 0x47E4B37E, 0x47E4BFD3, 0x47E4CC28, 0x47E4D87D, 0x47E4E4D3, 0x47E4F129,
 0x47E4FD7F, 0x47E509D5, 0x47E5162B, 0x47E52282, 0x47E52ED8, 0x47E53B2F, 0x47E54786, 0x47E553DD,
 0x47E56035, 0x47E56C8C, 0x47E578E4, 0x47E5853C, 0x47E59194, 0x47E59DEC, 0x47E5AA44, 0x47E5B69D,
 0x47E5C2F5, 0x47E5CF4E, 0x47E5DBA7, 0x47E5E800, 0x47E5F459, 0x47E600B3, 0x47E60D0C, 0x47E61966,
 0x47E625C0, 0x47E6321A, 0x47E63E75, 0x47E64ACF, 0x47E6572A, 0x47E66384, 0x47E66FDF, 0x47E67C3B,
 0x47E68896, 0x47E694F1, 0x47E6A14D, 0x47E6ADA9, 0x47E6BA05, 0x47E6C661, 0x47E6D2BD, 0x47E6DF19,
 0x47E6EB76, 0x47E6F7D3, 0x47E70430, 0x47E7108D, 0x47E71CEA, 0x47E72948, 0x47E735A5, 0x47E74203,
 0x47E74E61, 0x47E75ABF, 0x47E7671D, 0x47E7737C, 0x47E77FDA, 0x47E78C39, 0x47E79898, 0x47E7A4F7,
 0x47E7B156, 0x47E7BDB6, 0x47E7CA15, 0x47E7D675, 0x47E7E2D5, 0x47E7EF35, 0x47E7FB95, 0x47E807F6,
 0x47E81456, 0x47E820B7, 0x47E82D18, 0x47E83979, 0x47E845DA, 0x47E8523C, 0x47E85E9D, 0x47E86AFF,
 0x47E87761, 0x47E883C3, 0x47E89025, 0x47E89C88, 0x47E8A8EA, 0x47E8B54D, 0x47E8C1B0, 0x47E8CE13,
 0x47E8DA76, 0x47E8E6D9, 0x47E8F33D, 0x47E8FFA1, 0x47E90C04, 0x47E91868, 0x47E924CD, 0x47E93131,
 0x47E93D96, 0x47E949FA, 0x47E9565F, 0x47E962C4, 0x47E96F29, 0x47E97B8F, 0x47E987F4, 0x47E9945A,
 0x47E9A0C0, 0x47E9AD26, 0x47E9B98C, 0x47E9C5F2, 0x47E9D259, 0x47E9DEBF, 0x47E9EB26, 0x47E9F78D,
 0x47EA03F4, 0x47EA105C, 0x47EA1CC3, 0x47EA292B, 0x47EA3593, 0x47EA41FB, 0x47EA4E63, 0x47EA5ACB,
 0x47EA6734, 0x47EA739C, 0x47EA8005, 0x47EA8C6E, 0x47EA98D7, 0x47EAA541, 0x47EAB1AA, 0x47EABE14,
 0x47EACA7D, 0x47EAD6E7, 0x47EAE351, 0x47EAEFBC, 0x47EAFC26, 0x47EB0891, 0x47EB14FC, 0x47EB2167,
 0x47EB2DD2, 0x47EB3A3D, 0x47EB46A8, 0x47EB5314, 0x47EB5F80, 0x47EB6BEC, 0x47EB7858, 0x47EB84C4,
 0x47EB9130, 0x47EB9D9D, 0x47EBAA0A, 0x47EBB677, 0x47EBC2E4, 0x47EBCF51, 0x47EBDBBE, 0x47EBE82C,
 0x47EBF49A, 0x47EC0107, 0x47EC0D76, 0x47EC19E4, 0x47EC2652, 0x47EC32C1, 0x47EC3F2F, 0x47EC4B9E,
 0x47EC580D, 0x47EC647C, 0x47EC70EC, 0x47EC7D5B, 0x47EC89CB, 0x47EC963B, 0x47ECA2AB, 0x47ECAF1B,
 0x47ECBB8B, 0x47ECC7FC, 0x47ECD46D, 0x47ECE0DD, 0x47ECED4E, 0x47ECF9C0, 0x47ED0631, 0x47ED12A2,
 0x47ED1F14, 0x47ED2B86, 0x47ED37F8, 0x47ED446A, 0x47ED50DC, 0x47ED5D4F, 0x47ED69C1, 0x47ED7634,
 0x47ED82A7, 0x47ED8F1A, 0x47ED9B8D, 0x47EDA801, 0x47EDB475, 0x47EDC0E8, 0x47EDCD5C, 0x47EDD9D0,
 0x47EDE645, 0x47EDF2B9, 0x47EDFF2E, 0x47EE0BA2, 0x47EE1817, 0x47EE248C, 0x47EE3102, 0x47EE3D77,
 0x47EE49ED, 0x47EE5662, 0x47EE62D8, 0x47EE6F4E, 0x47EE7BC4, 0x47EE883B, 0x47EE94B1, 0x47EEA128,
 0x47EEAD9F, 0x47EEBA16, 0x47EEC68D, 0x47EED304, 0x47EEDF7C, 0x47EEEBF4, 0x47EEF86B, 0x47EF04E3,
 0x47EF115C, 0x47EF1DD4, 0x47EF2A4C, 0x47EF36C5, 0x47EF433E, 0x47EF4FB7, 0x47EF5C30, 0x47EF68A9,
 0x47EF7523, 0x47EF819C, 0x47EF8E16, 0x47EF9A90, 0x47EFA70A, 0x47EFB385, 0x47EFBFFF, 0x47EFCC7A,
 0x47EFD8F4, 0x47EFE56F, 0x47EFF1EA, 0x47EFFE66, 0x47F00AE1, 0x47F0175D, 0x47F023D8, 0x47F03054,
 0x47F03CD0, 0x47F0494D, 0x47F055C9, 0x47F06246, 0x47F06EC2, 0x47F07B3F, 0x47F087BC, 0x47F09439,
 0x47F0A0B7, 0x47F0AD34, 0x47F0B9B2, 0x47F0C630, 0x47F0D2AE, 0x47F0DF2C, 0x47F0EBAA, 0x47F0F829,
 0x47F104A7, 0x47F11126, 0x47F11DA5, 0x47F12A24, 0x47F136A4, 0x47F14323, 0x47F14FA3, 0x47F15C23,
 0x47F168A3, 0x47F17523, 0x47F181A3, 0x47F18E23, 0x47F19AA4, 0x47F1A725, 0x47F1B3A6, 0x47F1C027,
 0x47F1CCA8, 0x47F1D929, 0x47F1E5AB, 0x47F1F22D, 0x47F1FEAF, 0x47F20B31, 0x47F217B3, 0x47F22435,
 0x47F230B8, 0x47F23D3A, 0x47F249BD, 0x47F25640, 0x47F262C4, 0x47F26F47, 0x47F27BCA, 0x47F2884E,
 0x47F294D2, 0x47F2A156, 0x47F2ADDA, 0x47F2BA5E, 0x47F2C6E3, 0x47F2D367, 0x47F2DFEC, 0x47F2EC71,
 0x47F2F8F6, 0x47F3057C, 0x47F31201, 0x47F31E87, 0x47F32B0C, 0x47F33792, 0x47F34418, 0x47F3509F,
 0x47F35D25, 0x47F369AC, 0x47F37632, 0x47F382B9, 0x47F38F40, 0x47F39BC8, 0x47F3A84F, 0x47F3B4D6,
 0x47F3C15E, 0x47F3CDE6, 0x47F3DA6E, 0x47F3E6F6, 0x47F3F37F, 0x47F40007, 0x47F40C90, 0x47F41919,
 0x47F425A2, 0x47F4322B, 0x47F43EB4, 0x47F44B3E, 0x47F457C7, 0x47F46451, 0x47F470DB, 0x47F47D65,
 0x47F489EF, 0x47F4967A, 0x47F4A304, 0x47F4AF8F, 0x47F4BC1A, 0x47F4C8A5, 0x47F4D530, 0x47F4E1BC,
 0x47F4EE47, 0x47F4FAD3, 0x47F5075F, 0x47F513EB, 0x47F52077, 0x47F52D03, 0x47F53990, 0x47F5461D,
 0x47F552AA, 0x47F55F37, 0x47F56BC4, 0x47F57851, 0x47F584DF, 0x47F5916C, 0x47F59DFA, 0x47F5AA88,
 0x47F5B716, 0x47F5C3A4, 0x47F5D033, 0x47F5DCC1, 0x47F5E950, 0x47F5F5DF, 0x47F6026E, 0x47F60EFD,
 0x47F61B8D, 0x47F6281C, 0x47F634AC, 0x47F6413C, 0x47F64DCC, 0x47F65A5C, 0x47F666ED, 0x47F6737D,
 0x47F6800E, 0x47F68C9F, 0x47F69930, 0x47F6A5C1, 0x47F6B252, 0x47F6BEE4, 0x47F6CB75, 0x47F6D807,
 0x47F6E499, 0x47F6F12B, 0x47F6FDBE, 0x47F70A50, 0x47F716E3, 0x47F72376, 0x47F73008, 0x47F73C9C,
 0x47F7492F, 0x47F755C2, 0x47F76256, 0x47F76EEA, 0x47F77B7D, 0x47F78811, 0x47F794A6, 0x47F7A13A,
 0x47F7ADCF, 0x47F7BA63, 0x47F7C6F8, 0x47F7D38D, 0x47F7E022, 0x47F7ECB8, 0x47F7F94D, 0x47F805E3,
 0x47F81279, 0x47F81F0F, 0x47F82BA5, 0x47F8383B, 0x47F844D1, 0x47F85168, 0x47F85DFF, 0x47F86A96,
 0x47F8772D, 0x47F883C4, 0x47F8905C, 0x47F89CF3, 0x47F8A98B, 0x47F8B623, 0x47F8C2BB, 0x47F8CF53,
 0x47F8DBEB, 0x47F8E884, 0x47F8F51C, 0x47F901B5, 0x47F90E4E, 0x47F91AE7, 0x47F92781, 0x47F9341A,
 0x47F940B4, 0x47F94D4E, 0x47F959E8, 0x47F96682, 0x47F9731C, 0x47F97FB6, 0x47F98C51, 0x47F998EC,
 0x47F9A587, 0x47F9B222, 0x47F9BEBD, 0x47F9CB58, 0x47F9D7F4, 0x47F9E490, 0x47F9F12C, 0x47F9FDC8,
 0x47FA0A64, 0x47FA1700, 0x47FA239D, 0x47FA3039, 0x47FA3CD6, 0x47FA4973, 0x47FA5610, 0x47FA62AD,
 0x47FA6F4B, 0x47FA7BE9, 0x47FA8886, 0x47FA9524, 0x47FAA1C2, 0x47FAAE61, 0x47FABAFF, 0x47FAC79E,
 0x47FAD43C, 0x47FAE0DB, 0x47FAED7A, 0x47FAFA19, 0x47FB06B9, 0x47FB1358, 0x47FB1FF8, 0x47FB2C98,
 0x47FB3938, 0x47FB45D8, 0x47FB5278, 0x47FB5F19, 0x47FB6BB9, 0x47FB785A, 0x47FB84FB, 0x47FB919C,
 0x47FB9E3D, 0x47FBAADF, 0x47FBB780, 0x47FBC422, 0x47FBD0C4, 0x47FBDD66, 0x47FBEA08, 0x47FBF6AB,
 0x47FC034D, 0x47FC0FF0, 0x47FC1C93, 0x47FC2936, 0x47FC35D9, 0x47FC427C, 0x47FC4F20, 0x47FC5BC3,
 0x47FC6867, 0x47FC750B, 0x47FC81AF, 0x47FC8E54, 0x47FC9AF8, 0x47FCA79D, 0x47FCB441, 0x47FCC0E6,
 0x47FCCD8B, 0x47FCDA31, 0x47FCE6D6, 0x47FCF37B, 0x47FD0021, 0x47FD0CC7, 0x47FD196D, 0x47FD2613,
 0x47FD32B9, 0x47FD3F60, 0x47FD4C07, 0x47FD58AD, 0x47FD6554, 0x47FD71FB, 0x47FD7EA3, 0x47FD8B4A,
 0x47FD97F2, 0x47FDA49A, 0x47FDB141, 0x47FDBDEA, 0x47FDCA92, 0x47FDD73A, 0x47FDE3E3, 0x47FDF08B,
 0x47FDFD34, 0x47FE09DD, 0x47FE1686, 0x47FE2330, 0x47FE2FD9, 0x47FE3C83, 0x47FE492D, 0x47FE55D7,
 0x47FE6281, 0x47FE6F2B, 0x47FE7BD5, 0x47FE8880, 0x47FE952B, 0x47FEA1D6, 0x47FEAE81, 0x47FEBB2C,
 0x47FEC7D7, 0x47FED483, 0x47FEE12F, 0x47FEEDDA, 0x47FEFA86, 0x47FF0733, 0x47FF13DF, 0x47FF208B,
 0x47FF2D38, 0x47FF39E5, 0x47FF4692, 0x47FF533F, 0x47FF5FEC, 0x47FF6C9A, 0x47FF7947, 0x47FF85F5,
 0x47FF92A3, 0x47FF9F51, 0x47FFABFF, 0x47FFB8AD, 0x47FFC55C, 0x47FFD20B, 0x47FFDEB9, 0x47FFEB68,
 0x47FFF818, 0x48000263, 0x480008BB, 0x48000F13, 0x4800156B, 0x48001BC3, 0x4800221B, 0x48002873,
 0x48002ECB, 0x48003523, 0x48003B7C, 0x480041D4, 0x4800482D, 0x48004E85, 0x480054DE, 0x48005B37,
 0x48006190, 0x480067E9, 0x48006E42, 0x4800749B, 0x48007AF4, 0x4800814D, 0x480087A6, 0x48008E00,
 0x48009459, 0x48009AB3, 0x4800A10C, 0x4800A766, 0x4800ADC0, 0x4800B41A, 0x4800BA74, 0x4800C0CE,
 0x4800C728, 0x4800CD82, 0x4800D3DC, 0x4800DA36, 0x4800E091, 0x4800E6EB, 0x4800ED46, 0x4800F3A0,
 0x4800F9FB, 0x48010056, 0x480106B1, 0x48010D0C, 0x48011367, 0x480119C2, 0x4801201D, 0x48012678,
 0x48012CD4, 0x4801332F, 0x4801398B, 0x48013FE6, 0x48014642, 0x48014C9E, 0x480152F9, 0x48015955,
 0x48015FB1, 0x4801660D, 0x48016C69, 0x480172C6, 0x48017922, 0x48017F7E, 0x480185DB, 0x48018C37,
 0x48019294, 0x480198F0, 0x48019F4D, 0x4801A5AA, 0x4801AC07, 0x4801B264, 0x4801B8C1, 0x4801BF1E,
 0x4801C57B, 0x4801CBD9, 0x4801D236, 0x4801D893, 0x4801DEF1, 0x4801E54E, 0x4801EBAC, 0x4801F20A,
 0x4801F868, 0x4801FEC6, 0x48020524, 0x48020B82, 0x480211E0, 0x4802183E, 0x48021E9C, 0x480224FB,
 0x48022B59, 0x480231B8, 0x48023816, 0x48023E75, 0x480244D4, 0x48024B33, 0x48025192, 0x480257F1,
 0x48025E50, 0x480264AF, 0x48026B0E, 0x4802716D, 0x480277CD, 0x48027E2C, 0x4802848C, 0x48028AEC,
 0x4802914B, 0x480297AB, 0x48029E0B, 0x4802A46B, 0x4802AACB, 0x4802B12B, 0x4802B78B, 0x4802BDEB,
 0x4802C44C, 0x4802CAAC, 0x4802D10D, 0x4802D76D, 0x4802DDCE, 0x4802E42F, 0x4802EA8F, 0x4802F0F0,
 0x4802F751, 0x4802FDB2, 0x48030413, 0x48030A74, 0x480310D6, 0x48031737, 0x48031D98, 0x480323FA,
 0x48032A5C, 0x480330BD, 0x4803371F, 0x48033D81, 0x480343E3, 0x48034A45, 0x480350A7, 0x48035709,
 0x48035D6B, 0x480363CD, 0x48036A30, 0x48037092, 0x480376F4, 0x48037D57, 0x480383BA, 0x48038A1C,
 0x4803907F, 0x480396E2, 0x48039D45, 0x4803A3A8, 0x4803AA0B, 0x4803B06E, 0x4803B6D2, 0x4803BD35,
 0x4803C399, 0x4803C9FC, 0x4803D060, 0x4803D6C3, 0x4803DD27, 0x4803E38B, 0x4803E9EF, 0x4803F053,
 0x4803F6B7, 0x4803FD1B, 0x4804037F, 0x480409E3, 0x48041048, 0x480416AC, 0x48041D11, 0x48042375,
 0x480429DA, 0x4804303F, 0x480436A4, 0x48043D08, 0x4804436D, 0x480449D2, 0x48045038, 0x4804569D,
 0x48045D02, 0x48046367, 0x480469CD, 0x48047032, 0x48047698, 0x48047CFE, 0x48048363, 0x480489C9,
 0x4804902F, 0x48049695, 0x48049CFB, 0x4804A361, 0x4804A9C8, 0x4804B02E, 0x4804B694, 0x4804BCFB,
 0x4804C361, 0x4804C9C8, 0x4804D02F, 0x4804D695, 0x4804DCFC, 0x4804E363, 0x4804E9CA, 0x4804F031,
 0x4804F698, 0x4804FCFF, 0x48050367, 0x480509CE, 0x48051036, 0x4805169D, 0x48051D05, 0x4805236C,
 0x480529D4, 0x4805303C, 0x480536A4, 0x48053D0C, 0x48054374, 0x480549DC, 0x48055044, 0x480556AD,
 0x48055D15, 0x4805637D, 0x480569E6, 0x4805704F, 0x480576B7, 0x48057D20, 0x48058389, 0x480589F2,
 0x4805905B, 0x480596C4, 0x48059D2D, 0x4805A396, 0x4805A9FF, 0x4805B069, 0x4805B6D2, 0x4805BD3C,
 0x4805C3A5, 0x4805CA0F, 0x4805D079, 0x4805D6E3, 0x4805DD4D, 0x4805E3B7, 0x4805EA21, 0x4805F08B,
 0x4805F6F5, 0x4805FD5F, 0x480603CA, 0x48060A34, 0x4806109F, 0x48061709, 0x48061D74, 0x480623DF,
 0x48062A4A, 0x480630B4, 0x4806371F, 0x48063D8A, 0x480643F6, 0x48064A61, 0x480650CC, 0x48065738,
 0x48065DA3, 0x4806640E, 0x48066A7A, 0x480670E6, 0x48067752, 0x48067DBD, 0x48068429, 0x48068A95,
 0x48069101, 0x4806976D, 0x48069DDA, 0x4806A446, 0x4806AAB2, 0x4806B11F, 0x4806B78B, 0x4806BDF8,
 0x4806C465, 0x4806CAD1, 0x4806D13E, 0x4806D7AB, 0x4806DE18, 0x4806E485, 0x4806EAF2, 0x4806F15F,
 0x4806F7CD, 0x4806FE3A, 0x480704A7, 0x48070B15, 0x48071183, 0x480717F0, 0x48071E5E, 0x480724CC,
 0x48072B3A, 0x480731A8, 0x48073816, 0x48073E84, 0x480744F2, 0x48074B60, 0x480751CF, 0x4807583D,
 0x48075EAC, 0x4807651A, 0x48076B89, 0x480771F8, 0x48077866, 0x48077ED5, 0x48078544, 0x48078BB3,
 0x48079222, 0x48079892, 0x48079F01, 0x4807A570, 0x4807ABE0, 0x4807B24F, 0x4807B8BF, 0x4807BF2E,
 0x4807C59E, 0x4807CC0E, 0x4807D27E, 0x4807D8EE, 0x4807DF5E, 0x4807E5CE, 0x4807EC3E, 0x4807F2AE,
 0x4807F91F, 0x4807FF8F, 0x48080600, 0x48080C70, 0x480812E1, 0x48081952, 0x48081FC2, 0x48082633,
 0x48082CA4, 0x48083315, 0x48083986, 0x48083FF8, 0x48084669, 0x48084CDA, 0x4808534C, 0x480859BD,
 0x4808602F, 0x480866A0, 0x48086D12, 0x48087384, 0x480879F6, 0x48088068, 0x480886DA, 0x48088D4C,
 0x480893BE, 0x48089A30, 0x4808A0A2, 0x4808A715, 0x4808AD87, 0x4808B3FA, 0x4808BA6C, 0x4808C0DF,
 0x4808C752, 0x4808CDC5, 0x4808D438, 0x4808DAAB, 0x4808E11E, 0x4808E791, 0x4808EE04, 0x4808F477,
 0x4808FAEB, 0x4809015E, 0x480907D2, 0x48090E45, 0x480914B9, 0x48091B2D, 0x480921A1, 0x48092815,
 0x48092E89, 0x480934FD, 0x48093B71, 0x480941E5, 0x48094859, 0x48094ECE, 0x48095542, 0x48095BB7,
 0x4809622B, 0x480968A0, 0x48096F15, 0x4809758A, 0x48097BFE, 0x48098273, 0x480988E8, 0x48098F5E,
 0x480995D3, 0x48099C48, 0x4809A2BD, 0x4809A933, 0x4809AFA8, 0x4809B61E, 0x4809BC94, 0x4809C309,
 0x4809C97F, 0x4809CFF5, 0x4809D66B, 0x4809DCE1, 0x4809E357, 0x4809E9CD, 0x4809F044, 0x4809F6BA,
 0x4809FD30, 0x480A03A7, 0x480A0A1D, 0x480A1094, 0x480A170B, 0x480A1D82, 0x480A23F8, 0x480A2A6F,
 0x480A30E6, 0x480A375E, 0x480A3DD5, 0x480A444C, 0x480A4AC3, 0x480A513B, 0x480A57B2, 0x480A5E2A,
 0x480A64A1, 0x480A6B19, 0x480A7191, 0x480A7809, 0x480A7E81, 0x480A84F9, 0x480A8B71, 0x480A91E9,
 0x480A9861, 0x480A9ED9, 0x480AA552, 0x480AABCA, 0x480AB243, 0x480AB8BB, 0x480ABF34, 0x480AC5AD,
 0x480ACC26, 0x480AD29F, 0x480AD918, 0x480ADF91, 0x480AE60A, 0x480AEC83, 0x480AF2FC, 0x480AF976,
 0x480AFFEF, 0x480B0669, 0x480B0CE2, 0x480B135C, 0x480B19D6, 0x480B204F, 0x480B26C9, 0x480B2D43,
 0x480B33BD, 0x480B3A37, 0x480B40B2, 0x480B472C, 0x480B4DA6, 0x480B5421, 0x480B5A9B, 0x480B6116,
 0x480B6790, 0x480B6E0B, 0x480B7486, 0x480B7B01, 0x480B817C, 0x480B87F7, 0x480B8E72, 0x480B94ED,
 0x480B9B68, 0x480BA1E3, 0x480BA85F, 0x480BAEDA, 0x480BB556, 0x480BBBD1, 0x480BC24D, 0x480BC8C9,
 0x480BCF45, 0x480BD5C1, 0x480BDC3D, 0x480BE2B9, 0x480BE935, 0x480BEFB1, 0x480BF62D, 0x480BFCAA,
 0x480C0326, 0x480C09A3, 0x480C101F, 0x480C169C, 0x480C1D19, 0x480C2396, 0x480C2A13, 0x480C3090,
 0x480C370D, 0x480C3D8A, 0x480C4407, 0x480C4A84, 0x480C5102, 0x480C577F, 0x480C5DFC, 0x480C647A,
 0x480C6AF8, 0x480C7175, 0x480C77F3, 0x480C7E71, 0x480C84EF, 0x480C8B6D, 0x480C91EB, 0x480C9869,
 0x480C9EE8, 0x480CA566, 0x480CABE4, 0x480CB263, 0x480CB8E1, 0x480CBF60, 0x480CC5DF, 0x480CCC5D,
 0x480CD2DC, 0x480CD95B, 0x480CDFDA, 0x480CE659, 0x480CECD8, 0x480CF358, 0x480CF9D7, 0x480D0056,
 0x480D06D6, 0x480D0D55, 0x480D13D5, 0x480D1A55, 0x480D20D4, 0x480D2754, 0x480D2DD4, 0x480D3454,
 0x480D3AD4, 0x480D4154, 0x480D47D4, 0x480D4E55, 0x480D54D5, 0x480D5B55, 0x480D61D6, 0x480D6856,
 0x480D6ED7, 0x480D7558, 0x480D7BD9, 0x480D8259, 0x480D88DA, 0x480D8F5B, 0x480D95DD, 0x480D9C5E,
 0x480DA2DF, 0x480DA960, 0x480DAFE2, 0x480DB663, 0x480DBCE5, 0x480DC366, 0x480DC9E8, 0x480DD06A,
 0x480DD6EC, 0x480DDD6E, 0x480DE3EF, 0x480DEA72, 0x480DF0F4, 0x480DF776, 0x480DFDF8, 0x480E047B,
 0x480E0AFD, 0x480E1180, 0x480E1802, 0x480E1E85, 0x480E2507, 0x480E2B8A, 0x480E320D, 0x480E3890,
 0x480E3F13, 0x480E4596, 0x480E4C19, 0x480E529D, 0x480E5920, 0x480E5FA3, 0x480E6627, 0x480E6CAA,
 0x480E732E, 0x480E79B2, 0x480E8036, 0x480E86B9, 0x480E8D3D, 0x480E93C1, 0x480E9A45, 0x480EA0CA,
 0x480EA74E, 0x480EADD2, 0x480EB457, 0x480EBADB, 0x480EC15F, 0x480EC7E4, 0x480ECE69, 0x480ED4EE,
 0x480EDB72, 0x480EE1F7, 0x480EE87C, 0x480EEF01, 0x480EF586, 0x480EFC0C, 0x480F0291, 0x480F0916,
 0x480F0F9C, 0x480F1621, 0x480F1CA7, 0x480F232C, 0x480F29B2, 0x480F3038, 0x480F36BE, 0x480F3D44,
 0x480F43CA, 0x480F4A50, 0x480F50D6, 0x480F575C, 0x480F5DE2, 0x480F6469, 0x480F6AEF, 0x480F7176,
 0x480F77FC, 0x480F7E83, 0x480F850A, 0x480F8B91, 0x480F9217, 0x480F989E, 0x480F9F25, 0x480FA5AD,
 0x480FAC34, 0x480FB2BB, 0x480FB942, 0x480FBFCA, 0x480FC651, 0x480FCCD9, 0x480FD361, 0x480FD9E8,
 0x480FE070, 0x480FE6F8, 0x480FED80, 0x480FF408, 0x480FFA90, 0x48100118, 0x481007A0, 0x48100E29,
 0x481014B1, 0x48101B39, 0x481021C2, 0x4810284B, 0x48102ED3, 0x4810355C, 0x48103BE5, 0x4810426E,
 0x481048F7, 0x48104F80, 0x48105609, 0x48105C92, 0x4810631B, 0x481069A5, 0x4810702E, 0x481076B8,
 0x48107D41, 0x481083CB, 0x48108A55, 0x481090DE, 0x48109768, 0x48109DF2, 0x4810A47C, 0x4810AB06,
 0x4810B190, 0x4810B81B, 0x4810BEA5, 0x4810C52F, 0x4810CBBA, 0x4810D244, 0x4810D8CF, 0x4810DF5A,
 0x4810E5E4, 0x4810EC6F, 0x4810F2FA, 0x4810F985, 0x48110010, 0x4811069B, 0x48110D26, 0x481113B2,
 0x48111A3D, 0x481120C8, 0x48112754, 0x48112DDF, 0x4811346B, 0x48113AF7, 0x48114183, 0x4811480E,
 0x48114E9A, 0x48115526, 0x48115BB2, 0x4811623F, 0x481168CB, 0x48116F57, 0x481175E3, 0x48117C70,
 0x481182FC, 0x48118989, 0x48119016, 0x481196A2, 0x48119D2F, 0x4811A3BC, 0x4811AA49, 0x4811B0D6,
 0x4811B763, 0x4811BDF0, 0x4811C47E, 0x4811CB0B, 0x4811D198, 0x4811D826, 0x4811DEB3, 0x4811E541,
 0x4811EBCF, 0x4811F25D, 0x4811F8EA, 0x4811FF78, 0x48120606, 0x48120C94, 0x48121323, 0x481219B1,
 0x4812203F, 0x481226CD, 0x48122D5C, 0x481233EA, 0x48123A79, 0x48124108, 0x48124796, 0x48124E25,
 0x481254B4, 0x48125B43, 0x481261D2, 0x48126861, 0x48126EF0, 0x4812757F, 0x48127C0F, 0x4812829E,
 0x4812892E, 0x48128FBD, 0x4812964D, 0x48129CDC, 0x4812A36C, 0x4812A9FC, 0x4812B08C, 0x4812B71C,
 0x4812BDAC, 0x4812C43C, 0x4812CACC, 0x4812D15D, 0x4812D7ED, 0x4812DE7D, 0x4812E50E, 0x4812EB9E,
 0x4812F22F, 0x4812F8C0, 0x4812FF50, 0x481305E1, 0x48130C72, 0x48131303, 0x48131994, 0x48132025,
 0x481326B7, 0x48132D48, 0x481333D9, 0x48133A6B, 0x481340FC, 0x4813478E, 0x48134E20, 0x481354B1,
 0x48135B43, 0x481361D5, 0x48136867, 0x48136EF9, 0x4813758B, 0x48137C1D, 0x481382B0, 0x48138942,
 0x48138FD4, 0x48139667, 0x48139CF9, 0x4813A38C, 0x4813AA1F, 0x4813B0B1, 0x4813B744, 0x4813BDD7,
 0x4813C46A, 0x4813CAFD, 0x4813D190, 0x4813D823, 0x4813DEB7, 0x4813E54A, 0x4813EBDD, 0x4813F271,
 0x4813F904, 0x4813FF98, 0x4814062C, 0x48140CC0, 0x48141353, 0x481419E7, 0x4814207B, 0x4814270F,
 0x48142DA4, 0x48143438, 0x48143ACC, 0x48144160, 0x481447F5, 0x48144E89, 0x4814551E, 0x48145BB3,
 0x48146247, 0x481468DC, 0x48146F71, 0x48147606, 0x48147C9B, 0x48148330, 0x481489C5, 0x4814905B,
 0x481496F0, 0x48149D85, 0x4814A41B, 0x4814AAB0, 0x4814B146, 0x4814B7DC, 0x4814BE71, 0x4814C507,
 0x4814CB9D, 0x4814D233, 0x4814D8C9, 0x4814DF5F, 0x4814E5F5, 0x4814EC8C, 0x4814F322, 0x4814F9B8,
 0x4815004F, 0x481506E5, 0x48150D7C, 0x48151413, 0x48151AAA, 0x48152140, 0x481527D7, 0x48152E6E,
 0x48153505, 0x48153B9D, 0x48154234, 0x481548CB, 0x48154F62, 0x481555FA, 0x48155C91, 0x48156329,
 0x481569C1, 0x48157058, 0x481576F0, 0x48157D88, 0x48158420, 0x48158AB8, 0x48159150, 0x481597E8,
 0x48159E80, 0x4815A519, 0x4815ABB1, 0x4815B249, 0x4815B8E2, 0x4815BF7B, 0x4815C613, 0x4815CCAC,
 0x4815D345, 0x4815D9DE, 0x4815E077, 0x4815E710, 0x4815EDA9, 0x4815F442, 0x4815FADB, 0x48160174,
 0x4816080E, 0x48160EA7, 0x48161541, 0x48161BDA, 0x48162274, 0x4816290E, 0x48162FA8, 0x48163642,
 0x48163CDC, 0x48164376, 0x48164A10, 0x481650AA, 0x48165744, 0x48165DDF, 0x48166479, 0x48166B13,
 0x481671AE, 0x48167849, 0x48167EE3, 0x4816857E, 0x48168C19, 0x481692B4, 0x4816994F, 0x48169FEA,
 0x4816A685, 0x4816AD20, 0x4816B3BB, 0x4816BA57, 0x4816C0F2, 0x4816C78E, 0x4816CE29, 0x4816D4C5,
 0x4816DB61, 0x4816E1FC, 0x4816E898, 0x4816EF34, 0x4816F5D0, 0x4816FC6C, 0x48170308, 0x481709A5,
 0x48171041, 0x481716DD, 0x48171D7A, 0x48172416, 0x48172AB3, 0x4817314F, 0x481737EC, 0x48173E89,
 0x48174526, 0x48174BC3, 0x48175260, 0x481758FD, 0x48175F9A, 0x48176637, 0x48176CD4, 0x48177372,
 0x48177A0F, 0x481780AD, 0x4817874A, 0x48178DE8, 0x48179486, 0x48179B24, 0x4817A1C1, 0x4817A85F,
 0x4817AEFD, 0x4817B59C, 0x4817BC3A, 0x4817C2D8, 0x4817C976, 0x4817D015, 0x4817D6B3, 0x4817DD52,
 0x4817E3F0, 0x4817EA8F, 0x4817F12E, 0x4817F7CC, 0x4817FE6B, 0x4818050A, 0x48180BA9, 0x48181248,
 0x481818E8, 0x48181F87, 0x48182626, 0x48182CC6, 0x48183365, 0x48183A05, 0x481840A4, 0x48184744,
 0x48184DE4, 0x48185483, 0x48185B23, 0x481861C3, 0x48186863, 0x48186F03, 0x481875A4, 0x48187C44,
 0x481882E4, 0x48188985, 0x48189025, 0x481896C6, 0x48189D66, 0x4818A407, 0x4818AAA8, 0x4818B149,
 0x4818B7E9, 0x4818BE8A, 0x4818C52B, 0x4818CBCD, 0x4818D26E, 0x4818D90F, 0x4818DFB0, 0x4818E652,
 0x4818ECF3, 0x4818F395, 0x4818FA36, 0x481900D8, 0x4819077A, 0x48190E1C, 0x481914BE, 0x48191B60,
 0x48192202, 0x481928A4, 0x48192F46, 0x481935E8, 0x48193C8B, 0x4819432D, 0x481949D0, 0x48195072,
 0x48195715, 0x48195DB7, 0x4819645A, 0x48196AFD, 0x481971A0, 0x48197843, 0x48197EE6, 0x48198589,
 0x48198C2C, 0x481992D0, 0x48199973, 0x4819A016, 0x4819A6BA, 0x4819AD5D, 0x4819B401, 0x4819BAA5,
 0x4819C149, 0x4819C7EC, 0x4819CE90, 0x4819D534, 0x4819DBD8, 0x4819E27D, 0x4819E921, 0x4819EFC5,
 0x4819F669, 0x4819FD0E, 0x481A03B2, 0x481A0A57, 0x481A10FB, 0x481A17A0, 0x481A1E45, 0x481A24EA,
 0x481A2B8F, 0x481A3234, 0x481A38D9, 0x481A3F7E, 0x481A4623, 0x481A4CC8, 0x481A536E, 0x481A5A13,
 0x481A60B9, 0x481A675E, 0x481A6E04, 0x481A74AA, 0x481A7B4F, 0x481A81F5, 0x481A889B, 0x481A8F41,
 0x481A95E7, 0x481A9C8D, 0x481AA334, 0x481AA9DA, 0x481AB080, 0x481AB727, 0x481ABDCD, 0x481AC474,
 0x481ACB1A, 0x481AD1C1, 0x481AD868, 0x481ADF0F, 0x481AE5B6, 0x481AEC5D, 0x481AF304, 0x481AF9AB,
 0x481B0052, 0x481B06F9, 0x481B0DA1, 0x481B1448, 0x481B1AF0, 0x481B2197, 0x481B283F, 0x481B2EE7,
 0x481B358E, 0x481B3C36, 0x481B42DE, 0x481B4986, 0x481B502E, 0x481B56D6, 0x481B5D7E, 0x481B6427,
 0x481B6ACF, 0x481B7178, 0x481B7820, 0x481B7EC9, 0x481B8571, 0x481B8C1A, 0x481B92C3, 0x481B996C,
 0x481BA014, 0x481BA6BD, 0x481BAD67, 0x481BB410, 0x481BBAB9, 0x481BC162, 0x481BC80C, 0x481BCEB5,
 0x481BD55E, 0x481BDC08, 0x481BE2B2, 0x481BE95B, 0x481BF005, 0x481BF6AF, 0x481BFD59, 0x481C0403,
 0x481C0AAD, 0x481C1157, 0x481C1801, 0x481C1EAC, 0x481C2556, 0x481C2C00, 0x481C32AB, 0x481C3955,
 0x481C4000, 0x481C46AB, 0x481C4D55, 0x481C5400, 0x481C5AAB, 0x481C6156, 0x481C6801, 0x481C6EAC,
 0x481C7558, 0x481C7C03, 0x481C82AE, 0x481C895A, 0x481C9005, 0x481C96B1, 0x481C9D5C, 0x481CA408,
 0x481CAAB4, 0x481CB160, 0x481CB80C, 0x481CBEB7, 0x481CC564, 0x481CCC10, 0x481CD2BC, 0x481CD968,
 0x481CE014, 0x481CE6C1, 0x481CED6D, 0x481CF41A, 0x481CFAC7, 0x481D0173, 0x481D0820, 0x481D0ECD,
 0x481D157A, 0x481D1C27, 0x481D22D4, 0x481D2981, 0x481D302E, 0x481D36DB, 0x481D3D89, 0x481D4436,
 0x481D4AE3, 0x481D5191, 0x481D583F, 0x481D5EEC, 0x481D659A, 0x481D6C48, 0x481D72F6, 0x481D79A4,
 0x481D8052, 0x481D8700, 0x481D8DAE, 0x481D945C, 0x481D9B0B, 0x481DA1B9, 0x481DA868, 0x481DAF16,
 0x481DB5C5, 0x481DBC73, 0x481DC322, 0x481DC9D1, 0x481DD080, 0x481DD72F, 0x481DDDDE, 0x481DE48D,
 0x481DEB3C, 0x481DF1EB, 0x481DF89B, 0x481DFF4A, 0x481E05F9, 0x481E0CA9, 0x481E1359, 0x481E1A08,
 0x481E20B8, 0x481E2768, 0x481E2E18, 0x481E34C8, 0x481E3B78, 0x481E4228, 0x481E48D8, 0x481E4F88,
 0x481E5638, 0x481E5CE9, 0x481E6399, 0x481E6A4A, 0x481E70FA, 0x481E77AB, 0x481E7E5C, 0x481E850C,
 0x481E8BBD, 0x481E926E, 0x481E991F, 0x481E9FD0, 0x481EA682, 0x481EAD33, 0x481EB3E4, 0x481EBA95,
 0x481EC147, 0x481EC7F8, 0x481ECEAA, 0x481ED55C, 0x481EDC0D, 0x481EE2BF, 0x481EE971, 0x481EF023,
 0x481EF6D5, 0x481EFD87, 0x481F0439, 0x481F0AEB, 0x481F119D, 0x481F1850, 0x481F1F02, 0x481F25B5,
 0x481F2C67, 0x481F331A, 0x481F39CD, 0x481F407F, 0x481F4732, 0x481F4DE5, 0x481F5498, 0x481F5B4B,
 0x481F61FE, 0x481F68B1, 0x481F6F65, 0x481F7618, 0x481F7CCB, 0x481F837F, 0x481F8A33, 0x481F90E6,
 0x481F979A, 0x481F9E4E, 0x481FA501, 0x481FABB5, 0x481FB269, 0x481FB91D, 0x481FBFD1, 0x481FC686,
 0x481FCD3A, 0x481FD3EE, 0x481FDAA3, 0x481FE157, 0x481FE80C, 0x481FEEC0, 0x481FF575, 0x481FFC2A,
 0x482002DE, 0x48200993, 0x48201048, 0x482016FD, 0x48201DB2, 0x48202467, 0x48202B1D, 0x482031D2,
 0x48203887, 0x48203F3D, 0x482045F2, 0x48204CA8, 0x4820535E, 0x48205A13, 0x482060C9, 0x4820677F,
 0x48206E35, 0x482074EB, 0x48207BA1, 0x48208257, 0x4820890D, 0x48208FC4, 0x4820967A, 0x48209D30,
 0x4820A3E7, 0x4820AA9D, 0x4820B154, 0x4820B80B, 0x4820BEC2, 0x4820C578, 0x4820CC2F, 0x4820D2E6,
 0x4820D99D, 0x4820E054, 0x4820E70C, 0x4820EDC3, 0x4820F47A, 0x4820FB32, 0x482101E9, 0x482108A1,
 0x48210F58, 0x48211610, 0x48211CC8, 0x48212380, 0x48212A38, 0x482130EF, 0x482137A8, 0x48213E60
};

static const unsigned int pow2sf_tab[ 64 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 0x33000000, 0x33800000, 0x34000000, 0x34800000, 0x35000000, 0x35800000, 0x36000000, 0x36800000,
 0x37000000, 0x37800000, 0x38000000, 0x38800000, 0x39000000, 0x39800000, 0x3A000000, 0x3A800000,
 0x3B000000, 0x3B800000, 0x3C000000, 0x3C800000, 0x3D000000, 0x3D800000, 0x3E000000, 0x3E800000,
 0x3F000000, 0x3F800000, 0x40000000, 0x40800000, 0x41000000, 0x41800000, 0x42000000, 0x42800000,
 0x43000000, 0x43800000, 0x44000000, 0x44800000, 0x45000000, 0x45800000, 0x46000000, 0x46800000,
 0x47000000, 0x47800000, 0x48000000, 0x48800000, 0x49000000, 0x49800000, 0x4A000000, 0x4A800000,
 0x4B000000, 0x4B800000, 0x4C000000, 0x4C800000, 0x4D000000, 0x4D800000, 0x4E000000, 0x4E800000,
 0x4F000000, 0x4F800000, 0x50000000, 0x50800000, 0x51000000, 0x51800000, 0x52000000, 0x52800000
};

static const unsigned int exp_table[ 128 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 0x3F000000, 0x3E800000, 0x3E000000, 0x3D800000, 0x3D000000, 0x3C800000, 0x3C000000, 0x3B800000,
 0x3B000000, 0x3A800000, 0x3A000000, 0x39800000, 0x39000000, 0x38800000, 0x38000000, 0x37800000,
 0x37000000, 0x36800000, 0x36000000, 0x35800000, 0x35000000, 0x34800000, 0x34000000, 0x33800000,
 0x33000000, 0x32800000, 0x32000000, 0x31800000, 0x31000000, 0x30800000, 0x30000000, 0x2F800000,
 0x2F000000, 0x2E800000, 0x2E000000, 0x2D800000, 0x2D000000, 0x2C800000, 0x2C000000, 0x2B800000,
 0x2B000000, 0x2A800000, 0x2A000000, 0x29800000, 0x29000000, 0x28800000, 0x28000000, 0x27800000,
 0x27000000, 0x26800000, 0x26000000, 0x25800000, 0x25000000, 0x24800000, 0x24000000, 0x23800000,
 0x23000000, 0x22800000, 0x22000000, 0x21800000, 0x21000000, 0x20800000, 0x20000000, 0x1F800000,
 0x1F000000, 0x1E800000, 0x1E000000, 0x1D800000, 0x1D000000, 0x1C800000, 0x1C000000, 0x1B800000,
 0x1B000000, 0x1A800000, 0x1A000000, 0x19800000, 0x19000000, 0x18800000, 0x18000000, 0x17800000,
 0x17000000, 0x16800000, 0x16000000, 0x15800000, 0x15000000, 0x14800000, 0x14000000, 0x13800000,
 0x13000000, 0x12800000, 0x12000000, 0x11800000, 0x11000000, 0x10800000, 0x10000000, 0x0F800000,
 0x0F000000, 0x0E800000, 0x0E000000, 0x0D800000, 0x0D000000, 0x0C800000, 0x0C000000, 0x0B800000,
 0x0B000000, 0x0A800000, 0x0A000000, 0x09800000, 0x09000000, 0x08800000, 0x08000000, 0x07800000,
 0x07000000, 0x06800000, 0x06000000, 0x05800000, 0x05000000, 0x04800000, 0x04000000, 0x03800000,
 0x03000000, 0x02800000, 0x02000000, 0x01800000, 0x01000000, 0x00800000, 0x00000000, 0x00000000
};

static const unsigned int mnt_table[ 128 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 0x3F740000, 0x3F720000, 0x3F700000, 0x3F6E0000, 0x3F6D0000, 0x3F6B0000, 0x3F690000, 0x3F670000,
 0x3F660000, 0x3F640000, 0x3F620000, 0x3F610000, 0x3F5F0000, 0x3F5E0000, 0x3F5C0000, 0x3F5A0000,
 0x3F590000, 0x3F570000, 0x3F560000, 0x3F540000, 0x3F530000, 0x3F520000, 0x3F500000, 0x3F4F0000,
 0x3F4D0000, 0x3F4C0000, 0x3F4B0000, 0x3F490000, 0x3F480000, 0x3F470000, 0x3F460000, 0x3F440000,
 0x3F430000, 0x3F420000, 0x3F410000, 0x3F400000, 0x3F3E0000, 0x3F3D0000, 0x3F3C0000, 0x3F3B0000,
 0x3F3A0000, 0x3F390000, 0x3F380000, 0x3F370000, 0x3F360000, 0x3F350000, 0x3F330000, 0x3F320000,
 0x3F310000, 0x3F300000, 0x3F2F0000, 0x3F2E0000, 0x3F2E0000, 0x3F2D0000, 0x3F2C0000, 0x3F2B0000,
 0x3F2A0000, 0x3F290000, 0x3F280000, 0x3F270000, 0x3F260000, 0x3F250000, 0x3F240000, 0x3F240000,
 0x3F230000, 0x3F220000, 0x3F210000, 0x3F200000, 0x3F1F0000, 0x3F1F0000, 0x3F1E0000, 0x3F1D0000,
 0x3F1C0000, 0x3F1B0000, 0x3F1B0000, 0x3F1A0000, 0x3F190000, 0x3F180000, 0x3F180000, 0x3F170000,
 0x3F160000, 0x3F150000, 0x3F150000, 0x3F140000, 0x3F130000, 0x3F130000, 0x3F120000, 0x3F110000,
 0x3F110000, 0x3F100000, 0x3F0F0000, 0x3F0F0000, 0x3F0E0000, 0x3F0D0000, 0x3F0D0000, 0x3F0C0000,
 0x3F0B0000, 0x3F0B0000, 0x3F0A0000, 0x3F0A0000, 0x3F090000, 0x3F080000, 0x3F080000, 0x3F070000,
 0x3F070000, 0x3F060000, 0x3F050000, 0x3F050000, 0x3F040000, 0x3F040000, 0x3F030000, 0x3F030000,
 0x3F020000, 0x3F020000, 0x3F010000, 0x3F010000, 0x3F000000, 0x3EFF0000, 0x3EFE0000, 0x3EFD0000,
 0x3EFC0000, 0x3EFB0000, 0x3EFA0000, 0x3EF90000, 0x3EF80000, 0x3EF70000, 0x3EF60000, 0x3EF50000
};

static unsigned char const mes[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0x67, 0x20, 0x61, 0x20, 0x20, 0x20, 0x6F, 0x20,
 0x72, 0x20, 0x65, 0x20, 0x6E, 0x20, 0x20, 0x20,
 0x74, 0x20, 0x68, 0x20, 0x67, 0x20, 0x69, 0x20,
 0x72, 0x20, 0x79, 0x20, 0x70, 0x20, 0x6F, 0x20,
 0x63
};

static const uint8_t ObjectTypesTable[ 32 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, /*  0 NULL                               */
 1, /*  1 AAC Main                           */
 1, /*  2 AAC LC                             */
 0, /*  3 AAC SSR                            */
 0, /*  4 AAC LTP                            */
 1, /*  5 SBR                                */
 0, /*  6 AAC Scalable                       */
 0, /*  7 TwinVQ                             */
 0, /*  8 CELP                               */
 0, /*  9 HVXC                               */
 0, /* 10 Reserved                           */
 0, /* 11 Reserved                           */
 0, /* 12 TTSI                               */
 0, /* 13 Main synthetic                     */
 0, /* 14 Wavetable synthesis                */
 0, /* 15 General MIDI                       */
 0, /* 16 Algorithmic Synthesis and Audio FX */
 0, /* 17 ER AAC LC                          */
 0, /* 18 (Reserved)                         */
 0, /* 19 ER AAC LTP                         */
 0, /* 20 ER AAC scalable                    */
 0, /* 21 ER TwinVQ                          */
 0, /* 22 ER BSAC                            */
 0, /* 23 ER AAC LD                          */
 0, /* 24 ER CELP                            */
 0, /* 25 ER HVXC                            */
 0, /* 26 ER HILN                            */
 0, /* 27 ER Parametric                      */
 0, /* 28 (Reserved)                         */
 0, /* 29 (Reserved)                         */
 0, /* 30 (Reserved)                         */
 0  /* 31 (Reserved)                         */
};

static const uint8_t num_swb_1024_window[] __attribute__(   (  section( ".rodata" )  )   ) = {
 41, 41, 47, 49, 49, 51, 47, 47, 43, 43, 43, 40
};

static const uint16_t swb_offset_1024_96[] __attribute__(   (  section( ".rodata" )  )   ) = {
   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,   40,  44,  48,  52,  56,
  64,  72,  80,  88,  96, 108, 120, 132, 144, 156,  172, 188, 212, 240, 276,
 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024
};

static const uint16_t swb_offset_1024_64[] __attribute__(   (  section( ".rodata" )  )   ) = {
   0,   4,    8,  12,   16,  20,  24,  28,  32,  36,  40,  44,  48,  52, 56,
  64,  72,   80,  88,  100, 112, 124, 140, 156, 172, 192, 216, 240, 268, 304,
 344, 384,  424,  464, 504, 544, 584, 624, 664, 704, 744, 784, 824, 864, 904,
 944, 984, 1024
};

static const uint16_t swb_offset_1024_48[] __attribute__(   (  section( ".rodata" )  )   ) = {
   0,   4,   8,  12,   16,  20,   24,  28,  32,  36,  40,  48 , 56,  64,  72,
  80,  88,  96, 108,  120, 132,  144, 160, 176, 196, 216, 240, 264, 292, 320,
 352, 384, 416, 448,  480,  512, 544, 576, 608, 640, 672, 704, 736, 768, 800,
 832, 864, 896, 928, 1024
};

static const uint16_t swb_offset_1024_32[] __attribute__(   (  section( ".rodata" )  )   ) = {
   0,   4,   8,  12,  16,  20,   24,  28,  32,  36,  40,  48,  56 , 64, 72,
  80,  88,  96, 108, 120, 132,  144, 160, 176, 196, 216, 240, 264, 292, 320,
 352, 384, 416, 448, 480, 512,  544, 576, 608, 640, 672, 704, 736, 768, 800,
 832, 864, 896, 928, 960, 992, 1024
};

static const uint16_t swb_offset_1024_24[] __attribute__(   (  section( ".rodata" )  )   ) = {
   0,   4,    8,  12,   16,  20,  24,  28,  32,  36,  40,  44,  52,  60,  68,
  76,  84,   92, 100,  108, 116, 124, 136, 148, 160, 172, 188, 204, 220, 240,
 260, 284,  308,  336, 364, 396, 432, 468, 508, 552, 600, 652, 704, 768, 832,
 896, 960, 1024
};

static const uint16_t swb_offset_1024_16[] __attribute__(   (  section( ".rodata" )  )   ) = {
   0,   8,  16,  24,  32,  40,  48,  56,  64,  72,  80,  88, 100,  112, 124,
 136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320,  344, 368,
 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024
};

static const uint16_t swb_offset_1024_8[] __attribute__(   (  section( ".rodata" )  )   ) = {
   0,  12,  24,  36,  48,  60,  72,  84,  96, 108,  120, 132, 144, 156, 172,
 188, 204, 220, 236, 252, 268, 288, 308, 328, 348,  372, 396, 420, 448, 476,
 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024
};

static const uint16_t* swb_offset_1024_window[] __attribute__(   (  section( ".data" )  )   ) = {
 swb_offset_1024_96, swb_offset_1024_96,
 swb_offset_1024_64, swb_offset_1024_48,
 swb_offset_1024_48, swb_offset_1024_32,
 swb_offset_1024_24, swb_offset_1024_24,
 swb_offset_1024_16, swb_offset_1024_16,
 swb_offset_1024_16, swb_offset_1024_8
};

static const uint8_t num_swb_128_window[] __attribute__(   (  section( ".rodata" )  )   ) = {
 12, 12, 12, 14, 14, 14, 15, 15, 15, 15, 15, 15
};

static const uint16_t swb_offset_128_96[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};

static const uint16_t swb_offset_128_64[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128
};

static const uint16_t swb_offset_128_48[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128
};

static const uint16_t swb_offset_128_24[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128
};

static const uint16_t swb_offset_128_16[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128
};

static const uint16_t swb_offset_128_8[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128
};

static const uint16_t *swb_offset_128_window[] __attribute__(   (  section( ".data" )  )   ) = {
 swb_offset_128_96, swb_offset_128_96,
 swb_offset_128_64, swb_offset_128_48,
 swb_offset_128_48, swb_offset_128_48,
 swb_offset_128_24, swb_offset_128_24,
 swb_offset_128_16, swb_offset_128_16,
 swb_offset_128_16, swb_offset_128_8
};

static const uint8_t hcb_sf[][2] __attribute__(   (  section( ".rodata" )  )   ) = {
 {   1,  2 }, {  60,  0 }, {   1,  2 }, {   2,  3 }, {   3,  4 }, {  59,  0 }, {   3,  4 }, {   4,  5 },
 {   5,  6 }, {  61,  0 }, {  58,  0 }, {  62,  0 }, {   3,  4 }, {   4,  5 }, {   5,  6 }, {  57,  0 },
 {  63,  0 }, {   4,  5 }, {   5,  6 }, {   6,  7 }, {   7,  8 }, {  56,  0 }, {  64,  0 }, {  55,  0 },
 {  65,  0 }, {   4,  5 }, {   5,  6 }, {   6,  7 }, {   7,  8 }, {  66,  0 }, {  54,  0 }, {  67,  0 },
 {   5,  6 }, {   6,  7 }, {   7,  8 }, {   8,  9 }, {   9, 10 }, {  53,  0 }, {  68,  0 }, {  52,  0 },
 {  69,  0 }, {  51,  0 }, {   5,  6 }, {   6,  7 }, {   7,  8 }, {   8,  9 }, {   9, 10 }, {  70,  0 },
 {  50,  0 }, {  49,  0 }, {  71,  0 }, {   6,  7 }, {   7,  8 }, {   8,  9 }, {   9, 10 }, {  10, 11 },
 {  11, 12 }, {  72,  0 }, {  48,  0 }, {  73,  0 }, {  47,  0 }, {  74,  0 }, {  46,  0 }, {   6,  7 },
 {   7,  8 }, {   8,  9 }, {   9, 10 }, {  10, 11 }, {  11, 12 }, {  76,  0 }, {  75,  0 }, {  77,  0 },
 {  78,  0 }, {  45,  0 }, {  43,  0 }, {   6,  7 }, {   7,  8 }, {   8,  9 }, {   9, 10 }, {  10, 11 },
 {  11, 12 }, {  44,  0 }, {  79,  0 }, {  42,  0 }, {  41,  0 }, {  80,  0 }, {  40,  0 }, {   6,  7 },
 {   7,  8 }, {   8,  9 }, {   9, 10 }, {  10, 11 }, {  11, 12 }, {  81,  0 }, {  39,  0 }, {  82,  0 },
 {   38, 0 }, {  83,  0 }, {   7,  8 }, {   8,  9 }, {   9, 10 }, {  10, 11 }, {  11, 12 }, {  12, 13 },
 {  13, 14 }, {  37,  0 }, {  35,  0 }, {  85,  0 }, {  33,  0 }, {  36,  0 }, {  34,  0 }, {  84,  0 },
 {  32,  0 }, {   6,  7 }, {   7,  8 }, {   8,  9 }, {   9, 10 }, {  10, 11 }, {  11, 12 }, {  87,  0 },
 {  89,  0 }, {  30,  0 }, {  31,  0 }, {   8,  9 }, {   9, 10 }, {  10, 11 }, {  11, 12 }, {  12, 13 },
 {  13, 14 }, {  14, 15 }, {  15, 16 }, {  86,  0 }, {  29,  0 }, {  26,  0 }, {  27,  0 }, {  28,  0 },
 {  24,  0 }, {  88,  0 }, {   9, 10 }, {  10, 11 }, {  11, 12 }, {  12, 13 }, {  13, 14 }, {  14, 15 },
 {  15, 16 }, {  16, 17 }, {  17, 18 }, {  25,  0 }, {  22,  0 }, {  23,  0 }, {  15, 16 }, {  16, 17 },
 {  17, 18 }, {  18, 19 }, {  19, 20 }, {  20, 21 }, {  21, 22 }, {  22, 23 }, {  23, 24 }, {  24, 25 },
 {  25, 26 }, {  26, 27 }, {  27, 28 }, {  28, 29 }, {  29, 30 }, {  90,  0 }, {  21,  0 }, {  19,  0 },
 {   3,  0 }, {   1 , 0 }, {   2,  0 }, {   0,  0 }, {  23, 24 }, {  24, 25 }, {  25, 26 }, {  26, 27 },
 {  27, 28 }, {  28, 29 }, {  29, 30 }, {  30, 31 }, {  31, 32 }, {  32, 33 }, {  33, 34 }, {  34, 35 },
 {  35, 36 }, {  36, 37 }, {  37, 38 }, {  38, 39 }, {  39, 40 }, {  40, 41 }, {  41, 42 }, {  42, 43 },
 {  43, 44 }, {  44, 45 }, {  45, 46 }, {  98,  0 }, {  99,  0 }, { 100,  0 }, { 101,  0 }, { 102,  0 },
 { 117,  0 }, {  97,  0 }, {  91,  0 }, {  92,  0 }, {  93,  0 }, {  94,  0 }, {  95,  0 }, {  96,  0 },
 { 104 , 0 }, { 111,  0 }, { 112,  0 }, { 113,  0 }, { 114,  0 }, { 115,  0 }, { 116,  0 }, { 110,  0 },
 { 105,  0 }, { 106,  0 }, { 107,  0 }, { 108,  0 }, { 109,  0 }, { 118,  0 }, {   6,  0 }, {   8,  0 },
 {   9,  0 }, {  10,  0 }, {   5,  0 }, { 103,  0 }, { 120,  0 }, { 119,  0 }, {   4,  0 }, {   7,  0 },
 {  15,  0 }, {  16,  0 }, {  18,  0 }, {  20,  0 }, {  17,  0 }, {  11,  0 }, {  12,  0 }, {  14,  0 },
 {  13,  0 }
};

static const uint8_t hcbN[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 5, 5, 0, 5, 0, 5, 0, 5, 0, 6, 5
};

static const hcb hcb1_1[] __attribute__(   (  section( ".rodata" )  )   ) = {
 { 0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 },
 { 0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 },
 { 1, 0 }, {  2, 0 }, {  3, 0 }, {  4, 0 }, {  5, 0 }, {  6, 0 }, {  7, 0 }, {  8, 0 },
 { 9, 2 }, { 13, 2 }, { 17, 2 }, { 21, 2 }, { 25, 2 }, { 29, 2 }, { 33, 4 }, { 49, 6 }
};

static const hcb hcb2_1[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 }, {  1, 0 }, {  1, 0 }, {  2, 0 }, {  3, 0 },
 {  4, 0 }, {  5, 0 }, {  6, 0 }, {  7, 0 }, {  8, 0 }, {  9, 1 }, { 11, 1 }, { 13, 1 },
 { 15, 1 }, { 17, 1 }, { 19, 1 }, { 21, 1 }, { 23, 1 }, { 25, 1 }, { 27, 1 }, { 29, 1 },
 { 31, 1 }, { 33, 2 }, { 37, 2 }, { 41, 2 }, { 45, 3 }, { 53, 3 }, { 61, 3 }, { 69, 4 }
};

static const hcb hcb4_1[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  0, 0 }, {  0, 0 }, {  1, 0 }, {  1, 0 }, {  2, 0 }, {  2, 0 }, {  3, 0 }, {  3, 0 },
 {  4, 0 }, {  4, 0 }, {  5, 0 }, {  5, 0 }, {  6, 0 }, {  6, 0 }, {  7, 0 }, {  7, 0 },
 {  8, 0 }, {  8, 0 }, {  9, 0 }, {  9, 0 }, { 10, 0 }, { 11, 0 }, { 12, 0 }, { 13, 0 },
 { 14, 0 }, { 15, 0 }, { 16, 2 }, { 20, 2 }, { 24, 3 }, { 32, 3 }, { 40, 4 }, { 56, 7 }
};

static const hcb hcb6_1[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  0, 0 }, {  0, 0 }, {  1, 0 }, {  1, 0 }, {  2, 0 }, {  2, 0 }, {  3, 0 }, {  3, 0 },
 {  4, 0 }, {  4, 0 }, {  5, 0 }, {  5, 0 }, {  6, 0 }, {  6, 0 }, {  7, 0 }, {  7, 0 },
 {  8, 0 }, {  8, 0 }, {  9, 1 }, { 11, 1 }, { 13, 1 }, { 15, 1 }, { 17, 1 }, { 19, 1 },
 { 21, 1 }, { 23, 1 }, { 25, 2 }, { 29, 2 }, { 33, 2 }, { 37, 3 }, { 45, 4 }, { 61, 6 }
};

static const hcb hcb8_1[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  0, 0 }, {  0, 0 }, {  0, 0 }, {  0, 0 }, {  1, 0 }, {  1, 0 }, {  2, 0 }, {  2, 0 },
 {  3, 0 }, {  3, 0 }, {  4, 0 }, {  4, 0 }, {  5, 0 }, {  5, 0 }, {  6, 0 }, {  7, 0 },
 {  8, 0 }, {  9, 0 }, { 10, 0 }, { 11, 0 }, { 12, 0 }, { 13, 1 }, { 15, 1 }, { 17, 1 },
 { 19, 1 }, { 21, 1 }, { 23, 2 }, { 27, 2 }, { 31, 2 }, { 35, 3 }, { 43, 3 }, { 51, 5 }
};

static const hcb hcb10_1[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {   0, 0 }, {   0, 0 }, {   0, 0 }, {   0, 0 }, {   1, 0 }, {   1, 0 }, {   1, 0 }, {   1, 0 },
 {   2, 0 }, {   2, 0 }, {   2, 0 }, {   2, 0 }, {   3, 0 }, {   3, 0 }, {   4, 0 }, {   4, 0 },
 {   5, 0 }, {   5, 0 }, {   6, 0 }, {   6, 0 }, {   7, 0 }, {   7, 0 }, {   8, 0 }, {   8, 0 },
 {   9, 0 }, {   9, 0 }, {  10, 0 }, {  10, 0 }, {  11, 0 }, {  12, 0 }, {  13, 0 }, {  14, 0 },
 {  15, 0 }, {  16, 0 }, {  17, 0 }, {  18, 0 }, {  19, 0 }, {  20, 0 }, {  21, 0 }, {  22, 0 },
 {  23, 0 }, {  24, 0 }, {  25, 1 }, {  27, 1 }, {  29, 1 }, {  31, 1 }, {  33, 1 }, {  35, 1 },
 {  37, 1 }, {  39, 1 }, {  41, 2 }, {  45, 2 }, {  49, 2 }, {  53, 2 }, {  57, 2 }, {  61, 2 },
 {  65, 3 }, {  73, 3 }, {  81, 3 }, {  89, 3 }, {  97, 4 }, { 113, 4 }, { 129, 4 }, { 145, 6 }
};

static const hcb hcb11_1[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {   0, 0 }, {   0, 0 }, {   1, 0 }, {   1, 0 }, {   2, 0 }, {   3, 0 }, {   4, 0 }, {   5, 0 },
 {   6, 0 }, {   7, 0 }, {   8, 1 }, {  10, 1 }, {  12, 1 }, {  14, 2 }, {  18, 2 }, {  22, 2 },
 {  26, 2 }, {  30, 3 }, {  38, 3 }, {  46, 3 }, {  54, 3 }, {  62, 3 }, {  70, 3 }, {  78, 3 },
 {  86, 4 }, { 102, 4 }, { 118, 4 }, { 134, 4 }, { 150, 5 }, { 182, 5 }, { 214, 5 }, { 246, 7 }
};

static const hcb* hcb_table[] __attribute__(   (  section( ".data" )  )   ) = {
 0, hcb1_1, hcb2_1, 0, hcb4_1, 0, hcb6_1, 0, hcb8_1, 0, hcb10_1, hcb11_1
};

static const hcb_2_quad hcb1_2[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  1,  0,  0,  0,  0 }, {  5,  1,  0,  0,  0 }, {  5, -1,  0,  0,  0 }, {  5,  0,  0,  0, -1 },
 {  5,  0,  1,  0,  0 }, {  5,  0,  0,  0,  1 }, {  5,  0,  0, -1,  0 }, {  5,  0,  0,  1,  0 },
 {  5,  0, -1,  0,  0 }, {  7,  1, -1,  0,  0 }, {  7, -1,  1,  0,  0 }, {  7,  0,  0, -1,  1 },
 {  7,  0,  1, -1,  0 }, {  7,  0, -1,  1,  0 }, {  7,  0,  0,  1, -1 }, {  7,  1,  1,  0,  0 },
 {  7,  0,  0, -1, -1 }, {  7, -1, -1,  0,  0 }, {  7,  0, -1, -1,  0 }, {  7,  1,  0, -1,  0 },
 {  7,  0,  1,  0, -1 }, {  7, -1,  0,  1,  0 }, {  7,  0,  0,  1,  1 }, {  7,  1,  0,  1,  0 },
 {  7,  0, -1,  0,  1 }, {  7,  0,  1,  1,  0 }, {  7,  0,  1,  0,  1 }, {  7, -1,  0, -1,  0 },
 {  7,  1,  0,  0,  1 }, {  7, -1,  0,  0, -1 }, {  7,  1,  0,  0, -1 }, {  7, -1,  0,  0,  1 },
 {  7,  0, -1,  0, -1 }, {  9,  1,  1, -1,  0 }, {  9, -1,  1, -1,  0 }, {  9,  1, -1,  1,  0 },
 {  9,  0,  1,  1, -1 }, {  9,  0,  1, -1,  1 }, {  9,  0, -1,  1,  1 }, {  9,  0, -1,  1, -1 },
 {  9,  1, -1, -1,  0 }, {  9,  1,  0, -1,  1 }, {  9,  0,  1, -1, -1 }, {  9, -1,  1,  1,  0 },
 {  9, -1,  0,  1, -1 }, {  9, -1, -1,  1,  0 }, {  9,  0, -1, -1,  1 }, {  9,  1, -1,  0,  1 },
 {  9,  1, -1,  0, -1 }, {  9, -1,  1,  0, -1 }, {  9, -1,  1,  0, -1 }, {  9, -1,  1,  0, -1 },
 {  9, -1,  1,  0, -1 }, {  9, -1, -1, -1,  0 }, {  9, -1, -1, -1,  0 }, {  9, -1, -1, -1,  0 },
 {  9, -1, -1, -1,  0 }, {  9,  0, -1, -1, -1 }, {  9,  0, -1, -1, -1 }, {  9,  0, -1, -1, -1 },
 {  9,  0, -1, -1, -1 }, {  9,  0,  1,  1,  1 }, {  9,  0,  1,  1,  1 }, {  9,  0,  1,  1,  1 },
 {  9,  0,  1,  1,  1 }, {  9,  1,  0,  1, -1 }, {  9,  1,  0,  1, -1 }, {  9,  1,  0,  1, -1 },
 {  9,  1,  0,  1, -1 }, {  9,  1,  1,  0,  1 }, {  9,  1,  1,  0,  1 }, {  9,  1,  1,  0,  1 },
 {  9,  1,  1,  0,  1 }, {  9, -1,  1,  0,  1 }, {  9, -1,  1,  0,  1 }, {  9, -1,  1,  0,  1 },
 {  9, -1,  1,  0,  1 }, {  9,  1,  1,  1,  0 }, {  9,  1,  1,  1,  0 }, {  9,  1,  1,  1,  0 },
 {  9,  1,  1,  1,  0 }, { 10, -1, -1,  0,  1 }, { 10, -1, -1,  0,  1 }, { 10, -1,  0, -1, -1 },
 { 10, -1,  0, -1, -1 }, { 10,  1,  1,  0, -1 }, { 10,  1,  1,  0, -1 }, { 10,  1,  0, -1, -1 },
 { 10,  1,  0, -1, -1 }, { 10, -1,  0, -1,  1 }, { 10, -1,  0, -1,  1 }, { 10, -1, -1,  0, -1 },
 { 10, -1, -1,  0, -1 }, { 10, -1,  0,  1,  1 }, { 10, -1,  0,  1,  1 }, { 10,  1,  0,  1,  1 },
 { 10,  1,  0,  1,  1 }, { 11,  1, -1,  1, -1 }, { 11, -1,  1, -1,  1 }, { 11, -1,  1,  1, -1 },
 { 11,  1, -1, -1,  1 }, { 11,  1,  1,  1,  1 }, { 11, -1, -1,  1,  1 }, { 11,  1,  1, -1, -1 },
 { 11, -1, -1,  1, -1 }, { 11, -1, -1, -1, -1 }, { 11,  1,  1, -1,  1 }, { 11,  1, -1,  1,  1 },
 { 11, -1,  1,  1,  1 }, { 11, -1,  1, -1, -1 }, { 11, -1, -1, -1,  1 }, { 11,  1, -1, -1, -1 },
 { 11,  1,  1,  1, -1 }
};

static const hcb_2_quad hcb2_2[] __attribute__(   (  section( ".rodata" )  )   ) = {
 { 3,  0,  0,  0,  0 }, { 4,  1,  0,  0,  0 }, { 5, -1,  0,  0,  0 }, { 5,  0,  0,  0,  1 },
 { 5,  0,  0, -1,  0 }, { 5,  0,  0,  0, -1 }, { 5,  0, -1,  0,  0 }, { 5,  0,  0,  1,  0 },
 { 5,  0,  1,  0,  0 }, { 6,  0, -1,  1,  0 }, { 6, -1,  1,  0,  0 }, { 6,  0,  1, -1,  0 },
 { 6,  0,  0,  1, -1 }, { 6,  0,  1,  0, -1 }, { 6,  0,  0, -1,  1 }, { 6, -1,  0,  0, -1 },
 { 6,  1, -1,  0,  0 }, { 6,  1,  0, -1,  0 }, { 6, -1, -1,  0,  0 }, { 6,  0,  0, -1, -1 },
 { 6,  1,  0,  1,  0 }, { 6,  1,  0,  0,  1 }, { 6,  0, -1,  0,  1 }, { 6, -1,  0,  1,  0 },
 { 6,  0,  1,  0,  1 }, { 6,  0, -1, -1,  0 }, { 6, -1,  0,  0,  1 }, { 6,  0, -1,  0, -1 },
 { 6, -1,  0, -1,  0 }, { 6,  1,  1,  0,  0 }, { 6,  0,  1,  1,  0 }, { 6,  0,  0,  1,  1 },
 { 6,  1,  0,  0, -1 }, { 7,  0,  1, -1,  1 }, { 7,  1,  0, -1,  1 }, { 7, -1,  1, -1,  0 },
 { 7,  0, -1,  1, -1 }, { 7,  1, -1,  1,  0 }, { 7,  1,  1,  0, -1 }, { 7,  1,  0,  1,  1 },
 { 7, -1,  1,  1,  0 }, { 7,  0, -1, -1,  1 }, { 7,  1,  1,  1,  0 }, { 7, -1,  0,  1, -1 },
 { 7, -1, -1, -1,  0 }, { 7, -1,  0, -1,  1 }, { 7, -1,  0, -1,  1 }, { 7,  1, -1, -1,  0 },
 { 7,  1, -1, -1,  0 }, { 7,  1,  1, -1,  0 }, { 7,  1,  1, -1,  0 }, { 8,  1, -1,  0,  1 },
 { 8, -1,  1,  0, -1 }, { 8, -1, -1,  1,  0 }, { 8, -1,  0,  1,  1 }, { 8, -1, -1,  0,  1 },
 { 8, -1, -1,  0, -1 }, { 8,  0, -1, -1, -1 }, { 8,  1,  0,  1, -1 }, { 8,  1,  0, -1, -1 },
 { 8,  0,  1, -1, -1 }, { 8,  0,  1,  1,  1 }, { 8, -1,  1,  0,  1 }, { 8, -1,  0, -1, -1 },
 { 8,  0,  1,  1, -1 }, { 8,  1, -1,  0, -1 }, { 8,  0, -1,  1,  1 }, { 8,  1,  1,  0,  1 },
 { 8,  1, -1,  1, -1 }, { 8, -1,  1, -1,  1 }, { 8, -1,  1, -1,  1 }, { 9,  1, -1, -1,  1 },
 { 9, -1, -1, -1, -1 }, { 9, -1,  1,  1, -1 }, { 9, -1,  1,  1,  1 }, { 9,  1,  1,  1,  1 },
 { 9, -1, -1,  1, -1 }, { 9,  1, -1,  1,  1 }, { 9, -1,  1, -1, -1 }, { 9, -1, -1,  1,  1 },
 { 9,  1,  1, -1, -1 }, { 9,  1, -1, -1, -1 }, { 9, -1, -1, -1,  1 }, { 9,  1,  1, -1,  1 },
 { 9,  1,  1,  1, -1 }
};

static const hcb_2_quad hcb4_2[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  4,  1,  1,  1,  1 }, {  4,  0,  1,  1,  1 }, {  4,  1,  1,  0,  1 }, {  4,  1,  1,  1,  0 },
 {  4,  1,  0,  1,  1 }, {  4,  1,  0,  0,  0 }, {  4,  1,  1,  0,  0 }, {  4,  0,  0,  0,  0 },
 {  4,  0,  0,  1,  1 }, {  4,  1,  0,  1,  0 }, {  5,  1,  0,  0,  1 }, {  5,  0,  1,  1,  0 },
 {  5,  0,  0,  0,  1 }, {  5,  0,  1,  0,  1 }, {  5,  0,  0,  1,  0 }, {  5,  0,  1,  0,  0 },
 {  7,  2,  1,  1,  1 }, {  7,  1,  1,  2,  1 }, {  7,  1,  2,  1,  1 }, {  7,  1,  1,  1,  2 },
 {  7,  2,  1,  1,  0 }, {  7,  2,  1,  0,  1 }, {  7,  1,  2,  1,  0 }, {  7,  2,  0,  1,  1 },
 {  7,  0,  1,  2,  1 }, {  7,  0,  1,  2,  1 }, {  8,  0,  1,  1,  2 }, {  8,  1,  1,  2,  0 },
 {  8,  0,  2,  1,  1 }, {  8,  1,  0,  1,  2 }, {  8,  1,  2,  0,  1 }, {  8,  1,  1,  0,  2 },
 {  8,  1,  0,  2,  1 }, {  8,  2,  1,  0,  0 }, {  8,  2,  0,  1,  0 }, {  8,  1,  2,  0,  0 },
 {  8,  2,  0,  0,  1 }, {  8,  0,  1,  0,  2 }, {  8,  0,  2,  1,  0 }, {  8,  0,  0,  1,  2 },
 {  8,  0,  1,  2,  0 }, {  8,  0,  1,  2,  0 }, {  8,  0,  2,  0,  1 }, {  8,  0,  2,  0,  1 },
 {  8,  1,  0,  0,  2 }, {  8,  1,  0,  0,  2 }, {  8,  0,  0,  2,  1 }, {  8,  0,  0,  2,  1 },
 {  8,  1,  0,  2,  0 }, {  8,  1,  0,  2,  0 }, {  8,  2,  0,  0,  0 }, {  8,  2,  0,  0,  0 },
 {  8,  0,  0,  0,  2 }, {  8,  0,  0,  0,  2 }, {  9,  0,  2,  0,  0 }, {  9,  0,  0,  2,  0 },
 {  9,  1,  2,  2,  1 }, {  9,  1,  2,  2,  1 }, {  9,  1,  2,  2,  1 }, {  9,  1,  2,  2,  1 },
 {  9,  1,  2,  2,  1 }, {  9,  1,  2,  2,  1 }, {  9,  1,  2,  2,  1 }, {  9,  1,  2,  2,  1 },
 {  9,  2,  2,  1,  1 }, {  9,  2,  2,  1,  1 }, {  9,  2,  2,  1,  1 }, {  9,  2,  2,  1,  1 },
 {  9,  2,  2,  1,  1 }, {  9,  2,  2,  1,  1 }, {  9,  2,  2,  1,  1 }, {  9,  2,  2,  1,  1 },
 {  9,  2,  1,  2,  1 }, {  9,  2,  1,  2,  1 }, {  9,  2,  1,  2,  1 }, {  9,  2,  1,  2,  1 },
 {  9,  2,  1,  2,  1 }, {  9,  2,  1,  2,  1 }, {  9,  2,  1,  2,  1 }, {  9,  2,  1,  2,  1 },
 {  9,  1,  1,  2,  2 }, {  9,  1,  1,  2,  2 }, {  9,  1,  1,  2,  2 }, {  9,  1,  1,  2,  2 },
 {  9,  1,  1,  2,  2 }, {  9,  1,  1,  2,  2 }, {  9,  1,  1,  2,  2 }, {  9,  1,  1,  2,  2 },
 {  9,  1,  2,  1,  2 }, {  9,  1,  2,  1,  2 }, {  9,  1,  2,  1,  2 }, {  9,  1,  2,  1,  2 },
 {  9,  1,  2,  1,  2 }, {  9,  1,  2,  1,  2 }, {  9,  1,  2,  1,  2 }, {  9,  1,  2,  1,  2 },
 {  9,  2,  1,  1,  2 }, {  9,  2,  1,  1,  2 }, {  9,  2,  1,  1,  2 }, {  9,  2,  1,  1,  2 },
 {  9,  2,  1,  1,  2 }, {  9,  2,  1,  1,  2 }, {  9,  2,  1,  1,  2 }, {  9,  2,  1,  1,  2 },
 { 10,  1,  2,  2,  0 }, { 10,  1,  2,  2,  0 }, { 10,  1,  2,  2,  0 }, { 10,  1,  2,  2,  0 },
 { 10,  2,  2,  1,  0 }, { 10,  2,  2,  1,  0 }, { 10,  2,  2,  1,  0 }, { 10,  2,  2,  1,  0 },
 { 10,  2,  1,  2,  0 }, { 10,  2,  1,  2,  0 }, { 10,  2,  1,  2,  0 }, { 10,  2,  1,  2,  0 },
 { 10,  0,  2,  2,  1 }, { 10,  0,  2,  2,  1 }, { 10,  0,  2,  2,  1 }, { 10,  0,  2,  2,  1 },
 { 10,  0,  1,  2,  2 }, { 10,  0,  1,  2,  2 }, { 10,  0,  1,  2,  2 }, { 10,  0,  1,  2,  2 },
 { 10,  2,  2,  0,  1 }, { 10,  2,  2,  0,  1 }, { 10,  2,  2,  0,  1 }, { 10,  2,  2,  0,  1 },
 { 10,  0,  2,  1,  2 }, { 10,  0,  2,  1,  2 }, { 10,  0,  2,  1,  2 }, { 10,  0,  2,  1,  2 },
 { 10,  2,  0,  2,  1 }, { 10,  2,  0,  2,  1 }, { 10,  2,  0,  2,  1 }, { 10,  2,  0,  2,  1 },
 { 10,  1,  0,  2,  2 }, { 10,  1,  0,  2,  2 }, { 10,  1,  0,  2,  2 }, { 10,  1,  0,  2,  2 },
 { 10,  2,  2,  2,  1 }, { 10,  2,  2,  2,  1 }, { 10,  2,  2,  2,  1 }, { 10,  2,  2,  2,  1 },
 { 10,  1,  2,  0,  2 }, { 10,  1,  2,  0,  2 }, { 10,  1,  2,  0,  2 }, { 10,  1,  2,  0,  2 },
 { 10,  2,  0,  1,  2 }, { 10,  2,  0,  1,  2 }, { 10,  2,  0,  1,  2 }, { 10,  2,  0,  1,  2 },
 { 10,  2,  1,  0,  2 }, { 10,  2,  1,  0,  2 }, { 10,  2,  1,  0,  2 }, { 10,  2,  1,  0,  2 },
 { 10,  1,  2,  2,  2 }, { 10,  1,  2,  2,  2 }, { 10,  1,  2,  2,  2 }, { 10,  1,  2,  2,  2 },
 { 11,  2,  1,  2,  2 }, { 11,  2,  1,  2,  2 }, { 11,  2,  2,  1,  2 }, { 11,  2,  2,  1,  2 },
 { 11,  0,  2,  2,  0 }, { 11,  0,  2,  2,  0 }, { 11,  2,  2,  0,  0 }, { 11,  2,  2,  0,  0 },
 { 11,  0,  0,  2,  2 }, { 11,  0,  0,  2,  2 }, { 11,  2,  0,  2,  0 }, { 11,  2,  0,  2,  0 },
 { 11,  0,  2,  0,  2 }, { 11,  0,  2,  0,  2 }, { 11,  2,  0,  0,  2 }, { 11,  2,  0,  0,  2 },
 { 11,  2,  2,  2,  2 }, { 11,  2,  2,  2,  2 }, { 11,  0,  2,  2,  2 }, { 11,  0,  2,  2,  2 },
 { 11,  2,  2,  2,  0 }, { 11,  2,  2,  2,  0 }, { 12,  2,  2,  0,  2 }, { 12,  2,  0,  2,  2 }
};

static hcb_2_quad const* hcb_2_quad_table[] __attribute__(   (  section( ".data" )  )   ) = {
 0, hcb1_2, hcb2_2, 0, hcb4_2, 0, 0, 0, 0, 0, 0, 0
};

static const short hcb_2_quad_table_size[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 114, 86, 0, 185, 0, 0, 0, 0, 0, 0, 0
};

static const hcb_bin_quad hcb3[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  0, {  1,  2, 0, 0 }  }, {  1, {  0,  0, 0, 0 }  }, {  0, {  1,  2, 0, 0 }  }, {  0, {  2,  3, 0, 0 }  },
 {  0, {  3,  4, 0, 0 }  }, {  0, {  4,  5, 0, 0 }  }, {  0, {  5,  6, 0, 0 }  }, {  0, {  6,  7, 0, 0 }  },
 {  0, {  7,  8, 0, 0 }  }, {  1, {  1,  0, 0, 0 }  }, {  1, {  0,  0, 0, 1 }  }, {  1, {  0,  1, 0, 0 }  },
 {  1, {  0,  0, 1, 0 }  }, {  0, {  4,  5, 0, 0 }  }, {  0, {  5,  6, 0, 0 }  }, {  0, {  6,  7, 0, 0 }  },
 {  0, {  7,  8, 0, 0 }  }, {  1, {  1,  1, 0, 0 }  }, {  1, {  0,  0, 1, 1 }  }, {  0, {  6,  7, 0, 0 }  },
 {  0, {  7,  8, 0, 0 }  }, {  0, {  8,  9, 0, 0 }  }, {  0, {  9, 10, 0, 0 }  }, {  0, { 10, 11, 0, 0 }  },
 {  0, { 11, 12, 0, 0 }  }, {  1, {  0,  1, 1, 0 }  }, {  1, {  0,  1, 0, 1 }  }, {  1, {  1,  0, 1, 0 }  },
 {  1, {  0,  1, 1, 1 }  }, {  1, {  1,  0, 0, 1 }  }, {  1, {  1,  1, 1, 0 }  }, {  0, {  6,  7, 0, 0 }  },
 {  0, {  7,  8, 0, 0 }  }, {  0, {  8,  9, 0, 0 }  }, {  0, {  9, 10, 0, 0 }  }, {  0, { 10, 11, 0, 0 }  },
 {  0, { 11, 12, 0, 0 }  }, {  1, {  1,  1, 1, 1 }  }, {  1, {  1,  0, 1, 1 }  }, {  1, {  1,  1, 0, 1 }  },
 {  0, {  9, 10, 0, 0 }  }, {  0, { 10, 11, 0, 0 }  }, {  0, { 11, 12, 0, 0 }  }, {  0, { 12, 13, 0, 0 }  },
 {  0, { 13, 14, 0, 0 }  }, {  0, { 14, 15, 0, 0 }  }, {  0, { 15, 16, 0, 0 }  }, {  0, { 16, 17, 0, 0 }  },
 {  0, { 17, 18, 0, 0 }  }, {  1, {  2,  0, 0, 0 }  }, {  1, {  0,  0, 0, 2 }  }, {  1, {  0,  0, 1, 2 }  },
 {  1, {  2,  1, 0, 0 }  }, {  1, {  1,  2, 1, 0 }  }, {  0, { 13, 14, 0, 0 }  }, {  0, { 14, 15, 0, 0 }  },
 {  0, { 15, 16, 0, 0 }  }, {  0, { 16, 17, 0, 0 }  }, {  0, { 17, 18, 0, 0 }  }, {  0, { 18, 19, 0, 0 }  },
 {  0, { 19, 20, 0, 0 }  }, {  0, { 20, 21, 0, 0 }  }, {  0, { 21, 22, 0, 0 }  }, {  0, { 22, 23, 0, 0 }  },
 {  0, { 23, 24, 0, 0 }  }, {  0, { 24, 25, 0, 0 }  }, {  0, { 25, 26, 0, 0 }  }, {  1, {  0,  0, 2, 1 }  },
 {  1, {  0,  1, 2, 1 }  }, {  1, {  1,  2, 0, 0 }  }, {  1, {  0,  1, 1, 2 }  }, {  1, {  2,  1, 1, 0 }  },
 {  1, {  0,  0, 2, 0 }  }, {  1, {  0,  2, 1, 0 }  }, {  1, {  0,  1, 2, 0 }  }, {  1, {  0,  2, 0, 0 }  },
 {  1, {  0,  1, 0, 2 }  }, {  1, {  2,  0, 1, 0 }  }, {  1, {  1,  2, 1, 1 }  }, {  1, {  0,  2, 1, 1 }  },
 {  1, {  1,  1, 2, 0 }  }, {  1, {  1,  1, 2, 1 }  }, {  0, { 11, 12, 0, 0 }  }, {  0, { 12, 13, 0, 0 }  },
 {  0, { 13, 14, 0, 0 }  }, {  0, { 14, 15, 0, 0 }  }, {  0, { 15, 16, 0, 0 }  }, {  0, { 16, 17, 0, 0 }  },
 {  0, { 17, 18, 0, 0 }  }, {  0, { 18, 19, 0, 0 }  }, {  0, { 19, 20, 0, 0 }  }, {  0, { 20, 21, 0, 0 }  },
 {  0, { 21, 22, 0, 0 }  }, {  1, {  1,  2, 0, 1 }  }, {  1, {  1,  0, 2, 0 }  }, {  1, {  1,  0, 2, 1 }  },
 {  1, {  0,  2, 0, 1 }  }, {  1, {  2,  1, 1, 1 }  }, {  1, {  1,  1, 1, 2 }  }, {  1, {  2,  1, 0, 1 }  },
 {  1, {  1,  0, 1, 2 }  }, {  1, {  0,  0, 2, 2 }  }, {  1, {  0,  1, 2, 2 }  }, {  1, {  2,  2, 1, 0 }  },
 {  1, {  1,  2, 2, 0 }  }, {  1, {  1,  0, 0, 2 }  }, {  1, {  2,  0, 0, 1 }  }, {  1, {  0,  2, 2, 1 }  },
 {  0, {  7,  8, 0, 0 }  }, {  0, {  8,  9, 0, 0 }  }, {  0, {  9, 10, 0, 0 }  }, {  0, { 10, 11, 0, 0 }  },
 {  0, { 11, 12, 0, 0 }  }, {  0, { 12, 13, 0, 0 }  }, {  0, { 13, 14, 0, 0 }  }, {  1, {  2,  2, 0, 0 }  },
 {  1, {  1,  2, 2, 1 }  }, {  1, {  1,  1, 0, 2 }  }, {  1, {  2,  0, 1, 1 }  }, {  1, {  1,  1, 2, 2 }  },
 {  1, {  2,  2, 1, 1 }  }, {  1, {  0,  2, 2, 0 }  }, {  1, {  0,  2, 1, 2 }  }, {  0, {  6,  7, 0, 0 }  },
 {  0, {  7,  8, 0, 0 }  }, {  0, {  8,  9, 0, 0 }  }, {  0, {  9, 10, 0, 0 }  }, {  0, { 10, 11, 0, 0 }  },
 {  0, { 11, 12, 0, 0 }  }, {  1, {  1,  0, 2, 2 }  }, {  1, {  2,  2, 0, 1 }  }, {  1, {  2,  1, 2, 0 }  },
 {  1, {  2,  2, 2, 0 }  }, {  1, {  0,  2, 2, 2 }  }, {  1, {  2,  2, 2, 1 }  }, {  1, {  2,  1, 2, 1 }  },
 {  1, {  1,  2, 1, 2 }  }, {  1, {  1,  2, 2, 2 }  }, {  0, {  3,  4, 0, 0 }  }, {  0, {  4,  5, 0, 0 }  },
 {  0, {  5,  6, 0, 0 }  }, {  1, {  0,  2, 0, 2 }  }, {  1, {  2,  0, 2, 0 }  }, {  1, {  1,  2, 0, 2 }  },
 {  0, {  3,  4, 0, 0 }  }, {  0, {  4,  5, 0, 0 }  }, {  0, {  5,  6, 0, 0 }  }, {  1, {  2,  0, 2, 1 }  },
 {  1, {  2,  1, 1, 2 }  }, {  1, {  2,  1, 0, 2 }  }, {  0, {  3,  4, 0, 0 }  }, {  0, {  4,  5, 0, 0 }  },
 {  0, {  5,  6, 0, 0 }  }, {  1, {  2,  2, 2, 2 }  }, {  1, {  2,  2, 1, 2 }  }, {  1, {  2,  1, 2, 2 }  },
 {  1, {  2,  0, 1, 2 }  }, {  1, {  2,  0, 0, 2 }  }, {  0, {  1,  2, 0, 0 }  }, {  1, {  2,  2, 0, 2 }  },
 {  1, {  2,  0, 2, 2 }  }
};

static const short int hcb_bin_table_size[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 0, 0, 161, 0, 161, 0, 127, 0, 337, 0, 0
};

static const hcb_bin_pair hcb5[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  0, {  1,  2 }  }, {  1, {  0,  0 }  }, {  0, {  1,  2 }  }, {  0, {  2,  3 }  },
 {  0, {  3,  4 }  }, {  0, {  4,  5 }  }, {  0, {  5,  6 }  }, {  0, {  6,  7 }  },
 {  0, {  7,  8 }  }, {  1, { -1,  0 }  }, {  1, {  1,  0 }  }, {  1, {  0,  1 }  },
 {  1, {  0, -1 }  }, {  0, {  4,  5 }  }, {  0, {  5,  6 }  }, {  0, {  6,  7 }  },
 {  0, {  7,  8 }  }, {  1, {  1, -1 }  }, {  1, { -1,  1 }  }, {  1, { -1, -1 }  },
 {  1, {  1,  1 }  }, {  0, {  4,  5 }  }, {  0, {  5,  6 }  }, {  0, {  6,  7 }  },
 {  0, {  7,  8 }  }, {  0, {  8,  9 }  }, {  0, {  9, 10 }  }, {  0, { 10, 11 }  },
 {  0, { 11, 12 }  }, {  0, { 12, 13 }  }, {  0, { 13, 14 }  }, {  0, { 14, 15 }  },
 {  0, { 15, 16 }  }, {  1, { -2,  0 }  }, {  1, {  0,  2 }  }, {  1, {  2,  0 }  },
 {  1, {  0, -2 }  }, {  0, { 12, 13 }  }, {  0, { 13, 14 }  }, {  0, { 14, 15 }  },
 {  0, { 15, 16 }  }, {  0, { 16, 17 }  }, {  0, { 17, 18 }  }, {  0, { 18, 19 }  },
 {  0, { 19, 20 }  }, {  0, { 20, 21 }  }, {  0, { 21, 22 }  }, {  0, { 22, 23 }  },
 {  0, { 23, 24 }  }, {  1, { -2, -1 }  }, {  1, {  2,  1 }  }, {  1, { -1, -2 }  },
 {  1, {  1,  2 }  }, {  1, { -2,  1 }  }, {  1, {  2, -1 }  }, {  1, { -1,  2 }  },
 {  1, {  1, -2 }  }, {  1, { -3,  0 }  }, {  1, {  3,  0 }  }, {  1, {  0, -3 }  },
 {  1, {  0,  3 }  }, {  0, { 12, 13 }  }, {  0, { 13, 14 }  }, {  0, { 14, 15 }  },
 {  0, { 15, 16 }  }, {  0, { 16, 17 }  }, {  0, { 17, 18 }  }, {  0, { 18, 19 }  },
 {  0, { 19, 20 }  }, {  0, { 20, 21 }  }, {  0, { 21, 22 }  }, {  0, { 22, 23 }  },
 {  0, { 23, 24 }  }, {  1, { -3, -1 }  }, {  1, {  1,  3 }  }, {  1, {  3,  1 }  },
 {  1, { -1, -3 }  }, {  1, { -3,  1 }  }, {  1, {  3, -1 }  }, {  1, {  1, -3 }  },
 {  1, { -1,  3 }  }, {  1, { -2,  2 }  }, {  1, {  2,  2 }  }, {  1, { -2, -2 }  },
 {  1, {  2, -2 }  }, {  0, { 12, 13 }  }, {  0, { 13, 14 }  }, {  0, { 14, 15 }  },
 {  0, { 15, 16 }  }, {  0, { 16, 17 }  }, {  0, { 17, 18 }  }, {  0, { 18, 19 }  },
 {  0, { 19, 20 }  }, {  0, { 20, 21 }  }, {  0, { 21, 22 }  }, {  0, { 22, 23 }  },
 {  0, { 23, 24 }  }, {  1, { -3, -2 }  }, {  1, {  3, -2 }  }, {  1, { -2,  3 }  },
 {  1, {  2, -3 }  }, {  1, {  3,  2 }  }, {  1, {  2,  3 }  }, {  1, { -3,  2 }  },
 {  1, { -2, -3 }  }, {  1, {  0, -4 }  }, {  1, { -4,  0 }  }, {  1, {  4,  1 }  },
 {  1, {  4,  0 }  }, {  0, { 12, 13 }  }, {  0, { 13, 14 }  }, {  0, { 14, 15 }  },
 {  0, { 15, 16 }  }, {  0, { 16, 17 }  }, {  0, { 17, 18 }  }, {  0, { 18, 19 }  },
 {  0, { 19, 20 }  }, {  0, { 20, 21 }  }, {  0, { 21, 22 }  }, {  0, { 22, 23 }  },
 {  0, { 23, 24 }  }, {  1, { -4, -1 }  }, {  1, {  0,  4 }  }, {  1, {  4, -1 }  },
 {  1, { -1, -4 }  }, {  1, {  1,  4 }  }, {  1, { -1,  4 }  }, {  1, { -4,  1 }  },
 {  1, {  1, -4 }  }, {  1, {  3, -3 }  }, {  1, { -3, -3 }  }, {  1, { -3,  3 }  },
 {  1, { -2,  4 }  }, {  1, { -4, -2 }  }, {  1, {  4,  2 }  }, {  1, {  2, -4 }  },
 {  1, {  2,  4 }  }, {  1, {  3,  3 }  }, {  1, { -4,  2 }  }, {  0, {  6,  7 }  },
 {  0, {  7,  8 }  }, {  0, {  8,  9 }  }, {  0, {  9, 10 }  }, {  0, { 10, 11 }  },
 {  0, { 11, 12 }  }, {  1, { -2, -4 }  }, {  1, {  4, -2 }  }, {  1, {  3, -4 }  },
 {  1, { -4, -3 }  }, {  1, { -4,  3 }  }, {  1, {  3,  4 }  }, {  1, { -3,  4 }  },
 {  1, {  4,  3 }  }, {  1, {  4, -3 }  }, {  1, { -3, -4 }  }, {  0, {  2,  3 }  },
 {  0, {  3,  4 }  }, {  1, {  4, -4 }  }, {  1, { -4,  4 }  }, {  1, {  4,  4 }  },
 {  1, { -4, -4 }  }
};

static const hcb_bin_pair hcb7[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  0, {  1,  2 }  }, {  1, {  0,  0 }  }, {  0, {  1,  2 }  }, {  0, {  2,  3 }  },
 {  0, {  3,  4 }  }, {  1, {  1,  0 }  }, {  1, {  0,  1 }  }, {  0, {  2,  3 }  },
 {  0, {  3,  4 }  }, {  1, {  1,  1 }  }, {  0, {  3,  4 }  }, {  0, {  4,  5 }  },
 {  0, {  5,  6 }  }, {  0, {  6,  7 }  }, {  0, {  7,  8 }  }, {  0, {  8,  9 }  },
 {  0, {  9, 10 }  }, {  0, { 10, 11 }  }, {  0, { 11, 12 }  }, {  1, {  2,  1 }  },
 {  1, {  1,  2 }  }, {  1, {  2,  0 }  }, {  1, {  0,  2 }  }, {  0, {  8,  9 }  },
 {  0, {  9, 10 }  }, {  0, { 10, 11 }  }, {  0, { 11, 12 }  }, {  0, { 12, 13 }  },
 {  0, { 13, 14 }  }, {  0, { 14, 15 }  }, {  0, { 15, 16 }  }, {  1, {  3,  1 }  },
 {  1, {  1,  3 }  }, {  1, {  2,  2 }  }, {  1, {  3,  0 }  }, {  1, {  0,  3 }  },
 {  0, { 11, 12 }  }, {  0, { 12, 13 }  }, {  0, { 13, 14 }  }, {  0, { 14, 15 }  },
 {  0, { 15, 16 }  }, {  0, { 16, 17 }  }, {  0, { 17, 18 }  }, {  0, { 18, 19 }  },
 {  0, { 19, 20 }  }, {  0, { 20, 21 }  }, {  0, { 21, 22 }  }, {  1, {  2,  3 }  },
 {  1, {  3,  2 }  }, {  1, {  1,  4 }  }, {  1, {  4,  1 }  }, {  1, {  1,  5 }  },
 {  1, {  5,  1 }  }, {  1, {  3,  3 }  }, {  1, {  2,  4 }  }, {  1, {  0,  4 }  },
 {  1, {  4,  0 }  }, {  0, { 12, 13 }  }, {  0, { 13, 14 }  }, {  0, { 14, 15 }  },
 {  0, { 15, 16 }  }, {  0, { 16, 17 }  }, {  0, { 17, 18 }  }, {  0, { 18, 19 }  },
 {  0, { 19, 20 }  }, {  0, { 20, 21 }  }, {  0, { 21, 22 }  }, {  0, { 22, 23 }  },
 {  0, { 23, 24 }  }, {  1, {  4,  2 }  }, {  1, {  2,  5 }  }, {  1, {  5,  2 }  },
 {  1, {  0,  5 }  }, {  1, {  6,  1 }  }, {  1, {  5,  0 }  }, {  1, {  1,  6 }  },
 {  1, {  4,  3 }  }, {  1, {  3,  5 }  }, {  1, {  3,  4 }  }, {  1, {  5,  3 }  },
 {  1, {  2,  6 }  }, {  1, {  6,  2 }  }, {  1, {  1,  7 }  }, {  0, { 10, 11 }  },
 {  0, { 11, 12 }  }, {  0, { 12, 13 }  }, {  0, { 13, 14 }  }, {  0, { 14, 15 }  },
 {  0, { 15, 16 }  }, {  0, { 16, 17 }  }, {  0, { 17, 18 }  }, {  0, { 18, 19 }  },
 {  0, { 19, 20 }  }, {  1, {  3,  6 }  }, {  1, {  0,  6 }  }, {  1, {  6,  0 }  },
 {  1, {  4,  4 }  }, {  1, {  7,  1 }  }, {  1, {  4,  5 }  }, {  1, {  7,  2 }  },
 {  1, {  5,  4 }  }, {  1, {  6,  3 }  }, {  1, {  2,  7 }  }, {  1, {  7,  3 }  },
 {  1, {  6,  4 }  }, {  1, {  5,  5 }  }, {  1, {  4,  6 }  }, {  1, {  3,  7 }  },
 {  0, {  5,  6 }  }, {  0, {  6,  7 }  }, {  0, {  7,  8 }  }, {  0, {  8,  9 }  },
 {  0, {  9, 10 }  }, {  1, {  7,  0 }  }, {  1, {  0,  7 }  }, {  1, {  6,  5 }  },
 {  1, {  5,  6 }  }, {  1, {  7,  4 }  }, {  1, {  4,  7 }  }, {  1, {  5,  7 }  },
 {  1, {  7,  5 }  }, {  0, {  2,  3 }  }, {  0, {  3,  4 }  }, {  1, {  7,  6 }  },
 {  1, {  6,  6 }  }, {  1, {  6,  7 }  }, {  1, {  7,  7 }  }
};

static const hcb_bin_pair hcb9[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  0, {  1,  2 }  }, {  1, {  0,  0 }  }, {  0, {  1,  2 }  }, {  0, {  2,  3 }  },
 {  0, {  3,  4 }  }, {  1, {  1,  0 }  }, {  1, {  0,  1 }  }, {  0, {  2,  3 }  },
 {  0, {  3,  4 }  }, {  1, {  1,  1 }  }, {  0, {  3,  4 }  }, {  0, {  4,  5 }  },
 {  0, {  5,  6 }  }, {  0, {  6,  7 }  }, {  0, {  7,  8 }  }, {  0, {  8,  9 }  },
 {  0, {  9, 10 }  }, {  0, { 10, 11 }  }, {  0, { 11, 12 }  }, {  1, {  2,  1 }  },
 {  1, {  1,  2 }  }, {  1, {  2,  0 }  }, {  1, {  0,  2 }  }, {  0, {  8,  9 }  },
 {  0, {  9, 10 }  }, {  0, { 10, 11 }  }, {  0, { 11, 12 }  }, {  0, { 12, 13 }  },
 {  0, { 13, 14 }  }, {  0, { 14, 15 }  }, {  0, { 15, 16 }  }, {  1, {  3,  1 }  },
 {  1, {  2,  2 }  }, {  1, {  1,  3 }  }, {  0, { 13, 14 }  }, {  0, { 14, 15 }  },
 {  0, { 15, 16 }  }, {  0, { 16, 17 }  }, {  0, { 17, 18 }  }, {  0, { 18, 19 }  },
 {  0, { 19, 20 }  }, {  0, { 20, 21 }  }, {  0, { 21, 22 }  }, {  0, { 22, 23 }  },
 {  0, { 23, 24 }  }, {  0, { 24, 25 }  }, {  0, { 25, 26 }  }, {  1, {  3,  0 }  },
 {  1, {  0,  3 }  }, {  1, {  2,  3 }  }, {  1, {  3,  2 }  }, {  1, {  1,  4 }  },
 {  1, {  4,  1 }  }, {  1, {  2,  4 }  }, {  1, {  1,  5 }  }, {  0, { 18, 19 }  },
 {  0, { 19, 20 }  }, {  0, { 20, 21 }  }, {  0, { 21, 22 }  }, {  0, { 22, 23 }  },
 {  0, { 23, 24 }  }, {  0, { 24, 25 }  }, {  0, { 25, 26 }  }, {  0, { 26, 27 }  },
 {  0, { 27, 28 }  }, {  0, { 28, 29 }  }, {  0, { 29, 30 }  }, {  0, { 30, 31 }  },
 {  0, { 31, 32 }  }, {  0, { 32, 33 }  }, {  0, { 33, 34 }  }, {  0, { 34, 35 }  },
 {  0, { 35, 36 }  }, {  1, {  4,  2 }  }, {  1, {  3,  3 }  }, {  1, {  0,  4 }  },
 {  1, {  4,  0 }  }, {  1, {  5,  1 }  }, {  1, {  2,  5 }  }, {  1, {  1,  6 }  },
 {  1, {  3,  4 }  }, {  1, {  5,  2 }  }, {  1, {  6,  1 }  }, {  1, {  4,  3 }  },
 {  0, { 25, 26 }  }, {  0, { 26, 27 }  }, {  0, { 27, 28 }  }, {  0, { 28, 29 }  },
 {  0, { 29, 30 }  }, {  0, { 30, 31 }  }, {  0, { 31, 32 }  }, {  0, { 32, 33 }  },
 {  0, { 33, 34 }  }, {  0, { 34, 35 }  }, {  0, { 35, 36 }  }, {  0, { 36, 37 }  },
 {  0, { 37, 38 }  }, {  0, { 38, 39 }  }, {  0, { 39, 40 }  }, {  0, { 40, 41 }  },
 {  0, { 41, 42 }  }, {  0, { 42, 43 }  }, {  0, { 43, 44 }  }, {  0, { 44, 45 }  },
 {  0, { 45, 46 }  }, {  0, { 46, 47 }  }, {  0, { 47, 48 }  }, {  0, { 48, 49 }  },
 {  0, { 49, 50 }  }, {  1, {  0,  5 }  }, {  1, {  2,  6 }  }, {  1, {  5,  0 }  },
 {  1, {  1,  7 }  }, {  1, {  3,  5 }  }, {  1, {  1,  8 }  }, {  1, {  8,  1 }  },
 {  1, {  4,  4 }  }, {  1, {  5,  3 }  }, {  1, {  6,  2 }  }, {  1, {  7,  1 }  },
 {  1, {  0,  6 }  }, {  1, {  8,  2 }  }, {  1, {  2,  8 }  }, {  1, {  3,  6 }  },
 {  1, {  2,  7 }  }, {  1, {  4,  5 }  }, {  1, {  9,  1 }  }, {  1, {  1,  9 }  },
 {  1, {  7,  2 }  }, {  0, { 30, 31 }  }, {  0, { 31, 32 }  }, {  0, { 32, 33 }  },
 {  0, { 33, 34 }  }, {  0, { 34, 35 }  }, {  0, { 35, 36 }  }, {  0, { 36, 37 }  },
 {  0, { 37, 38 }  }, {  0, { 38, 39 }  }, {  0, { 39, 40 }  }, {  0, { 40, 41 }  },
 {  0, { 41, 42 }  }, {  0, { 42, 43 }  }, {  0, { 43, 44 }  }, {  0, { 44, 45 }  },
 {  0, { 45, 46 }  }, {  0, { 46, 47 }  }, {  0, { 47, 48 }  }, {  0, { 48, 49 }  },
 {  0, { 49, 50 }  }, {  0, { 50, 51 }  }, {  0, { 51, 52 }  }, {  0, { 52, 53 }  },
 {  0, { 53, 54 }  }, {  0, { 54, 55 }  }, {  0, { 55, 56 }  }, {  0, { 56, 57 }  },
 {  0, { 57, 58 }  }, {  0, { 58, 59 }  }, {  0, { 59, 60 }  }, {  1, {  6,  0 }  },
 {  1, {  5,  4 }  }, {  1, {  6,  3 }  }, {  1, {  8,  3 }  }, {  1, {  0,  7 }  },
 {  1, {  9,  2 }  }, {  1, {  3,  8 }  }, {  1, {  4,  6 }  }, {  1, {  3,  7 }  },
 {  1, {  0,  8 }  }, {  1, { 10,  1 }  }, {  1, {  6,  4 }  }, {  1, {  2,  9 }  },
 {  1, {  5,  5 }  }, {  1, {  8,  0 }  }, {  1, {  7,  0 }  }, {  1, {  7,  3 }  },
 {  1, { 10,  2 }  }, {  1, {  9,  3 }  }, {  1, {  8,  4 }  }, {  1, {  1, 10 }  },
 {  1, {  7,  4 }  }, {  1, {  6,  5 }  }, {  1, {  5,  6 }  }, {  1, {  4,  8 }  },
 {  1, {  4,  7 }  }, {  1, {  3,  9 }  }, {  1, { 11,  1 }  }, {  1, {  5,  8 }  },
 {  1, {  9,  0 }  }, {  1, {  8,  5 }  }, {  0, { 29, 30 }  }, {  0, { 30, 31 }  },
 {  0, { 31, 32 }  }, {  0, { 32, 33 }  }, {  0, { 33, 34 }  }, {  0, { 34, 35 }  },
 {  0, { 35, 36 }  }, {  0, { 36, 37 }  }, {  0, { 37, 38 }  }, {  0, { 38, 39 }  },
 {  0, { 39, 40 }  }, {  0, { 40, 41 }  }, {  0, { 41, 42 }  }, {  0, { 42, 43 }  },
 {  0, { 43, 44 }  }, {  0, { 44, 45 }  }, {  0, { 45, 46 }  }, {  0, { 46, 47 }  },
 {  0, { 47, 48 }  }, {  0, { 48, 49 }  }, {  0, { 49, 50 }  }, {  0, { 50, 51 }  },
 {  0, { 51, 52 }  }, {  0, { 52, 53 }  }, {  0, { 53, 54 }  }, {  0, { 54, 55 }  },
 {  0, { 55, 56 }  }, {  0, { 56, 57 }  }, {  0, { 57, 58 }  }, {  1, { 10,  3 }  },
 {  1, {  2, 10 }  }, {  1, {  0,  9 }  }, {  1, { 11,  2 }  }, {  1, {  9,  4 }  },
 {  1, {  6,  6 }  }, {  1, { 12,  1 }  }, {  1, {  4,  9 }  }, {  1, {  8,  6 }  },
 {  1, {  1, 11 }  }, {  1, {  9,  5 }  }, {  1, { 10,  4 }  }, {  1, {  5,  7 }  },
 {  1, {  7,  5 }  }, {  1, {  2, 11 }  }, {  1, {  1, 12 }  }, {  1, { 12,  2 }  },
 {  1, { 11,  3 }  }, {  1, {  3, 10 }  }, {  1, {  5,  9 }  }, {  1, {  6,  7 }  },
 {  1, {  8,  7 }  }, {  1, { 11,  4 }  }, {  1, {  0, 10 }  }, {  1, {  7,  6 }  },
 {  1, { 12,  3 }  }, {  1, { 10,  0 }  }, {  1, { 10,  5 }  }, {  1, {  4, 10 }  },
 {  1, {  6,  8 }  }, {  1, {  2, 12 }  }, {  1, {  9,  6 }  }, {  1, {  9,  7 }  },
 {  1, {  4, 11 }  }, {  1, { 11,  0 }  }, {  1, {  6,  9 }  }, {  1, {  3, 11 }  },
 {  1, {  5, 10 }  }, {  0, { 20, 21 }  }, {  0, { 21, 22 }  }, {  0, { 22, 23 }  },
 {  0, { 23, 24 }  }, {  0, { 24, 25 }  }, {  0, { 25, 26 }  }, {  0, { 26, 27 }  },
 {  0, { 27, 28 }  }, {  0, { 28, 29 }  }, {  0, { 29, 30 }  }, {  0, { 30, 31 }  },
 {  0, { 31, 32 }  }, {  0, { 32, 33 }  }, {  0, { 33, 34 }  }, {  0, { 34, 35 }  },
 {  0, { 35, 36 }  }, {  0, { 36, 37 }  }, {  0, { 37, 38 }  }, {  0, { 38, 39 }  },
 {  0, { 39, 40 }  }, {  1, {  8,  8 }  }, {  1, {  7,  8 }  }, {  1, { 12,  5 }  },
 {  1, {  3, 12 }  }, {  1, { 11,  5 }  }, {  1, {  7,  7 }  }, {  1, { 12,  4 }  },
 {  1, { 11,  6 }  }, {  1, { 10,  6 }  }, {  1, {  4, 12 }  }, {  1, {  7,  9 }  },
 {  1, {  5, 11 }  }, {  1, {  0, 11 }  }, {  1, { 12,  6 }  }, {  1, {  6, 10 }  },
 {  1, { 12,  0 }  }, {  1, { 10,  7 }  }, {  1, {  5, 12 }  }, {  1, {  7, 10 }  },
 {  1, {  9,  8 }  }, {  1, {  0, 12 }  }, {  1, { 11,  7 }  }, {  1, {  8,  9 }  },
 {  1, {  9,  9 }  }, {  1, { 10,  8 }  }, {  1, {  7, 11 }  }, {  1, { 12,  7 }  },
 {  1, {  6, 11 }  }, {  1, {  8, 11 }  }, {  1, { 11,  8 }  }, {  1, {  7, 12 }  },
 {  1, {  6, 12 }  }, {  0, {  8,  9 }  }, {  0, {  9, 10 }  }, {  0, { 10, 11 }  },
 {  0, { 11, 12 }  }, {  0, { 12, 13 }  }, {  0, { 13, 14 }  }, {  0, { 14, 15 }  },
 {  0, { 15, 16 }  }, {  1, {  8, 10 }  }, {  1, { 10,  9 }  }, {  1, {  8, 12 }  },
 {  1, {  9, 10 }  }, {  1, {  9, 11 }  }, {  1, {  9, 12 }  }, {  1, { 10, 11 }  },
 {  1, { 12,  9 }  }, {  1, { 10, 10 }  }, {  1, { 11,  9 }  }, {  1, { 12,  8 }  },
 {  1, { 11, 10 }  }, {  1, { 12, 10 }  }, {  1, { 12, 11 }  }, {  0, {  2,  3 }  },
 {  0, {  3,  4 }  }, {  1, { 10, 12 }  }, {  1, { 11, 11 }  }, {  1, { 11, 12 }  },
 {  1, { 12, 12 }  }
};

static hcb_bin_pair const* hcb_bin_table[] __attribute__(   (  section( ".data" )  )   ) = {
 0, 0, 0, 0, 0, hcb5, 0, hcb7, 0, hcb9, 0, 0
};

static const hcb_2_pair hcb6_2[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  4,  0,  0 }, {  4,  1,  0 }, {  4,  0, -1 }, {  4,  0,  1 },
 {  4, -1,  0 }, {  4,  1,  1 }, {  4, -1,  1 }, {  4,  1, -1 },
 {  4, -1, -1 }, {  6,  2, -1 }, {  6,  2,  1 }, {  6, -2,  1 },
 {  6, -2, -1 }, {  6, -2,  0 }, {  6, -1,  2 }, {  6,  2,  0 },
 {  6,  1, -2 }, {  6,  1,  2 }, {  6,  0, -2 }, {  6, -1, -2 },
 {  6,  0,  2 }, {  6,  2, -2 }, {  6, -2,  2 }, {  6, -2, -2 },
 {  6,  2,  2 }, {  7, -3,  1 }, {  7,  3,  1 }, {  7,  3, -1 },
 {  7, -1,  3 }, {  7, -3, -1 }, {  7,  1,  3 }, {  7,  1, -3 },
 {  7, -1, -3 }, {  7,  3,  0 }, {  7, -3,  0 }, {  7,  0, -3 },
 {  7,  0,  3 }, {  7,  3,  2 }, {  7,  3,  2 }, {  8, -3, -2 },
 {  8, -2,  3 }, {  8,  2,  3 }, {  8,  3, -2 }, {  8,  2, -3 },
 {  8, -2, -3 }, {  8, -3,  2 }, {  8, -3,  2 }, {  8,  3,  3 },
 {  8,  3,  3 }, {  9,  3, -3 }, {  9, -3, -3 }, {  9, -3,  3 },
 {  9,  1, -4 }, {  9, -1, -4 }, {  9,  4,  1 }, {  9, -4,  1 },
 {  9, -4, -1 }, {  9,  1,  4 }, {  9,  4, -1 }, {  9, -1,  4 },
 {  9,  0, -4 }, {  9, -4,  2 }, {  9, -4,  2 }, {  9, -4,  2 },
 {  9, -4,  2 }, {  9, -4, -2 }, {  9, -4, -2 }, {  9, -4, -2 },
 {  9, -4, -2 }, {  9,  2,  4 }, {  9,  2,  4 }, {  9,  2,  4 },
 {  9,  2,  4 }, {  9, -2, -4 }, {  9, -2, -4 }, {  9, -2, -4 },
 {  9, -2, -4 }, {  9, -4,  0 }, {  9, -4,  0 }, {  9, -4,  0 },
 {  9, -4,  0 }, {  9,  4,  2 }, {  9,  4,  2 }, {  9,  4,  2 },
 {  9,  4,  2 }, {  9,  4, -2 }, {  9,  4, -2 }, {  9,  4, -2 },
 {  9,  4, -2 }, {  9, -2,  4 }, {  9, -2,  4 }, {  9, -2,  4 },
 {  9, -2,  4 }, {  9,  4,  0 }, {  9,  4,  0 }, {  9,  4,  0 },
 {  9,  4,  0 }, {  9,  2, -4 }, {  9,  2, -4 }, {  9,  2, -4 },
 {  9,  2, -4 }, {  9,  0,  4 }, {  9,  0,  4 }, {  9,  0,  4 },
 {  9,  0,  4 }, { 10, -3, -4 }, { 10, -3, -4 }, { 10, -3,  4 },
 { 10, -3,  4 }, { 10,  3, -4 }, { 10,  3, -4 }, { 10,  4, -3 },
 { 10,  4, -3 }, { 10,  3,  4 }, { 10,  3,  4 }, { 10,  4,  3 },
 { 10,  4,  3 }, { 10, -4,  3 }, { 10, -4,  3 }, { 10, -4, -3 },
 { 10, -4, -3 }, { 11,  4,  4 }, { 11, -4,  4 }, { 11, -4, -4 },
 { 11,  4, -4 }
};

static const hcb_2_pair hcb8_2[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  3,  1,  1 }, {  4,  2,  1 }, {  4,  1,  0 }, {  4,  1,  2 },
 {  4,  0,  1 }, {  4,  2,  2 }, {  5,  0,  0 }, {  5,  2,  0 },
 {  5,  0,  2 }, {  5,  3,  1 }, {  5,  1,  3 }, {  5,  3,  2 },
 {  5,  2,  3 }, {  6,  3,  3 }, {  6,  4,  1 }, {  6,  1,  4 },
 {  6,  4,  2 }, {  6,  2,  4 }, {  6,  3,  0 }, {  6,  0,  3 },
 {  6,  4,  3 }, {  6,  3,  4 }, {  6,  5,  2 }, {  7,  5,  1 },
 {  7,  2,  5 }, {  7,  1,  5 }, {  7,  5,  3 }, {  7,  3,  5 },
 {  7,  4,  4 }, {  7,  5,  4 }, {  7,  0,  4 }, {  7,  4,  5 },
 {  7,  4,  0 }, {  7,  2,  6 }, {  7,  6,  2 }, {  7,  6,  1 },
 {  7,  6,  1 }, {  7,  1,  6 }, {  7,  1,  6 }, {  8,  3,  6 },
 {  8,  6,  3 }, {  8,  5,  5 }, {  8,  5,  0 }, {  8,  6,  4 },
 {  8,  0,  5 }, {  8,  4,  6 }, {  8,  7,  1 }, {  8,  7,  2 },
 {  8,  2,  7 }, {  8,  6,  5 }, {  8,  7,  3 }, {  8,  1,  7 },
 {  8,  1,  7 }, {  8,  1,  7 }, {  8,  1,  7 }, {  8,  5,  6 },
 {  8,  5,  6 }, {  8,  5,  6 }, {  8,  5,  6 }, {  8,  3,  7 },
 {  8,  3,  7 }, {  8,  3,  7 }, {  8,  3,  7 }, {  9,  6,  6 },
 {  9,  6,  6 }, {  9,  7,  4 }, {  9,  7,  4 }, {  9,  6,  0 },
 {  9,  6,  0 }, {  9,  4,  7 }, {  9,  4,  7 }, {  9,  0,  6 },
 {  9,  0,  6 }, {  9,  7,  5 }, {  9,  7,  5 }, {  9,  7,  6 },
 {  9,  7,  6 }, {  9,  6,  7 }, {  9,  6,  7 }, { 10,  5,  7 },
 { 10,  7,  0 }, { 10,  0,  7 }, { 10,  7,  7 }
};

static const hcb_2_pair hcb10_2[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  4,  1,  1 }, {  4,  1,  2 }, {  4,  2,  1 }, {  5,  2,  2 },
 {  5,  1,  0 }, {  5,  0,  1 }, {  5,  1,  3 }, {  5,  3,  2 },
 {  5,  3,  1 }, {  5,  2,  3 }, {  5,  3,  3 }, {  6,  2,  0 },
 {  6,  0,  2 }, {  6,  2,  4 }, {  6,  4,  2 }, {  6,  1,  4 },
 {  6,  4,  1 }, {  6,  0,  0 }, {  6,  4,  3 }, {  6,  3,  4 },
 {  6,  3,  0 }, {  6,  0,  3 }, {  6,  4,  4 }, {  6,  2,  5 },
 {  6,  5,  2 }, {  7,  1,  5 }, {  7,  5,  1 }, {  7,  5,  3 },
 {  7,  3,  5 }, {  7,  5,  4 }, {  7,  4,  5 }, {  7,  6,  2 },
 {  7,  2,  6 }, {  7,  6,  3 }, {  7,  4,  0 }, {  7,  6,  1 },
 {  7,  0,  4 }, {  7,  1,  6 }, {  7,  3,  6 }, {  7,  5,  5 },
 {  7,  6,  4 }, {  7,  4,  6 }, {  7,  4,  6 }, {  8,  6,  5 },
 {  8,  7,  2 }, {  8,  3,  7 }, {  8,  2,  7 }, {  8,  5,  6 },
 {  8,  8,  2 }, {  8,  7,  3 }, {  8,  5,  0 }, {  8,  7,  1 },
 {  8,  0,  5 }, {  8,  8,  1 }, {  8,  1,  7 }, {  8,  8,  3 },
 {  8,  7,  4 }, {  8,  4,  7 }, {  8,  2,  8 }, {  8,  6,  6 },
 {  8,  7,  5 }, {  8,  1,  8 }, {  8,  3,  8 }, {  8,  8,  4 },
 {  8,  4,  8 }, {  8,  5,  7 }, {  8,  5,  7 }, {  8,  8,  5 },
 {  8,  8,  5 }, {  8,  5,  8 }, {  8,  5,  8 }, {  9,  7,  6 },
 {  9,  6,  7 }, {  9,  9,  2 }, {  9,  6,  0 }, {  9,  6,  8 },
 {  9,  9,  3 }, {  9,  3,  9 }, {  9,  9,  1 }, {  9,  2,  9 },
 {  9,  0,  6 }, {  9,  8,  6 }, {  9,  9,  4 }, {  9,  4,  9 },
 {  9, 10,  2 }, {  9,  1,  9 }, {  9,  7,  7 }, {  9,  8,  7 },
 {  9,  9,  5 }, {  9,  7,  8 }, {  9, 10,  3 }, {  9,  5,  9 },
 {  9, 10,  4 }, {  9,  2, 10 }, {  9, 10,  1 }, {  9,  3, 10 },
 {  9,  9,  6 }, {  9,  6,  9 }, {  9,  6,  9 }, {  9,  8,  0 },
 {  9,  8,  0 }, {  9,  4, 10 }, {  9,  4, 10 }, {  9,  7,  0 }, 
 {  9,  7,  0 }, {  9, 11,  2 }, {  9, 11,  2 }, { 10,  7,  9 },
 { 10, 11,  3 }, { 10, 10,  6 }, { 10,  1, 10 }, { 10, 11,  1 },
 { 10,  9,  7 }, { 10,  0,  7 }, { 10,  8,  8 }, { 10, 10,  5 },
 { 10,  3, 11 }, { 10,  5, 10 }, { 10,  8,  9 }, { 10, 11,  5 },
 { 10,  0,  8 }, { 10, 11,  4 }, { 10,  2, 11 }, { 10,  7, 10 },
 { 10,  6, 10 }, { 10, 10,  7 }, { 10,  4, 11 }, { 10,  1, 11 },
 { 10, 12,  2 }, { 10,  9,  8 }, { 10, 12,  3 }, { 10, 11,  6 },
 { 10,  5, 11 }, { 10, 12,  4 }, { 10, 11,  7 }, { 10, 12,  5 },
 { 10,  3, 12 }, { 10,  6, 11 }, { 10,  9,  0 }, { 10, 10,  8 },
 { 10, 10,  0 }, { 10, 12,  1 }, { 10,  0,  9 }, { 10,  4, 12 },
 { 10,  9,  9 }, { 10, 12,  6 }, { 10, 12,  6 }, { 10, 12,  6 },
 { 10, 12,  6 }, { 10,  2, 12 }, { 10,  2, 12 }, { 10,  2, 12 },
 { 10,  2, 12 }, { 10,  8, 10 }, { 10,  8, 10 }, { 10,  8, 10 },
 { 10,  8, 10 }, { 11,  9, 10 }, { 11,  9, 10 }, { 11,  1, 12 },
 { 11,  1, 12 }, { 11, 11,  8 }, { 11, 11,  8 }, { 11, 12,  7 },
 { 11, 12,  7 }, { 11,  7, 11 }, { 11,  7, 11 }, { 11,  5, 12 },
 { 11,  5, 12 }, { 11,  6, 12 }, { 11,  6, 12 }, { 11, 10,  9 }, 
 { 11, 10,  9 }, { 11,  8, 11 }, { 11,  8, 11 }, { 11, 12,  8 },
 { 11, 12,  8 }, { 11,  0, 10 }, { 11,  0, 10 }, { 11,  7, 12 }, 
 { 11,  7, 12 }, { 11, 11,  0 }, { 11, 11,  0 }, { 11, 10, 10 }, 
 { 11, 10, 10 }, { 11, 11,  9 }, { 11, 11,  9 }, { 11, 11, 10 },
 { 11, 11, 10 }, { 11,  0, 11 }, { 11,  0, 11 }, { 11, 11, 11 },
 { 11, 11, 11 }, { 11,  9, 11 }, { 11,  9, 11 }, { 11, 10, 11 },
 { 11, 10, 11 }, { 11, 12,  0 }, { 11, 12,  0 }, { 11,  8, 12 },
 { 11,  8, 12 }, { 12, 12,  9 }, { 12, 10, 12 }, { 12,  9, 12 },
 { 12, 11, 12 }, { 12, 12, 11 }, { 12,  0, 12 }, { 12, 12, 10 },
 { 12, 12, 12 }
};

static const hcb_2_pair hcb11_2[] __attribute__(   (  section( ".rodata" )  )   ) = {
 {  4,  0,  0 }, {  4,  1,  1 }, {  5, 16, 16 }, {  5,  1,  0 },
 {  5,  0,  1 }, {  5,  2,  1 }, {  5,  1,  2 }, {  5,  2,  2 },
 {  6,  1,  3 }, {  6,  3,  1 }, {  6,  3,  2 }, {  6,  2,  0 },
 {  6,  2,  3 }, {  6,  0,  2 }, {  6,  3,  3 }, {  6,  3,  3 },
 {  7,  4,  1 }, {  7,  1,  4 }, {  7,  4,  2 }, {  7,  2,  4 },
 {  7,  4,  3 }, {  7,  3,  4 }, {  7,  3,  0 }, {  7,  0,  3 },
 {  7,  5,  1 }, {  7,  5,  2 }, {  7,  2,  5 }, {  7,  4,  4 },
 {  7,  1,  5 }, {  7,  5,  3 }, {  7,  3,  5 }, {  7,  3,  5 },
 {  7,  5,  4 }, {  7,  5,  4 }, {  8,  4,  5 }, {  8,  6,  2 },
 {  8,  2,  6 }, {  8,  6,  1 }, {  8,  6,  3 }, {  8,  3,  6 },
 {  8,  1,  6 }, {  8,  4, 16 }, {  8,  3, 16 }, {  8, 16,  5 },
 {  8, 16,  3 }, {  8, 16,  4 }, {  8,  6,  4 }, {  8, 16,  6 },
 {  8,  4,  0 }, {  8,  4,  6 }, {  8,  0,  4 }, {  8,  2, 16 },
 {  8,  5,  5 }, {  8,  5, 16 }, {  8, 16,  7 }, {  8, 16,  2 },
 {  8, 16,  8 }, {  8,  2,  7 }, {  8,  7,  2 }, {  8,  3,  7 },
 {  8,  6,  5 }, {  8,  5,  6 }, {  8,  6, 16 }, {  8, 16, 10 },
 {  8,  7,  3 }, {  8,  7,  1 }, {  8, 16,  9 }, {  8,  7, 16 },
 {  8,  1, 16 }, {  8,  1,  7 }, {  8,  4,  7 }, {  8, 16, 11 },
 {  8,  7,  4 }, {  8, 16, 12 }, {  8,  8, 16 }, {  8, 16,  1 },
 {  8,  6,  6 }, {  8,  9, 16 }, {  8,  2,  8 }, {  8,  5,  7 },
 {  8, 10, 16 }, {  8, 16, 13 }, {  8,  8,  3 }, {  8,  8,  2 },
 {  8,  3,  8 }, {  8,  5,  0 }, {  8, 16, 14 }, {  8, 16, 14 },
 {  8, 11, 16 }, {  8, 11, 16 }, {  8,  7,  5 }, {  8,  7,  5 },
 {  8,  4,  8 }, {  8,  4,  8 }, {  8,  6,  7 }, {  8,  6,  7 },
 {  8,  7,  6 }, {  8,  7,  6 }, {  8,  0,  5 }, {  8,  0,  5 },
 {  9,  8,  4 }, {  9, 16, 15 }, {  9, 12, 16 }, {  9,  1,  8 },
 {  9,  8,  1 }, {  9, 14, 16 }, {  9,  5,  8 }, {  9, 13, 16 },
 {  9,  3,  9 }, {  9,  8,  5 }, {  9,  7,  7 }, {  9,  2,  9 },
 {  9,  8,  6 }, {  9,  9,  2 }, {  9,  9,  3 }, {  9, 15, 16 },
 {  9,  4,  9 }, {  9,  6,  8 }, {  9,  6,  0 }, {  9,  9,  4 },
 {  9,  5,  9 }, {  9,  8,  7 }, {  9,  7,  8 }, {  9,  1,  9 },
 {  9, 10,  3 }, {  9,  0,  6 }, {  9, 10,  2 }, {  9,  9,  1 },
 {  9,  9,  5 }, {  9,  4, 10 }, {  9,  2, 10 }, {  9,  9,  6 },
 {  9,  3, 10 }, {  9,  6,  9 }, {  9, 10,  4 }, {  9,  8,  8 },
 {  9, 10,  5 }, {  9,  9,  7 }, {  9, 11,  3 }, {  9,  1, 10 },
 {  9,  7,  0 }, {  9, 10,  6 }, {  9,  7,  9 }, {  9,  3, 11 },
 {  9,  5, 10 }, {  9, 10,  1 }, {  9,  4, 11 }, {  9, 11,  2 },
 {  9, 13,  2 }, {  9,  6, 10 }, {  9, 13,  3 }, {  9, 13,  3 },
 {  9,  2, 11 }, {  9,  2, 11 }, {  9, 16,  0 }, {  9, 16,  0 },
 {  9,  5, 11 }, {  9,  5, 11 }, {  9, 11,  5 }, {  9, 11,  5 },
 { 10, 11,  4 }, { 10,  9,  8 }, { 10,  7, 10 }, { 10,  8,  9 },
 { 10,  0, 16 }, { 10,  4, 13 }, { 10,  0,  7 }, { 10,  3, 13 },
 { 10, 11,  6 }, { 10, 13,  1 }, { 10, 13,  4 }, { 10, 12,  3 },
 { 10,  2, 13 }, { 10, 13,  5 }, { 10,  8, 10 }, { 10,  6, 11 },
 { 10, 10,  8 }, { 10, 10,  7 }, { 10, 14,  2 }, { 10, 12,  4 },
 { 10,  1, 11 }, { 10,  4, 12 }, { 10, 11,  1 }, { 10,  3, 12 },
 { 10,  1, 13 }, { 10, 12,  2 }, { 10,  7, 11 }, { 10,  3, 14 },
 { 10,  5, 12 }, { 10,  5, 13 }, { 10, 14,  4 }, { 10,  4, 14 },
 { 10, 11,  7 }, { 10, 14,  3 }, { 10, 12,  5 }, { 10, 13,  6 },
 { 10, 12,  6 }, { 10,  8,  0 }, { 10, 11,  8 }, { 10,  2, 12 },
 { 10,  9,  9 }, { 10, 14,  5 }, { 10,  6, 13 }, { 10, 10, 10 },
 { 10, 15,  2 }, { 10,  8, 11 }, { 10,  9, 10 }, { 10, 14,  6 },
 { 10, 10,  9 }, { 10,  5, 14 }, { 10, 11,  9 }, { 10, 14,  1 },
 { 10,  2, 14 }, { 10,  6, 12 }, { 10,  1, 12 }, { 10, 13,  8 },
 { 10,  0,  8 }, { 10, 13,  7 }, { 10,  7, 12 }, { 10, 12,  7 },
 { 10,  7, 13 }, { 10, 15,  3 }, { 10, 12,  1 }, { 10,  6, 14 },
 { 10,  2, 15 }, { 10, 15,  5 }, { 10, 15,  4 }, { 10,  1, 14 },
 { 10,  9, 11 }, { 10,  4, 15 }, { 10, 14,  7 }, { 10,  8, 13 },
 { 10, 13,  9 }, { 10,  8, 12 }, { 10,  5, 15 }, { 10,  3, 15 },
 { 10, 10, 11 }, { 10, 11, 10 }, { 10, 12,  8 }, { 10, 15,  6 },
 { 10, 15,  7 }, { 10,  8, 14 }, { 10, 15,  1 }, { 10,  7, 14 },
 { 10,  9,  0 }, { 10,  0,  9 }, { 10,  9, 13 }, { 10,  9, 13 },
 { 10,  9, 13 }, { 10,  9, 13 }, { 10,  9, 12 }, { 10,  9, 12 },
 { 10,  9, 12 }, { 10,  9, 12 }, { 10, 12,  9 }, { 10, 12,  9 },
 { 10, 12,  9 }, { 10, 12,  9 }, { 10, 14,  8 }, { 10, 14,  8 },
 { 10, 14,  8 }, { 10, 14,  8 }, { 10, 10, 13 }, { 10, 10, 13 },
 { 10, 10, 13 }, { 10, 10, 13 }, { 10, 14,  9 }, { 10, 14,  9 },
 { 10, 14,  9 }, { 10, 14,  9 }, { 10, 12, 10 }, { 10, 12, 10 },
 { 10, 12, 10 }, { 10, 12, 10 }, { 10,  6, 15 }, { 10,  6, 15 },
 { 10,  6, 15 }, { 10,  6, 15 }, { 10,  7, 15 }, { 10,  7, 15 },
 { 10,  7, 15 }, { 10,  7, 15 }, { 11,  9, 14 }, { 11,  9, 14 },
 { 11, 15,  8 }, { 11, 15,  8 }, { 11, 11, 11 }, { 11, 11, 11 },
 { 11, 11, 14 }, { 11, 11, 14 }, { 11,  1, 15 }, { 11,  1, 15 },
 { 11, 10, 12 }, { 11, 10, 12 }, { 11, 10, 14 }, { 11, 10, 14 },
 { 11, 13, 11 }, { 11, 13, 11 }, { 11, 13, 10 }, { 11, 13, 10 },
 { 11, 11, 13 }, { 11, 11, 13 }, { 11, 11, 12 }, { 11, 11, 12 },
 { 11,  8, 15 }, { 11,  8, 15 }, { 11, 14, 11 }, { 11, 14, 11 },
 { 11, 13, 12 }, { 11, 13, 12 }, { 11, 12, 13 }, { 11, 12, 13 },
 { 11, 15,  9 }, { 11, 15,  9 }, { 11, 14, 10 }, { 11, 14, 10 },
 { 11, 10,  0 }, { 11, 10,  0 }, { 11, 12, 11 }, { 11, 12, 11 },
 { 11,  9, 15 }, { 11,  9, 15 }, { 11,  0, 10 }, { 11,  0, 10 },
 { 11, 12, 12 }, { 11, 12, 12 }, { 11, 11,  0 }, { 11, 11,  0 },
 { 11, 12, 14 }, { 11, 12, 14 }, { 11, 10, 15 }, { 11, 10, 15 },
 { 11, 13, 13 }, { 11, 13, 13 }, { 11,  0, 13 }, { 11,  0, 13 },
 { 11, 14, 12 }, { 11, 14, 12 }, { 11, 15, 10 }, { 11, 15, 10 },
 { 11, 15, 11 }, { 11, 15, 11 }, { 11, 11, 15 }, { 11, 11, 15 },
 { 11, 14, 13 }, { 11, 14, 13 }, { 11, 13,  0 }, { 11, 13,  0 },
 { 11,  0, 11 }, { 11,  0, 11 }, { 11, 13, 14 }, { 11, 13, 14 },
 { 11, 15, 12 }, { 11, 15, 12 }, { 11, 15, 13 }, { 11, 15, 13 },
 { 11, 12, 15 }, { 11, 12, 15 }, { 11, 14,  0 }, { 11, 14,  0 },
 { 11, 14, 14 }, { 11, 14, 14 }, { 11, 13, 15 }, { 11, 13, 15 },
 { 11, 12,  0 }, { 11, 12,  0 }, { 11, 14, 15 }, { 11, 14, 15 },
 { 12,  0, 14 }, { 12,  0, 12 }, { 12, 15, 14 }, { 12, 15,  0 },
 { 12,  0, 15 }, { 12, 15, 15 }
};

static hcb_2_pair const* hcb_2_pair_table[] __attribute__(   (  section( ".data" )  )   ) = {
 0, 0, 0, 0, 0, 0, hcb6_2, 0, hcb8_2, 0, hcb10_2, hcb11_2
};

static const short hcb_2_pair_table_size[] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 0, 0, 0, 0, 0, 126, 0, 83, 0, 210, 373
};

static const uint8_t Parity[ 256 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0
};

static const float tns_coef_0_3[] __attribute__(   (  section( ".rodata" )  )   ) = {
 ( float ) 0.0000000000, ( float ) 0.4338837391, ( float ) 0.7818314825, ( float ) 0.9749279122,
 ( float )-0.9848077530, ( float )-0.8660254038, ( float )-0.6427876097, ( float )-0.3420201433,
 ( float )-0.4338837391, ( float )-0.7818314825, ( float )-0.9749279122, ( float )-0.9749279122,
 ( float )-0.9848077530, ( float )-0.8660254038, ( float )-0.6427876097, ( float )-0.3420201433
};

static const float tns_coef_0_4[] __attribute__(   (  section( ".rodata" )  )   ) = {
 ( float ) 0.0000000000, ( float ) 0.2079116908, ( float ) 0.4067366431, ( float ) 0.5877852523,
 ( float ) 0.7431448255, ( float ) 0.8660254038, ( float ) 0.9510565163, ( float ) 0.9945218954,
 ( float )-0.9957341763, ( float )-0.9618256432, ( float )-0.8951632914, ( float )-0.7980172273,
 ( float )-0.6736956436, ( float )-0.5264321629, ( float )-0.3612416662, ( float )-0.1837495178
};

static const float tns_coef_1_3[] __attribute__(   (  section( ".rodata" )  )   ) = {
 ( float ) 0.0000000000, ( float ) 0.4338837391, ( float )-0.6427876097, ( float )-0.3420201433,
 ( float ) 0.9749279122, ( float ) 0.7818314825, ( float )-0.6427876097, ( float )-0.3420201433,
 ( float )-0.4338837391, ( float )-0.7818314825, ( float )-0.6427876097, ( float )-0.3420201433,
 ( float )-0.7818314825, ( float )-0.4338837391, ( float )-0.6427876097, ( float )-0.3420201433
};

static const float tns_coef_1_4[] __attribute__(   (  section( ".rodata" )  )   ) = {
 ( float ) 0.0000000000, ( float ) 0.2079116908, ( float ) 0.4067366431, ( float ) 0.5877852523,
 ( float )-0.6736956436, ( float )-0.5264321629, ( float )-0.3612416662, ( float )-0.1837495178,
 ( float ) 0.9945218954, ( float ) 0.9510565163, ( float ) 0.8660254038, ( float ) 0.7431448255,
 ( float )-0.6736956436, ( float )-0.5264321629, ( float )-0.3612416662, ( float )-0.1837495178
};

static const int8_t t_huffman_env_bal_3_0dB[ 24 ][ 2 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { -64,   1 }, { -63,   2 }, { -65,   3 }, { -66,   4 },
 { -62,   5 }, { -61,   6 }, { -67,   7 }, { -68,   8 },
 { -60,   9 }, {  10,  16 }, {  11,  13 }, { -69,  12 },
 { -76, -75 }, {  14,  15 }, { -74, -73 }, { -72, -71 },
 {  17,  20 }, {  18,  19 }, { -70, -59 }, { -58, -57 },
 {  21,  22 }, { -56, -55 }, { -54,  23 }, { -53, -52 }
};

static const int8_t f_huffman_env_bal_3_0dB[ 24 ][ 2 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { -64,   1 }, { -65,   2 }, { -63,   3 }, { -66,   4 },
 { -62,   5 }, { -61,   6 }, { -67,   7 }, { -68,   8 },
 { -60,   9 }, {  10,  13 }, { -69,  11 }, { -59,  12 },
 { -58, -76 }, {  14,  17 }, {  15,  16 }, { -75, -74 },
 { -73, -72 }, {  18,  21 }, {  19,  20 }, { -71, -70 },
 { -57, -56 }, {  22,  23 }, { -55, -54 }, { -53, -52 }
};

static const int8_t t_huffman_env_bal_1_5dB[ 48 ][ 2 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { -64,   1 }, { -63,   2 }, { -65,   3 }, { -62,   4 },
 { -66,   5 }, { -61,   6 }, { -67,   7 }, { -60,   8 },
 { -68,   9 }, {  10,  11 }, { -69, -59 }, {  12,  13 },
 { -70, -58 }, {  14,  28 }, {  15,  21 }, {  16,  18 },
 { -57,  17 }, { -71, -56 }, {  19,  20 }, { -88, -87 },
 { -86, -85 }, {  22,  25 }, {  23,  24 }, { -84, -83 },
 { -82, -81 }, {  26,  27 }, { -80, -79 }, { -78, -77 },
 {  29,  36 }, {  30,  33 }, {  31,  32 }, { -76, -75 },
 { -74, -73 }, {  34,  35 }, { -72, -55 }, { -54, -53 },
 {  37,  41 }, {  38,  39 }, { -52, -51 }, { -50,  40 },
 { -49, -48 }, {  42,  45 }, {  43,  44 }, { -47, -46 },
 { -45, -44 }, {  46,  47 }, { -43, -42 }, { -41, -40 }
};

static const int8_t f_huffman_env_bal_1_5dB[ 48 ][ 2 ] __attribute__(   (  section( ".rodata" )  )   ) = {
  { -64,   1 }, { -65,   2 }, { -63,   3 }, { -66,   4 },
  { -62,   5 }, { -61,   6 }, { -67,   7 }, { -68,   8 },
  { -60,   9 }, {  10,  11 }, { -69, -59 }, { -70,  12 },
  { -58,  13 }, {  14,  17 }, { -71,  15 }, { -57,  16 },
  { -56, -73 }, {  18,  32 }, {  19,  25 }, {  20,  22 },
  { -72,  21 }, { -88, -87 }, {  23,  24 }, { -86, -85 },
  { -84, -83 }, {  26,  29 }, {  27,  28 }, { -82, -81 },
  { -80, -79 }, {  30,  31 }, { -78, -77 }, { -76, -75 },
  {  33,  40 }, {  34,  37 }, {  35,  36 }, { -74, -55 },
  { -54, -53 }, {  38,  39 }, { -52, -51 }, { -50, -49 },
  {  41,  44 }, {  42,  43 }, { -48, -47 }, { -46, -45 },
  {  45,  46 }, { -44, -43 }, { -42,  47 }, { -41, -40 }
};

static const int8_t t_huffman_env_3_0dB[ 62 ][ 2 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { -64,   1 }, { -65,   2 }, { -63,   3 }, { -66,   4 },
 { -62,   5 }, { -67,   6 }, { -61,   7 }, { -68,   8 },
 { -60,   9 }, {  10,  11 }, { -69, -59 }, {  12,  14 },
 { -70,  13 }, { -71, -58 }, {  15,  18 }, {  16,  17 },
 { -72, -57 }, { -73, -74 }, {  19,  22 }, { -56,  20 },
 { -55,  21 }, { -54, -77 }, {  23,  31 }, {  24,  25 },
 { -75, -76 }, {  26,  27 }, { -78, -53 }, {  28,  29 },
 { -52, -95 }, { -94,  30 }, { -93, -92 }, {  32,  47 },
 {  33,  40 }, {  34,  37 }, {  35,  36 }, { -91, -90 },
 { -89, -88 }, {  38,  39 }, { -87, -86 }, { -85, -84 },
 {  41,  44 }, {  42,  43 }, { -83, -82 }, { -81, -80 },
 {  45,  46 }, { -79, -51 }, { -50, -49 }, {  48,  55 },
 {  49,  52 }, {  50,  51 }, { -48, -47 }, { -46, -45 },
 {  53,  54 }, { -44, -43 }, { -42, -41 }, {  56,  59 },
 {  57,  58 }, { -40, -39 }, { -38, -37 }, {  60,  61 },
 { -36, -35 }, { -34, -33 }
};

static const int8_t f_huffman_env_3_0dB[ 62 ][ 2 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { -64,   1 }, { -65,   2 }, { -63,   3 }, { -66,   4 },
 { -62,   5 }, { -67,   6 }, {   7,   8 }, { -61, -68 },
 {   9,  10 }, { -60, -69 }, {  11,  12 }, { -59, -70 },
 {  13,  14 }, { -58, -71 }, {  15,  16 }, { -57, -72 },
 {  17,  19 }, { -56,  18 }, { -55, -73 }, {  20,  24 },
 {  21,  22 }, { -74, -54 }, { -53,  23 }, { -75, -76 },
 {  25,  30 }, {  26,  27 }, { -52, -51 }, {  28,  29 },
 { -77, -79 }, { -50, -49 }, {  31,  39 }, {  32,  35 },
 {  33,  34 }, { -78, -46 }, { -82, -88 }, {  36,  37 },
 { -83, -48 }, { -47,  38 }, { -86, -85 }, {  40,  47 },
 {  41,  44 }, {  42,  43 }, { -80, -44 }, { -43, -42 },
 {  45,  46 }, { -39, -87 }, { -84, -40 }, {  48,  55 },
 {  49,  52 }, {  50,  51 }, { -95, -94 }, { -93, -92 },
 {  53,  54 }, { -91, -90 }, { -89, -81 }, {  56,  59 },
 {  57,  58 }, { -45, -41 }, { -38, -37 }, {  60,  61 },
 { -36, -35 }, { -34, -33 }
};

static const int8_t t_huffman_env_1_5dB[ 120 ][ 2 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 {    1,   2 }, {  -64, -65 }, {    3,   4 }, {  -63, -66 },
 {    5,   6 }, {  -62, -67 }, {    7,   8 }, {  -61, -68 },
 {    9,  10 }, {  -60, -69 }, {   11,  12 }, {  -59, -70 },
 {   13,  14 }, {  -58, -71 }, {   15,  16 }, {  -57, -72 },
 {   17,  18 }, {  -73, -56 }, {   19,  21 }, {  -74,  20 },
 {  -55, -75 }, {   22,  26 }, {   23,  24 }, {  -54, -76 },
 {  -77,  25 }, {  -53, -78 }, {   27,  34 }, {   28,  29 },
 {  -52, -79 }, {   30,  31 }, {  -80, -51 }, {   32,  33 },
 {  -83, -82 }, {  -81, -50 }, {   35,  57 }, {   36,  40 },
 {   37,  38 }, {  -88, -84 }, {  -48,  39 }, {  -90, -85 },
 {   41,  46 }, {   42,  43 }, {  -49, -87 }, {   44,  45 },
 {  -89, -86 }, { -124,-123 }, {   47,  50 }, {   48,  49 },
 { -122,-121 }, { -120,-119 }, {   51,  54 }, {   52,  53 },
 { -118,-117 }, { -116,-115 }, {   55,  56 }, { -114,-113 },
 { -112,-111 }, {   58,  89 }, {   59,  74 }, {   60,  67 },
 {   61,  64 }, {   62,  63 }, { -110,-109 }, { -108,-107 },
 {   65,  66 }, { -106,-105 }, { -104,-103 }, {   68,  71 },
 {   69,  70 }, { -102,-101 }, { -100, -99 }, {   72,  73 },
 {  -98, -97 }, {  -96, -95 }, {   75,  82 }, {   76,  79 },
 {   77,  78 }, {  -94, -93 }, {  -92, -91 }, {   80,  81 },
 {  -47, -46 }, {  -45, -44 }, {   83,  86 }, {   84,  85 },
 {  -43, -42 }, {  -41, -40 }, {   87,  88 }, {  -39, -38 },
 {  -37, -36 }, {   90, 105 }, {   91,  98 }, {   92,  95 },
 {   93,  94 }, {  -35, -34 }, {  -33, -32 }, {   96,  97 },
 {  -31, -30 }, {  -29, -28 }, {   99, 102 }, {  100, 101 },
 {  -27, -26 }, {  -25, -24 }, {  103, 104 }, {  -23, -22 },
 {  -21, -20 }, {  106, 113 }, {  107, 110 }, {  108, 109 },
 {  -19, -18 }, {  -17, -16 }, {  111, 112 }, {  -15, -14 },
 {  -13, -12 }, {  114, 117 }, {  115, 116 }, {  -11, -10 },
 {   -9,  -8 }, {  118, 119 }, {   -7,  -6 }, {   -5,  -4 }
};

static const int8_t f_huffman_env_1_5dB[ 120 ][ 2 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 {    1,   2 }, {  -64, -65 }, {    3,   4 }, {  -63, -66 },
 {    5,   6 }, {  -67, -62 }, {    7,   8 }, {  -68, -61 },
 {    9,  10 }, {  -69, -60 }, {   11,  13 }, {  -70,  12 },
 {  -59, -71 }, {   14,  16 }, {  -58,  15 }, {  -72, -57 },
 {   17,  19 }, {  -73,  18 }, {  -56, -74 }, {   20,  23 },
 {   21,  22 }, {  -55, -75 }, {  -54, -53 }, {   24,  27 },
 {   25,  26 }, {  -76, -52 }, {  -77, -51 }, {   28,  31 },
 {   29,  30 }, {  -50, -78 }, {  -79, -49 }, {   32,  36 },
 {   33,  34 }, {  -48, -47 }, {  -80,  35 }, {  -81, -82 },
 {   37,  47 }, {   38,  41 }, {   39,  40 }, {  -83, -46 },
 {  -45, -84 }, {   42,  44 }, {  -85,  43 }, {  -44, -43 },
 {   45,  46 }, {  -88, -87 }, {  -86, -90 }, {   48,  66 },
 {   49,  56 }, {   50,  53 }, {   51,  52 }, {  -92, -42 },
 {  -41, -39 }, {   54,  55 }, { -105, -89 }, {  -38, -37 },
 {   57,  60 }, {   58,  59 }, {  -94, -91 }, {  -40, -36 },
 {   61,  63 }, {  -20,  62 }, { -115,-110 }, {   64,  65 },
 { -108,-107 }, { -101, -97 }, {   67,  89 }, {   68,  75 },
 {   69,  72 }, {   70,  71 }, {  -95, -93 }, {  -34, -27 },
 {   73,  74 }, {  -22, -17 }, {  -16,-124 }, {   76,  82 },
 {   77,  79 }, { -123,  78 }, { -122,-121 }, {   80,  81 },
 { -120,-119 }, { -118,-117 }, {   83,  86 }, {   84,  85 },
 { -116,-114 }, { -113,-112 }, {   87,  88 }, { -111,-109 },
 { -106,-104 }, {   90, 105 }, {   91,  98 }, {   92,  95 },
 {   93,  94 }, { -103,-102 }, { -100, -99 }, {   96,  97 },
 {  -98, -96 }, {  -35, -33 }, {   99, 102 }, {  100, 101 },
 {  -32, -31 }, {  -30, -29 }, {  103, 104 }, {  -28, -26 },
 {  -25, -24 }, {  106, 113 }, {  107, 110 }, {  108, 109 },
 {  -23, -21 }, {  -19, -18 }, {  111, 112 }, {  -15, -14 },
 {  -13, -12 }, {  114, 117 }, {  115, 116 }, {  -11, -10 },
 {   -9,  -8 }, {  118, 119 }, {   -7,  -6 }, {   -5,  -4 }
};

static const int8_t t_huffman_noise_bal_3_0dB[ 24 ][ 2 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { -64,   1 }, { -65,   2 }, { -63,   3 }, {   4,   9 },
 { -66,   5 }, { -62,   6 }, {   7,   8 }, { -76, -75 },
 { -74, -73 }, {  10,  17 }, {  11,  14 }, {  12,  13 },
 { -72, -71 }, { -70, -69 }, {  15,  16 }, { -68, -67 },
 { -61, -60 }, {  18,  21 }, {  19,  20 }, { -59, -58 },
 { -57, -56 }, {  22,  23 }, { -55, -54 }, { -53, -52 }
};

static const int8_t t_huffman_noise_3_0dB[ 62 ][ 2 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { -64,   1 }, { -63,   2 }, { -65,   3 }, { -66,   4 },
 { -62,   5 }, { -67,   6 }, {   7,   8 }, { -61, -68 },
 {   9,  30 }, {  10,  15 }, { -60,  11 }, { -69,  12 },
 {  13,  14 }, { -59, -53 }, { -95, -94 }, {  16,  23 },
 {  17,  20 }, {  18,  19 }, { -93, -92 }, { -91, -90 },
 {  21,  22 }, { -89, -88 }, { -87, -86 }, {  24,  27 },
 {  25,  26 }, { -85, -84 }, { -83, -82 }, {  28,  29 },
 { -81, -80 }, { -79, -78 }, {  31,  46 }, {  32,  39 },
 {  33,  36 }, {  34,  35 }, { -77, -76 }, { -75, -74 },
 {  37,  38 }, { -73, -72 }, { -71, -70 }, {  40,  43 },
 {  41,  42 }, { -58, -57 }, { -56, -55 }, {  44,  45 },
 { -54, -52 }, { -51, -50 }, {  47,  54 }, {  48,  51 },
 {  49,  50 }, { -49, -48 }, { -47, -46 }, {  52,  53 },
 { -45, -44 }, { -43, -42 }, {  55,  58 }, {  56,  57 },
 { -41, -40 }, { -39, -38 }, {  59,  60 }, { -37, -36 },
 { -35,  61 }, { -34, -33 }
};

static const float E_deq_tab[ 64 ] __attribute__(   (  section( ".rodata" )  )   ) = {
         64.0F,        128.0F,        256.0F,        512.0F,       1024.0F,       2048.0F,       4096.0F,       8192.0F,
      16384.0F,      32768.0F,      65536.0F,     131072.0F,     262144.0F,     524288.0F, 1.04858E+006F, 2.09715E+006F,
  4.1943E+006F, 8.38861E+006F, 1.67772E+007F, 3.35544E+007F, 6.71089E+007F, 1.34218E+008F, 2.68435E+008F, 5.36871E+008F,
 1.07374E+009F, 2.14748E+009F, 4.29497E+009F, 8.58993E+009F, 1.71799E+010F, 3.43597E+010F, 6.87195E+010F, 1.37439E+011F,
 2.74878E+011F, 5.49756E+011F, 1.09951E+012F, 2.19902E+012F, 4.39805E+012F, 8.79609E+012F, 1.75922E+013F, 3.51844E+013F,
 7.03687E+013F, 1.40737E+014F, 2.81475E+014F,  5.6295E+014F,  1.1259E+015F,  2.2518E+015F,  4.5036E+015F,  9.0072E+015F,
 1.80144E+016F, 3.60288E+016F, 7.20576E+016F, 1.44115E+017F,  2.8823E+017F, 5.76461E+017F, 1.15292E+018F, 2.30584E+018F,
 4.61169E+018F, 9.22337E+018F, 1.84467E+019F, 3.68935E+019F,  7.3787E+019F, 1.47574E+020F, 2.95148E+020F, 5.90296E+020F
};

static const float Q_div_tab_left[ 31 ][ 13 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { 0.969704F, 0.888985F, 0.667532F, 0.336788F, 0.117241F, 0.0375940F, 0.0153846F, 0.00967118F, 0.00823245F, 0.00787211F, 0.00778198F, 0.00775945F, 0.00775382F },
 { 0.984619F, 0.941230F, 0.800623F, 0.503876F, 0.209877F, 0.0724638F, 0.0303030F, 0.01915710F, 0.01633050F, 0.01562120F, 0.01544380F, 0.01539940F, 0.01538830F },
 { 0.992250F, 0.969726F, 0.889273F, 0.670103F, 0.346939F, 0.1351350F, 0.0588235F, 0.03759400F, 0.03213610F, 0.03076190F, 0.03041780F, 0.03033170F, 0.03031020F },
 { 0.996110F, 0.984630F, 0.941392F, 0.802469F, 0.515152F, 0.2380950F, 0.1111110F, 0.07246380F, 0.06227110F, 0.05968780F, 0.05903970F, 0.05887760F, 0.05883700F },
 { 0.998051F, 0.992256F, 0.969811F, 0.890411F, 0.680000F, 0.3846150F, 0.2000000F, 0.13513500F, 0.11724100F, 0.11265200F, 0.11149700F, 0.11120800F, 0.11113500F },
 { 0.999025F, 0.996113F, 0.984674F, 0.942029F, 0.809524F, 0.5555560F, 0.3333330F, 0.23809500F, 0.20987700F, 0.20249200F, 0.20062500F, 0.20015600F, 0.20003900F },
 { 0.999512F, 0.998053F, 0.992278F, 0.970149F, 0.894737F, 0.7142860F, 0.5000000F, 0.38461500F, 0.34693900F, 0.33678800F, 0.33420000F, 0.33355000F, 0.33338800F },
 { 0.999756F, 0.999025F, 0.996124F, 0.984848F, 0.944444F, 0.8333330F, 0.6666670F, 0.55555600F, 0.51515200F, 0.50387600F, 0.50097500F, 0.50024400F, 0.50006100F },
 { 0.999878F, 0.999512F, 0.998058F, 0.992366F, 0.971429F, 0.9090910F, 0.8000000F, 0.71428600F, 0.68000000F, 0.67010300F, 0.66753200F, 0.66688400F, 0.66672100F },
 { 0.999939F, 0.999756F, 0.999028F, 0.996169F, 0.985507F, 0.9523810F, 0.8888890F, 0.83333300F, 0.80952400F, 0.80246900F, 0.80062300F, 0.80015600F, 0.80003900F },
 { 0.999969F, 0.999878F, 0.999514F, 0.998081F, 0.992701F, 0.9756100F, 0.9411760F, 0.90909100F, 0.89473700F, 0.89041100F, 0.88927300F, 0.88898500F, 0.88891300F },
 { 0.999985F, 0.999939F, 0.999757F, 0.999039F, 0.996337F, 0.9876540F, 0.9696970F, 0.95238100F, 0.94444400F, 0.94202900F, 0.94139200F, 0.94123000F, 0.94119000F },
 { 0.999992F, 0.999970F, 0.999878F, 0.999519F, 0.998165F, 0.9937890F, 0.9846150F, 0.97561000F, 0.97142900F, 0.97014900F, 0.96981100F, 0.96972600F, 0.96970400F },
 { 0.999996F, 0.999985F, 0.999939F, 0.999760F, 0.999082F, 0.9968850F, 0.9922480F, 0.98765400F, 0.98550700F, 0.98484800F, 0.98467400F, 0.98463000F, 0.98461900F },
 { 0.999998F, 0.999992F, 0.999970F, 0.999880F, 0.999541F, 0.9984400F, 0.9961090F, 0.99378900F, 0.99270100F, 0.99236600F, 0.99227800F, 0.99225600F, 0.99225000F },
 { 0.999999F, 0.999996F, 0.999985F, 0.999940F, 0.999770F, 0.9992190F, 0.9980510F, 0.99688500F, 0.99633700F, 0.99616900F, 0.99612400F, 0.99611300F, 0.99611000F },
 { 1.000000F, 0.999998F, 0.999992F, 0.999970F, 0.999885F, 0.9996100F, 0.9990240F, 0.99844000F, 0.99816500F, 0.99808100F, 0.99805800F, 0.99805300F, 0.99805100F },
 { 1.000000F, 0.999999F, 0.999996F, 0.999985F, 0.999943F, 0.9998050F, 0.9995120F, 0.99921900F, 0.99908200F, 0.99903900F, 0.99902800F, 0.99902500F, 0.99902500F },
 { 1.000000F, 1.000000F, 0.999998F, 0.999992F, 0.999971F, 0.9999020F, 0.9997560F, 0.99961000F, 0.99954100F, 0.99951900F, 0.99951400F, 0.99951200F, 0.99951200F },
 { 1.000000F, 1.000000F, 0.999999F, 0.999996F, 0.999986F, 0.9999510F, 0.9998780F, 0.99980500F, 0.99977000F, 0.99976000F, 0.99975700F, 0.99975600F, 0.99975600F },
 { 1.000000F, 1.000000F, 1.000000F, 0.999998F, 0.999993F, 0.9999760F, 0.9999390F, 0.99990200F, 0.99988500F, 0.99988000F, 0.99987800F, 0.99987800F, 0.99987800F },
 { 1.000000F, 1.000000F, 1.000000F, 0.999999F, 0.999996F, 0.9999880F, 0.9999690F, 0.99995100F, 0.99994300F, 0.99994000F, 0.99993900F, 0.99993900F, 0.99993900F },
 { 1.000000F, 1.000000F, 1.000000F, 1.000000F, 0.999998F, 0.9999940F, 0.9999850F, 0.99997600F, 0.99997100F, 0.99997000F, 0.99997000F, 0.99997000F, 0.99996900F },
 { 1.000000F, 1.000000F, 1.000000F, 1.000000F, 0.999999F, 0.9999970F, 0.9999920F, 0.99998800F, 0.99998600F, 0.99998500F, 0.99998500F, 0.99998500F, 0.99998500F },
 { 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 0.9999980F, 0.9999960F, 0.99999400F, 0.99999300F, 0.99999200F, 0.99999200F, 0.99999200F, 0.99999200F },
 { 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 0.9999990F, 0.9999980F, 0.99999700F, 0.99999600F, 0.99999600F, 0.99999600F, 0.99999600F, 0.99999600F },
 { 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.0000000F, 0.9999990F, 0.99999800F, 0.99999800F, 0.99999800F, 0.99999800F, 0.99999800F, 0.99999800F },
 { 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.0000000F, 1.0000000F, 0.99999900F, 0.99999900F, 0.99999900F, 0.99999900F, 0.99999900F, 0.99999900F },
 { 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.0000000F, 1.0000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F },
 { 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.0000000F, 1.0000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F },
 { 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.0000000F, 1.0000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F }
};

static const float Q_div_tab_right[ 31 ][ 13 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { 0.00775382F, 0.00775945F, 0.00778198F, 0.00787211F, 0.00823245F, 0.00967118F, 0.0153846F, 0.0375940F, 0.117241F, 0.336788F, 0.667532F, 0.888985F, 0.969704F },
 { 0.01538830F, 0.01539940F, 0.01544380F, 0.01562120F, 0.01633050F, 0.01915710F, 0.0303030F, 0.0724638F, 0.209877F, 0.503876F, 0.800623F, 0.941230F, 0.984619F },
 { 0.03031020F, 0.03033170F, 0.03041780F, 0.03076190F, 0.03213610F, 0.03759400F, 0.0588235F, 0.1351350F, 0.346939F, 0.670103F, 0.889273F, 0.969726F, 0.992250F },
 { 0.05883700F, 0.05887760F, 0.05903970F, 0.05968780F, 0.06227110F, 0.07246380F, 0.1111110F, 0.2380950F, 0.515152F, 0.802469F, 0.941392F, 0.984630F, 0.996110F },
 { 0.11113500F, 0.11120800F, 0.11149700F, 0.11265200F, 0.11724100F, 0.13513500F, 0.2000000F, 0.3846150F, 0.680000F, 0.890411F, 0.969811F, 0.992256F, 0.998051F },
 { 0.20003900F, 0.20015600F, 0.20062500F, 0.20249200F, 0.20987700F, 0.23809500F, 0.3333330F, 0.5555560F, 0.809524F, 0.942029F, 0.984674F, 0.996113F, 0.999025F },
 { 0.33338800F, 0.33355000F, 0.33420000F, 0.33678800F, 0.34693900F, 0.38461500F, 0.5000000F, 0.7142860F, 0.894737F, 0.970149F, 0.992278F, 0.998053F, 0.999512F },
 { 0.50006100F, 0.50024400F, 0.50097500F, 0.50387600F, 0.51515200F, 0.55555600F, 0.6666670F, 0.8333330F, 0.944444F, 0.984848F, 0.996124F, 0.999025F, 0.999756F },
 { 0.66672100F, 0.66688400F, 0.66753200F, 0.67010300F, 0.68000000F, 0.71428600F, 0.8000000F, 0.9090910F, 0.971429F, 0.992366F, 0.998058F, 0.999512F, 0.999878F },
 { 0.80003900F, 0.80015600F, 0.80062300F, 0.80246900F, 0.80952400F, 0.83333300F, 0.8888890F, 0.9523810F, 0.985507F, 0.996169F, 0.999028F, 0.999756F, 0.999939F },
 { 0.88891300F, 0.88898500F, 0.88927300F, 0.89041100F, 0.89473700F, 0.90909100F, 0.9411760F, 0.9756100F, 0.992701F, 0.998081F, 0.999514F, 0.999878F, 0.999969F },
 { 0.94119000F, 0.94123000F, 0.94139200F, 0.94202900F, 0.94444400F, 0.95238100F, 0.9696970F, 0.9876540F, 0.996337F, 0.999039F, 0.999757F, 0.999939F, 0.999985F },
 { 0.96970400F, 0.96972600F, 0.96981100F, 0.97014900F, 0.97142900F, 0.97561000F, 0.9846150F, 0.9937890F, 0.998165F, 0.999519F, 0.999878F, 0.999970F, 0.999992F },
 { 0.98461900F, 0.98463000F, 0.98467400F, 0.98484800F, 0.98550700F, 0.98765400F, 0.9922480F, 0.9968850F, 0.999082F, 0.999760F, 0.999939F, 0.999985F, 0.999996F },
 { 0.99225000F, 0.99225600F, 0.99227800F, 0.99236600F, 0.99270100F, 0.99378900F, 0.9961090F, 0.9984400F, 0.999541F, 0.999880F, 0.999970F, 0.999992F, 0.999998F },
 { 0.99611000F, 0.99611300F, 0.99612400F, 0.99616900F, 0.99633700F, 0.99688500F, 0.9980510F, 0.9992190F, 0.999770F, 0.999940F, 0.999985F, 0.999996F, 0.999999F },
 { 0.99805100F, 0.99805300F, 0.99805800F, 0.99808100F, 0.99816500F, 0.99844000F, 0.9990240F, 0.9996100F, 0.999885F, 0.999970F, 0.999992F, 0.999998F, 1.000000F },
 { 0.99902500F, 0.99902500F, 0.99902800F, 0.99903900F, 0.99908200F, 0.99921900F, 0.9995120F, 0.9998050F, 0.999943F, 0.999985F, 0.999996F, 0.999999F, 1.000000F },
 { 0.99951200F, 0.99951200F, 0.99951400F, 0.99951900F, 0.99954100F, 0.99961000F, 0.9997560F, 0.9999020F, 0.999971F, 0.999992F, 0.999998F, 1.000000F, 1.000000F },
 { 0.99975600F, 0.99975600F, 0.99975700F, 0.99976000F, 0.99977000F, 0.99980500F, 0.9998780F, 0.9999510F, 0.999986F, 0.999996F, 0.999999F, 1.000000F, 1.000000F },
 { 0.99987800F, 0.99987800F, 0.99987800F, 0.99988000F, 0.99988500F, 0.99990200F, 0.9999390F, 0.9999760F, 0.999993F, 0.999998F, 1.000000F, 1.000000F, 1.000000F },
 { 0.99993900F, 0.99993900F, 0.99993900F, 0.99994000F, 0.99994300F, 0.99995100F, 0.9999690F, 0.9999880F, 0.999996F, 0.999999F, 1.000000F, 1.000000F, 1.000000F },
 { 0.99996900F, 0.99997000F, 0.99997000F, 0.99997000F, 0.99997100F, 0.99997600F, 0.9999850F, 0.9999940F, 0.999998F, 1.000000F, 1.000000F, 1.000000F, 1.000000F },
 { 0.99998500F, 0.99998500F, 0.99998500F, 0.99998500F, 0.99998600F, 0.99998800F, 0.9999920F, 0.9999970F, 0.999999F, 1.000000F, 1.000000F, 1.000000F, 1.000000F },
 { 0.99999200F, 0.99999200F, 0.99999200F, 0.99999200F, 0.99999300F, 0.99999400F, 0.9999960F, 0.9999980F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F },
 { 0.99999600F, 0.99999600F, 0.99999600F, 0.99999600F, 0.99999600F, 0.99999700F, 0.9999980F, 0.9999990F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F },
 { 0.99999800F, 0.99999800F, 0.99999800F, 0.99999800F, 0.99999800F, 0.99999800F, 0.9999990F, 1.0000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F },
 { 0.99999900F, 0.99999900F, 0.99999900F, 0.99999900F, 0.99999900F, 0.99999900F, 1.0000000F, 1.0000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F },
 { 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.0000000F, 1.0000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F },
 { 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.0000000F, 1.0000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F },
 { 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.00000000F, 1.0000000F, 1.0000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F }
};

static const float Q_div_tab[ 31 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 0.0153846F, 0.030303F, 0.0588235F, 0.111111F, 0.2000000F, 0.333333F,
 0.5000000F, 0.666667F, 0.8000000F, 0.888889F, 0.9411760F, 0.969697F,
 0.9846150F, 0.992248F, 0.9961090F, 0.998051F, 0.9990240F, 0.999512F,
 0.9997560F, 0.999878F, 0.9999390F, 0.999969F, 0.9999850F, 0.999992F,
 0.9999960F, 0.999998F, 0.9999990F, 1.000000F, 1.0000000F, 1.000000F,
 1.0000000F
};

static const float Q_div2_tab_left[ 31 ][ 13 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { 0.0302959000F, 0.1110150000F, 0.3324680000F, 0.6632120000F, 0.8827590000F, 0.9624060000F, 0.9846150000F, 0.9903290000F, 0.9917680000F, 0.9921280000F, 0.9922180000F, 0.9922410000F, 0.9922460000F },
 { 0.0153809000F, 0.0587695000F, 0.1993770000F, 0.4961240000F, 0.7901230000F, 0.9275360000F, 0.9696970000F, 0.9808430000F, 0.9836700000F, 0.9843790000F, 0.9845560000F, 0.9846010000F, 0.9846120000F },
 { 0.0077500600F, 0.0302744000F, 0.1107270000F, 0.3298970000F, 0.6530610000F, 0.8648650000F, 0.9411760000F, 0.9624060000F, 0.9678640000F, 0.9692380000F, 0.9695820000F, 0.9696680000F, 0.9696900000F },
 { 0.0038901000F, 0.0153698000F, 0.0586081000F, 0.1975310000F, 0.4848480000F, 0.7619050000F, 0.8888890000F, 0.9275360000F, 0.9377290000F, 0.9403120000F, 0.9409600000F, 0.9411220000F, 0.9411630000F },
 { 0.0019488400F, 0.0077444300F, 0.0301887000F, 0.1095890000F, 0.3200000000F, 0.6153850000F, 0.8000000000F, 0.8648650000F, 0.8827590000F, 0.8873480000F, 0.8885030000F, 0.8887920000F, 0.8888650000F },
 { 0.0009753720F, 0.0038872700F, 0.0153257000F, 0.0579710000F, 0.1904760000F, 0.4444440000F, 0.6666670000F, 0.7619050000F, 0.7901230000F, 0.7975080000F, 0.7993750000F, 0.7998440000F, 0.7999610000F },
 { 0.0004879240F, 0.0019474200F, 0.0077220100F, 0.0298507000F, 0.1052630000F, 0.2857140000F, 0.5000000000F, 0.6153850000F, 0.6530610000F, 0.6632120000F, 0.6658000000F, 0.6664500000F, 0.6666120000F },
 { 0.0002440210F, 0.0009746590F, 0.0038759700F, 0.0151515000F, 0.0555556000F, 0.1666670000F, 0.3333330000F, 0.4444440000F, 0.4848480000F, 0.4961240000F, 0.4990250000F, 0.4997560000F, 0.4999390000F },
 { 0.0001220260F, 0.0004875670F, 0.0019417500F, 0.0076335900F, 0.0285714000F, 0.0909091000F, 0.2000000000F, 0.2857140000F, 0.3200000000F, 0.3298970000F, 0.3324680000F, 0.3331160000F, 0.3332790000F },
 { 6.10165E-005F, 0.0002438430F, 0.0009718170F, 0.0038314200F, 0.0144928000F, 0.0476190000F, 0.1111110000F, 0.1666670000F, 0.1904760000F, 0.1975310000F, 0.1993770000F, 0.1998440000F, 0.1999610000F },
 { 3.05092E-005F, 0.0001219360F, 0.0004861450F, 0.0019193900F, 0.0072992700F, 0.0243902000F, 0.0588235000F, 0.0909091000F, 0.1052630000F, 0.1095890000F, 0.1107270000F, 0.1110150000F, 0.1110870000F },
 { 1.52548E-005F, 6.09719E-005F, 0.0002431320F, 0.0009606150F, 0.0036630000F, 0.0123457000F, 0.0303030000F, 0.0476190000F, 0.0555556000F, 0.0579710000F, 0.0586081000F, 0.0587695000F, 0.0588100000F },
 { 7.62747E-006F, 3.04869E-005F, 0.0001215810F, 0.0004805380F, 0.0018348600F, 0.0062111800F, 0.0153846000F, 0.0243902000F, 0.0285714000F, 0.0298507000F, 0.0301887000F, 0.0302744000F, 0.0302959000F },
 { 3.81375E-006F, 1.52437E-005F, 6.07940E-005F, 0.0002403270F, 0.0009182740F, 0.0031152600F, 0.0077519400F, 0.0123457000F, 0.0144928000F, 0.0151515000F, 0.0153257000F, 0.0153698000F, 0.0153809000F },
 { 1.90688E-006F, 7.62189E-006F, 3.03979E-005F, 0.0001201780F, 0.0004593480F, 0.0015600600F, 0.0038910500F, 0.0062111800F, 0.0072992700F, 0.0076335900F, 0.0077220100F, 0.0077444300F, 0.0077500600F },
 { 9.53441E-007F, 3.81096E-006F, 1.51992E-005F, 6.00925E-005F, 0.0002297270F, 0.0007806400F, 0.0019493200F, 0.0031152600F, 0.0036630000F, 0.0038314200F, 0.0038759700F, 0.0038872700F, 0.0038901000F },
 { 4.76721E-007F, 1.90548E-006F, 7.59965E-006F, 3.00472E-005F, 0.0001148770F, 0.0003904720F, 0.0009756100F, 0.0015600600F, 0.0018348600F, 0.0019193900F, 0.0019417500F, 0.0019474200F, 0.0019488400F },
 { 2.38360E-007F, 9.52743E-007F, 3.79984E-006F, 1.50238E-005F, 5.74416E-005F, 0.0001952740F, 0.0004880430F, 0.0007806400F, 0.0009182740F, 0.0009606150F, 0.0009718170F, 0.0009746590F, 0.0009753720F },
 { 1.19180E-007F, 4.76372E-007F, 1.89992E-006F, 7.51196E-006F, 2.87216E-005F, 9.76467E-005F, 0.0002440810F, 0.0003904720F, 0.0004593480F, 0.0004805380F, 0.0004861450F, 0.0004875670F, 0.0004879240F },
 { 5.95901E-008F, 2.38186E-007F, 9.49963E-007F, 3.75600E-006F, 1.43610E-005F, 4.88257E-005F, 0.0001220550F, 0.0001952740F, 0.0002297270F, 0.0002403270F, 0.0002431320F, 0.0002438430F, 0.0002440210F },
 { 2.97950E-008F, 1.19093E-007F, 4.74982E-007F, 1.87800E-006F, 7.18056E-006F, 2.44135E-005F, 6.10314E-005F, 9.76467E-005F, 0.0001148770F, 0.0001201780F, 0.0001215810F, 0.0001219360F, 0.0001220260F },
 { 1.48975E-008F, 5.95465E-008F, 2.37491E-007F, 9.39002E-007F, 3.59029E-006F, 1.22069E-005F, 3.05166E-005F, 4.88257E-005F, 5.74416E-005F, 6.00925E-005F, 6.07940E-005F, 6.09719E-005F, 6.10165E-005F },
 { 7.44876E-009F, 2.97732E-008F, 1.18745E-007F, 4.69501E-007F, 1.79515E-006F, 6.10348E-006F, 1.52586E-005F, 2.44135E-005F, 2.87216E-005F, 3.00472E-005F, 3.03979E-005F, 3.04869E-005F, 3.05092E-005F },
 { 3.72438E-009F, 1.48866E-008F, 5.93727E-008F, 2.34751E-007F, 8.97575E-007F, 3.05175E-006F, 7.62934E-006F, 1.22069E-005F, 1.43610E-005F, 1.50238E-005F, 1.51992E-005F, 1.52437E-005F, 1.52548E-005F },
 { 1.86219E-009F, 7.44331E-009F, 2.96864E-008F, 1.17375E-007F, 4.48788E-007F, 1.52588E-006F, 3.81468E-006F, 6.10348E-006F, 7.18056E-006F, 7.51196E-006F, 7.59965E-006F, 7.62189E-006F, 7.62747E-006F },
 { 9.31095E-010F, 3.72166E-009F, 1.48432E-008F, 5.86876E-008F, 2.24394E-007F, 7.62939E-007F, 1.90734E-006F, 3.05175E-006F, 3.59029E-006F, 3.75600E-006F, 3.79984E-006F, 3.81096E-006F, 3.81375E-006F },
 { 4.65548E-010F, 1.86083E-009F, 7.42159E-009F, 2.93438E-008F, 1.12197E-007F, 3.81470E-007F, 9.53673E-007F, 1.52588E-006F, 1.79515E-006F, 1.87800E-006F, 1.89992E-006F, 1.90548E-006F, 1.90688E-006F },
 { 2.32774E-010F, 9.30414E-010F, 3.71079E-009F, 1.46719E-008F, 5.60985E-008F, 1.90735E-007F, 4.76837E-007F, 7.62939E-007F, 8.97575E-007F, 9.39002E-007F, 9.49963E-007F, 9.52743E-007F, 9.53441E-007F },
 { 1.16387E-010F, 4.65207E-010F, 1.85540E-009F, 7.33596E-009F, 2.80492E-008F, 9.53674E-008F, 2.38419E-007F, 3.81470E-007F, 4.48788E-007F, 4.69501E-007F, 4.74982E-007F, 4.76372E-007F, 4.76721E-007F },
 { 5.81935E-011F, 2.32603E-010F, 9.27699E-010F, 3.66798E-009F, 1.40246E-008F, 4.76837E-008F, 1.19209E-007F, 1.90735E-007F, 2.24394E-007F, 2.34751E-007F, 2.37491E-007F, 2.38186E-007F, 2.38360E-007F },
 { 2.90967E-011F, 1.16302E-010F, 4.63849E-010F, 1.83399E-009F, 7.01231E-009F, 2.38419E-008F, 5.96046E-008F, 9.53674E-008F, 1.12197E-007F, 1.17375E-007F, 1.18745E-007F, 1.19093E-007F, 1.19180E-007F }
};

static const float Q_div2_tab_right[ 31 ][ 13 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 { 0.9922460000F, 0.9922410000F, 0.9922180000F, 0.9921280000F, 0.9917680000F, 0.9903290000F, 0.9846150000F, 0.9624060000F, 0.8827590000F, 0.6632120000F, 0.3324680000F, 0.1110150000F, 0.0302959000F },
 { 0.9846120000F, 0.9846010000F, 0.9845560000F, 0.9843790000F, 0.9836700000F, 0.9808430000F, 0.9696970000F, 0.9275360000F, 0.7901230000F, 0.4961240000F, 0.1993770000F, 0.0587695000F, 0.0153809000F },
 { 0.9696900000F, 0.9696680000F, 0.9695820000F, 0.9692380000F, 0.9678640000F, 0.9624060000F, 0.9411760000F, 0.8648650000F, 0.6530610000F, 0.3298970000F, 0.1107270000F, 0.0302744000F, 0.0077500600F },
 { 0.9411630000F, 0.9411220000F, 0.9409600000F, 0.9403120000F, 0.9377290000F, 0.9275360000F, 0.8888890000F, 0.7619050000F, 0.4848480000F, 0.1975310000F, 0.0586081000F, 0.0153698000F, 0.0038901000F },
 { 0.8888650000F, 0.8887920000F, 0.8885030000F, 0.8873480000F, 0.8827590000F, 0.8648650000F, 0.8000000000F, 0.6153850000F, 0.3200000000F, 0.1095890000F, 0.0301887000F, 0.0077444300F, 0.0019488400F },
 { 0.7999610000F, 0.7998440000F, 0.7993750000F, 0.7975080000F, 0.7901230000F, 0.7619050000F, 0.6666670000F, 0.4444440000F, 0.1904760000F, 0.0579710000F, 0.0153257000F, 0.0038872700F, 0.0009753720F },
 { 0.6666120000F, 0.6664500000F, 0.6658000000F, 0.6632120000F, 0.6530610000F, 0.6153850000F, 0.5000000000F, 0.2857140000F, 0.1052630000F, 0.0298507000F, 0.0077220100F, 0.0019474200F, 0.0004879240F },
 { 0.4999390000F, 0.4997560000F, 0.4990250000F, 0.4961240000F, 0.4848480000F, 0.4444440000F, 0.3333330000F, 0.1666670000F, 0.0555556000F, 0.0151515000F, 0.0038759700F, 0.0009746590F, 0.0002440210F },
 { 0.3332790000F, 0.3331160000F, 0.3324680000F, 0.3298970000F, 0.3200000000F, 0.2857140000F, 0.2000000000F, 0.0909091000F, 0.0285714000F, 0.0076335900F, 0.0019417500F, 0.0004875670F, 0.0001220260F },
 { 0.1999610000F, 0.1998440000F, 0.1993770000F, 0.1975310000F, 0.1904760000F, 0.1666670000F, 0.1111110000F, 0.0476190000F, 0.0144928000F, 0.0038314200F, 0.0009718170F, 0.0002438430F, 6.10165E-005F },
 { 0.1110870000F, 0.1110150000F, 0.1107270000F, 0.1095890000F, 0.1052630000F, 0.0909091000F, 0.0588235000F, 0.0243902000F, 0.0072992700F, 0.0019193900F, 0.0004861450F, 0.0001219360F, 3.05092E-005F },
 { 0.0588100000F, 0.0587695000F, 0.0586081000F, 0.0579710000F, 0.0555556000F, 0.0476190000F, 0.0303030000F, 0.0123457000F, 0.0036630000F, 0.0009606150F, 0.0002431320F, 6.09719E-005F, 1.52548E-005F },
 { 0.0302959000F, 0.0302744000F, 0.0301887000F, 0.0298507000F, 0.0285714000F, 0.0243902000F, 0.0153846000F, 0.0062111800F, 0.0018348600F, 0.0004805380F, 0.0001215810F, 3.04869E-005F, 7.62747E-006F },
 { 0.0153809000F, 0.0153698000F, 0.0153257000F, 0.0151515000F, 0.0144928000F, 0.0123457000F, 0.0077519400F, 0.0031152600F, 0.0009182740F, 0.0002403270F, 6.0794E-0050F, 1.52437E-005F, 3.81375E-006F },
 { 0.0077500600F, 0.0077444300F, 0.0077220100F, 0.0076335900F, 0.0072992700F, 0.0062111800F, 0.0038910500F, 0.0015600600F, 0.0004593480F, 0.0001201780F, 3.03979E-005F, 7.62189E-006F, 1.90688E-006F },
 { 0.0038901000F, 0.0038872700F, 0.0038759700F, 0.0038314200F, 0.0036630000F, 0.0031152600F, 0.0019493200F, 0.0007806400F, 0.0002297270F, 6.00925E-005F, 1.51992E-005F, 3.81096E-006F, 9.53441E-007F },
 { 0.0019488400F, 0.0019474200F, 0.0019417500F, 0.0019193900F, 0.0018348600F, 0.0015600600F, 0.0009756100F, 0.0003904720F, 0.0001148770F, 3.00472E-005F, 7.59965E-006F, 1.90548E-006F, 4.76721E-007F },
 { 0.0009753720F, 0.0009746590F, 0.0009718170F, 0.0009606150F, 0.0009182740F, 0.0007806400F, 0.0004880430F, 0.0001952740F, 5.74416E-005F, 1.50238E-005F, 3.79984E-006F, 9.52743E-007F, 2.38360E-007F },
 { 0.0004879240F, 0.0004875670F, 0.0004861450F, 0.0004805380F, 0.0004593480F, 0.0003904720F, 0.0002440810F, 9.76467E-005F, 2.87216E-005F, 7.51196E-006F, 1.89992E-006F, 4.76372E-007F, 1.19180E-007F },
 { 0.0002440210F, 0.0002438430F, 0.0002431320F, 0.0002403270F, 0.0002297270F, 0.0001952740F, 0.0001220550F, 4.88257E-005F, 1.43610E-005F, 3.75600E-006F, 9.49963E-007F, 2.38186E-007F, 5.95901E-008F },
 { 0.0001220260F, 0.0001219360F, 0.0001215810F, 0.0001201780F, 0.0001148770F, 9.76467E-005F, 6.10314E-005F, 2.44135E-005F, 7.18056E-006F, 1.87800E-006F, 4.74982E-007F, 1.19093E-007F, 2.97950E-008F },
 { 6.10165E-005F, 6.09719E-005F, 6.07940E-005F, 6.00925E-005F, 5.74416E-005F, 4.88257E-005F, 3.05166E-005F, 1.22069E-005F, 3.59029E-006F, 9.39002E-007F, 2.37491E-007F, 5.95465E-008F, 1.48975E-008F },
 { 3.05092E-005F, 3.04869E-005F, 3.03979E-005F, 3.00472E-005F, 2.87216E-005F, 2.44135E-005F, 1.52586E-005F, 6.10348E-006F, 1.79515E-006F, 4.69501E-007F, 1.18745E-007F, 2.97732E-008F, 7.44876E-009F },
 { 1.52548E-005F, 1.52437E-005F, 1.51992E-005F, 1.50238E-005F, 1.43610E-005F, 1.22069E-005F, 7.62934E-006F, 3.05175E-006F, 8.97575E-007F, 2.34751E-007F, 5.93727E-008F, 1.48866E-008F, 3.72438E-009F },
 { 7.62747E-006F, 7.62189E-006F, 7.59965E-006F, 7.51196E-006F, 7.18056E-006F, 6.10348E-006F, 3.81468E-006F, 1.52588E-006F, 4.48788E-007F, 1.17375E-007F, 2.96864E-008F, 7.44331E-009F, 1.86219E-009F },
 { 3.81375E-006F, 3.81096E-006F, 3.79984E-006F, 3.75600E-006F, 3.59029E-006F, 3.05175E-006F, 1.90734E-006F, 7.62939E-007F, 2.24394E-007F, 5.86876E-008F, 1.48432E-008F, 3.72166E-009F, 9.31095E-010F },
 { 1.90688E-006F, 1.90548E-006F, 1.89992E-006F, 1.87800E-006F, 1.79515E-006F, 1.52588E-006F, 9.53673E-007F, 3.81470E-007F, 1.12197E-007F, 2.93438E-008F, 7.42159E-009F, 1.86083E-009F, 4.65548E-010F },
 { 9.53441E-007F, 9.52743E-007F, 9.49963E-007F, 9.39002E-007F, 8.97575E-007F, 7.62939E-007F, 4.76837E-007F, 1.90735E-007F, 5.60985E-008F, 1.46719E-008F, 3.71079E-009F, 9.30414E-010F, 2.32774E-010F },
 { 4.76721E-007F, 4.76372E-007F, 4.74982E-007F, 4.69501E-007F, 4.48788E-007F, 3.81470E-007F, 2.38419E-007F, 9.53674E-008F, 2.80492E-008F, 7.33596E-009F, 1.85540E-009F, 4.65207E-010F, 1.16387E-010F },
 { 2.38360E-007F, 2.38186E-007F, 2.37491E-007F, 2.34751E-007F, 2.24394E-007F, 1.90735E-007F, 1.19209E-007F, 4.76837E-008F, 1.40246E-008F, 3.66798E-009F, 9.27699E-010F, 2.32603E-010F, 5.81935E-011F },
 { 1.19180E-007F, 1.19093E-007F, 1.18745E-007F, 1.17375E-007F, 1.12197E-007F, 9.53674E-008F, 5.96046E-008F, 2.38419E-008F, 7.01231E-009F, 1.83399E-009F, 4.63849E-010F, 1.16302E-010F, 2.90967E-011F }
};

static const float Q_div2_tab[ 31 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 0.9846150000F, 0.9696970000F, 0.9411760000F, 0.8888890000F, 0.8000000000F, 0.6666670000F,
 0.5000000000F, 0.3333330000F, 0.2000000000F, 0.1111110000F, 0.0588235000F, 0.0303030000F,
 0.0153846000F, 0.0077519400F, 0.0038910500F, 0.0019493200F, 0.0009756100F, 0.0004880430F,
 0.0002440810F, 0.0001220550F, 6.10314E-005F, 3.05166E-005F, 1.52586E-005F, 7.62934E-006F,
 3.81468E-006F, 1.90734E-006F, 9.53673E-007F, 4.76837E-007F, 2.38419E-007F, 1.19209E-007F,
 5.96046E-008F
};

static const float E_pan_tab[ 25 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 0.000244081F, 0.000488043F, 0.000975610F, 0.001949320F, 0.003891050F, 0.007751940F,
 0.015384600F, 0.030303000F, 0.058823500F, 0.111111000F, 0.200000000F, 0.333333000F,
 0.500000000F, 0.666667000F, 0.800000000F, 0.888889000F, 0.941176000F, 0.969697000F,
 0.984615000F, 0.992248000F, 0.996109000F, 0.998051000F, 0.999024000F, 0.999512000F,
 0.999756000F
};

static const complex_t V[] __attribute__(   (  section( ".rodata" )  )   ) = {
 { -0.99948155879974F, -0.59483414888382F }, {  0.97113454341888F, -0.67528516054153F },
 {  0.14130051434040F, -0.95090985298157F }, { -0.47005495429039F, -0.37340548634529F },
 {  0.80705064535141F,  0.29653668403625F }, { -0.38981479406357F,  0.89572608470917F },
 { -0.01053049881011F, -0.66959059238434F }, { -0.91266369819641F, -0.11522938311100F },
 {  0.54840421676636F,  0.75221365690231F }, {  0.40009254217148F, -0.98929399251938F },
 { -0.99867975711823F, -0.88147068023682F }, { -0.95531076192856F,  0.90908759832382F },
 { -0.45725932717323F, -0.56716322898865F }, { -0.72929674386978F, -0.98008275032043F },
 {  0.75622802972794F,  0.20950329303741F }, {  0.07069442421198F, -0.78247898817062F },
 {  0.74496251344681F, -0.91169005632401F }, { -0.96440184116364F, -0.94739919900894F },
 {  0.30424630641937F, -0.49438267946243F }, {  0.66565030813217F,  0.64652937650681F },
 {  0.91697007417679F,  0.17514097690582F }, { -0.70774918794632F,  0.52548652887344F },
 { -0.70051413774490F, -0.45340028405190F }, { -0.99496513605118F, -0.90071910619736F },
 {  0.98164492845535F, -0.77463155984879F }, { -0.54671579599380F, -0.02570928446949F },
 { -0.01689629070461F,  0.00287506449968F }, { -0.86110347509384F,  0.42548584938049F },
 { -0.98892980813980F, -0.87881129980087F }, {  0.51756626367569F,  0.66926783323288F },
 { -0.99635028839111F, -0.58107727766037F }, { -0.99969369173050F,  0.98369991779327F },
 {  0.55266261100769F,  0.59449058771133F }, {  0.34581178426743F,  0.94879418611526F },
 {  0.62664210796356F, -0.74402970075607F }, { -0.77149701118469F, -0.33883658051491F },
 { -0.91592246294022F,  0.03687901422381F }, { -0.76285493373871F, -0.91371870040894F },
 {  0.79788339138031F, -0.93180972337723F }, {  0.54473078250885F, -0.11919206380844F },
 { -0.85639280080795F,  0.42429855465889F }, { -0.92882400751114F,  0.27871808409691F },
 { -0.11708371341228F, -0.99800843000412F }, {  0.21356749534607F, -0.90716296434402F },
 { -0.76191693544388F,  0.99768120050430F }, {  0.98111045360565F, -0.95854461193085F },
 { -0.85913270711899F,  0.95766568183899F }, { -0.93307244777679F,  0.49431759119034F },
 {  0.30485755205154F, -0.70540034770966F }, {  0.85289651155472F,  0.46766132116318F },
 {  0.91328084468842F, -0.99839597940445F }, { -0.05890199914575F,  0.70741826295853F },
 {  0.28398686647415F,  0.34633556008339F }, {  0.95258164405823F, -0.54893416166306F },
 { -0.78566324710846F, -0.75568538904190F }, { -0.95789498090744F, -0.20423194766045F },
 {  0.82411158084869F,  0.96654617786407F }, { -0.65185445547104F, -0.88734990358353F },
 { -0.93643605709076F,  0.99870789051056F }, {  0.91427159309387F, -0.98290503025055F },
 { -0.70395684242249F,  0.58796799182892F }, {  0.00563771976158F,  0.61768198013306F },
 {  0.89065051078796F,  0.52783352136612F }, { -0.68683707714081F,  0.80806946754456F },
 {  0.72165340185165F, -0.69259858131409F }, { -0.62928247451782F,  0.13627037405968F },
 {  0.29938435554504F, -0.46051329374313F }, { -0.91781955957413F, -0.74012714624405F },
 {  0.99298715591431F,  0.40816611051559F }, {  0.82368296384811F, -0.74036049842834F },
 { -0.98512834310532F, -0.99972331523895F }, { -0.95915371179581F, -0.99237799644470F },
 { -0.21411126852036F, -0.93424820899963F }, { -0.68821477890015F, -0.26892307400703F },
 {  0.91851997375488F,  0.09358228743076F }, { -0.96062767505646F,  0.36099094152451F },
 {  0.51646184921265F, -0.71373331546783F }, {  0.61130720376968F,  0.46950140595436F },
 {  0.47336128354073F, -0.27333179116249F }, {  0.90998309850693F,  0.96715664863586F },
 {  0.44844800233841F,  0.99211573600769F }, {  0.66614890098572F,  0.96590173244476F },
 {  0.74922239780426F, -0.89879858493805F }, { -0.99571585655212F,  0.52785521745682F },
 {  0.97401082515717F, -0.16855870187283F }, {  0.72683745622635F, -0.48060774803162F },
 {  0.95432192087173F,  0.68849605321884F }, { -0.72962206602097F, -0.76608443260193F },
 { -0.85359477996826F,  0.88738125562668F }, { -0.81412428617477F, -0.97480767965317F },
 { -0.87930774688721F,  0.74748307466507F }, { -0.71573328971863F, -0.98570609092712F },
 {  0.83524298667908F,  0.83702534437180F }, { -0.48086065053940F, -0.98848503828049F },
 {  0.97139126062393F,  0.80093622207642F }, {  0.51992827653885F,  0.80247628688812F },
 { -0.00848591234535F, -0.76670128107071F }, { -0.70294374227524F,  0.55359911918640F },
 { -0.95894426107407F, -0.43265503644943F }, {  0.97079253196716F,  0.09325857460499F },
 { -0.92404294013977F,  0.85507702827454F }, { -0.69506472349167F,  0.98633414506912F },
 {  0.26559203863144F,  0.73314309120178F }, {  0.28038442134857F,  0.14537914097309F },
 { -0.74138122797012F,  0.99310338497162F }, { -0.01752796024084F, -0.82616633176804F },
 { -0.55126774311066F, -0.98898541927338F }, {  0.97960901260376F, -0.94021445512772F },
 { -0.99196308851242F,  0.67019015550613F }, { -0.67684930562973F,  0.12631492316723F },
 {  0.09140039235353F, -0.20537731051445F }, { -0.71658962965012F, -0.97788202762604F },
 {  0.81014639139175F,  0.53722649812698F }, {  0.40616992115974F, -0.26469007134438F },
 { -0.67680186033249F,  0.94502049684525F }, {  0.86849772930145F, -0.18333598971367F },
 { -0.99500381946564F, -0.02634122036397F }, {  0.84329187870026F,  0.10406957566738F },
 { -0.09215968847275F,  0.69540011882782F }, {  0.99956172704697F, -0.12358541786671F },
 { -0.79732781648636F, -0.91582524776459F }, {  0.96349972486496F,  0.96640455722809F },
 { -0.79942780733109F,  0.64323902130127F }, { -0.11566039919853F,  0.28587844967842F },
 { -0.39922955632210F,  0.94129604101181F }, {  0.99089199304581F, -0.92062628269196F },
 {  0.28631284832954F, -0.91035044193268F }, { -0.83302724361420F, -0.67330408096313F },
 {  0.95404446125031F,  0.49162766337395F }, { -0.06449863314629F,  0.03250560909510F },
 { -0.99575054645538F,  0.42389783263206F }, { -0.65501141548157F,  0.82546114921570F },
 { -0.81254440546036F, -0.51627236604691F }, { -0.99646371603012F,  0.84490531682968F },
 {  0.00287840608507F,  0.64768260717392F }, {  0.70176988840103F, -0.20453028380871F },
 {  0.96361881494522F,  0.40706968307495F }, { -0.68883758783340F,  0.91338956356049F },
 { -0.34875586628914F,  0.71472293138504F }, {  0.91980081796646F,  0.66507452726364F },
 { -0.99009048938751F,  0.85868018865585F }, {  0.68865793943405F,  0.55660319328308F },
 { -0.99484401941299F, -0.20052559673786F }, {  0.94214510917664F, -0.99696427583694F },
 { -0.67414629459381F,  0.49548220634460F }, { -0.47339352965355F, -0.85904330015182F },
 {  0.14323651790619F, -0.94145596027374F }, { -0.29268294572830F,  0.05759225040674F },
 {  0.43793860077858F, -0.78904968500137F }, { -0.36345127224922F,  0.64874434471130F },
 { -0.08750604838133F,  0.97686946392059F }, { -0.96495270729065F, -0.53960305452347F },
 {  0.55526942014694F,  0.78891521692276F }, {  0.73538213968277F,  0.96452075242996F },
 { -0.30889773368835F, -0.80664390325546F }, {  0.03574995696545F, -0.97325617074966F },
 {  0.98720687627792F,  0.48409134149551F }, { -0.81689298152924F, -0.90827703475952F },
 {  0.67866861820221F,  0.81284505128860F }, { -0.15808570384979F,  0.85279554128647F },
 {  0.80723392963409F, -0.24717418849468F }, {  0.47788757085800F, -0.46333149075508F },
 {  0.96367555856705F,  0.38486748933792F }, { -0.99143874645233F, -0.24945276975632F },
 {  0.83081877231598F, -0.94780850410461F }, { -0.58753192424774F,  0.01290772389621F },
 {  0.95538109540939F, -0.85557049512863F }, { -0.96490919589996F, -0.64020973443985F },
 { -0.97327101230621F,  0.12378127872944F }, {  0.91400367021561F,  0.57972472906113F },
 { -0.99925839900970F,  0.71084845066071F }, { -0.86875903606415F, -0.20291699469090F },
 { -0.26240035891533F, -0.68264555931091F }, { -0.24664412438869F, -0.87642270326614F },
 {  0.02416275814176F,  0.27192914485931F }, {  0.82068622112274F, -0.85087788105011F },
 {  0.88547372817993F, -0.89636802673340F }, { -0.18173077702522F, -0.26152145862579F },
 {  0.09355476498604F,  0.54845124483109F }, { -0.54668414592743F,  0.95980775356293F },
 {  0.37050989270210F, -0.59910142421722F }, { -0.70373594760895F,  0.91227668523788F },
 { -0.34600785374641F, -0.99441426992416F }, { -0.68774479627609F, -0.30238837003708F },
 { -0.26843291521072F,  0.83115667104721F }, {  0.49072334170341F, -0.45359709858894F },
 {  0.38975992798805F,  0.95515358448029F }, { -0.97757124900818F,  0.05305894464254F },
 { -0.17325553297997F, -0.92770671844482F }, {  0.99948036670685F,  0.58285546302795F },
 { -0.64946246147156F,  0.68645507097244F }, { -0.12016920745373F, -0.57147324085236F },
 { -0.58947455883026F, -0.34847131371498F }, { -0.41815140843391F,  0.16276422142982F },
 {  0.99885648488998F,  0.11136095225811F }, { -0.56649613380432F, -0.90494865179062F },
 {  0.94138020277023F,  0.35281917452812F }, { -0.75725078582764F,  0.53650552034378F },
 {  0.20541973412037F, -0.94435143470764F }, {  0.99980372190475F,  0.79835915565491F },
 {  0.29078277945518F,  0.35393777489662F }, { -0.62858772277832F,  0.38765692710876F },
 {  0.43440905213356F, -0.98546332120895F }, { -0.98298585414886F,  0.21021524071693F },
 {  0.19513028860092F, -0.94239830970764F }, { -0.95476663112640F,  0.98364555835724F },
 {  0.93379634618759F, -0.70881992578506F }, { -0.85235410928726F, -0.08342348039150F },
 { -0.86425095796585F, -0.45795026421547F }, {  0.38879778981209F,  0.97274428606033F },
 {  0.92045122385025F, -0.62433654069901F }, {  0.89162534475327F,  0.54950958490372F },
 { -0.36834338307381F,  0.96458297967911F }, {  0.93891763687134F, -0.89968353509903F },
 {  0.99267655611038F, -0.03757034242153F }, { -0.94063472747803F,  0.41332337260246F },
 {  0.99740225076675F, -0.16830494999886F }, { -0.35899412631989F, -0.46633225679398F },
 {  0.05237237364054F, -0.25640362501144F }, {  0.36703583598137F, -0.38653266429901F },
 {  0.91653180122375F, -0.30587628483772F }, {  0.69000804424286F,  0.90952169895172F },
 { -0.38658750057220F,  0.99501574039459F }, { -0.29250815510750F,  0.37444993853569F },
 { -0.60182201862335F,  0.86779648065567F }, { -0.97418588399887F,  0.96468526124954F },
 {  0.88461571931839F,  0.57508403062820F }, {  0.05198933184147F,  0.21269661188126F },
 { -0.53499621152878F,  0.97241556644440F }, { -0.49429559707642F,  0.98183864355087F },
 { -0.98935145139694F, -0.40249159932137F }, { -0.98081380128860F, -0.72856897115707F },
 { -0.27338150143623F,  0.99950921535492F }, {  0.06310802698135F, -0.54539585113525F },
 { -0.20461677014828F, -0.14209978282452F }, {  0.66223841905594F,  0.72528582811356F },
 { -0.84764343500137F,  0.02372316829860F }, { -0.89039862155914F,  0.88866579532623F },
 {  0.95903307199478F,  0.76744925975800F }, {  0.73504126071930F, -0.03747203201056F },
 { -0.31744435429573F, -0.36834111809731F }, { -0.34110826253891F,  0.40211221575737F },
 {  0.47803884744644F, -0.39423218369484F }, {  0.98299193382263F,  0.01989791356027F },
 { -0.30963072180748F, -0.18076720833778F }, {  0.99992591142654F, -0.26281872391701F },
 { -0.93149733543396F, -0.98313164710999F }, {  0.99923473596573F, -0.80142992734909F },
 { -0.26024168729782F, -0.75999760627747F }, { -0.35712513327599F,  0.19298963248730F },
 { -0.99899083375931F,  0.74645155668259F }, {  0.86557173728943F,  0.55593866109848F },
 {  0.33408042788506F,  0.86185956001282F }, {  0.99010735750198F,  0.04602397605777F },
 { -0.66694271564484F, -0.91643613576889F }, {  0.64016789197922F,  0.15649530291557F },
 {  0.99570536613464F,  0.45844584703445F }, { -0.63431465625763F,  0.21079117059708F },
 { -0.07706847041845F, -0.89581435918808F }, {  0.98590087890625F,  0.88241720199585F },
 {  0.80099332332611F, -0.36851897835732F }, {  0.78368133306503F,  0.45506998896599F },
 {  0.08707806468010F,  0.80938994884491F }, { -0.86811882257462F,  0.39347308874130F },
 { -0.39466530084610F, -0.66809433698654F }, {  0.97875326871872F, -0.72467839717865F },
 { -0.95038563013077F,  0.89563220739365F }, {  0.17005239427090F,  0.54683053493500F },
 { -0.76910793781281F, -0.96226614713669F }, {  0.99743282794952F,  0.42697158455849F },
 {  0.95437383651733F,  0.97002321481705F }, {  0.99578905105591F, -0.54106825590134F },
 {  0.28058260679245F, -0.85361421108246F }, {  0.85256522893906F, -0.64567607641220F },
 { -0.50608539581299F, -0.65846014022827F }, { -0.97210735082626F, -0.23095212876797F },
 {  0.95424050092697F, -0.99240148067474F }, { -0.96926569938660F,  0.73775655031204F },
 {  0.30872163176537F,  0.41514959931374F }, { -0.24523839354515F,  0.63206630945206F },
 { -0.33813264966011F, -0.38661777973175F }, { -0.05826828256249F, -0.06940773874521F },
 { -0.22898460924625F,  0.97054851055145F }, { -0.18509915471077F,  0.47565764188766F },
 { -0.10488238185644F, -0.87769949436188F }, { -0.71886587142944F,  0.78030979633331F },
 {  0.99793875217438F,  0.90041309595108F }, {  0.57563304901123F, -0.91034334897995F },
 {  0.28909647464752F,  0.96307784318924F }, {  0.42188999056816F,  0.48148649930954F },
 {  0.93335050344467F, -0.43537023663521F }, { -0.97087377309799F,  0.86636447906494F },
 {  0.36722871661186F,  0.65291655063629F }, { -0.81093025207520F,  0.08778370171785F },
 { -0.26240602135658F, -0.92774093151093F }, {  0.83996498584747F,  0.55839848518372F },
 { -0.99909615516663F, -0.96024608612061F }, {  0.74649465084076F,  0.12144893407822F },
 { -0.74774593114853F, -0.26898062229156F }, {  0.95781666040421F, -0.79047924280167F },
 {  0.95472306013107F, -0.08588775992393F }, {  0.48708331584930F,  0.99999040365219F },
 {  0.46332037448883F,  0.10964126139879F }, { -0.76497006416321F,  0.89210927486420F },
 {  0.57397389411926F,  0.35289704799652F }, {  0.75374317169189F,  0.96705216169357F },
 { -0.59174400568008F, -0.89405369758606F }, {  0.75087904930115F, -0.29612672328949F },
 { -0.98607856035233F,  0.25034910440445F }, { -0.40761056542397F, -0.90045571327209F },
 {  0.66929268836975F,  0.98629492521286F }, { -0.97463697195053F, -0.00190223299433F },
 {  0.90145510435104F,  0.99781388044357F }, { -0.87259286642075F,  0.99233585596085F },
 { -0.91529458761215F, -0.15698707103729F }, { -0.03305738791823F, -0.37205263972282F },
 {  0.07223051041365F, -0.88805001974106F }, {  0.99498009681702F,  0.97094357013702F },
 { -0.74904936552048F,  0.99985486268997F }, {  0.04585228487849F,  0.99812334775925F },
 { -0.89054954051971F, -0.31791913509369F }, { -0.83782142400742F,  0.97637635469437F },
 {  0.33454805612564F, -0.86231517791748F }, { -0.99707579612732F,  0.93237990140915F },
 { -0.22827528417110F,  0.18874759972095F }, {  0.67248046398163F, -0.03646211326122F },
 { -0.05146538093686F, -0.92599701881409F }, {  0.99947297573090F,  0.93625229597092F },
 {  0.66951125860214F,  0.98905825614929F }, { -0.99602955579758F, -0.44654715061188F },
 {  0.82104903459549F,  0.99540740251541F }, {  0.99186509847641F,  0.72022998332977F },
 { -0.65284591913223F,  0.52186721563339F }, {  0.93885445594788F, -0.74895310401917F },
 {  0.96735250949860F,  0.90891814231873F }, { -0.22225968539715F,  0.57124030590057F },
 { -0.44132784008980F, -0.92688840627670F }, { -0.85694974660873F,  0.88844531774521F },
 {  0.91783040761948F, -0.46356892585754F }, {  0.72556972503662F, -0.99899554252625F },
 { -0.99711579084396F,  0.58211559057236F }, {  0.77638977766037F,  0.94321835041046F },
 {  0.07717324048281F,  0.58638399839401F }, { -0.56049829721451F,  0.82522302865982F },
 {  0.98398894071579F,  0.39467439055443F }, {  0.47546947002411F,  0.68613046407700F },
 {  0.65675091743469F,  0.18331636488438F }, {  0.03273375332355F, -0.74933111667633F },
 { -0.38684144616127F,  0.51337349414825F }, { -0.97346270084381F, -0.96549361944199F },
 { -0.53282153606415F, -0.91423267126083F }, {  0.99817311763763F,  0.61133575439453F },
 { -0.50254499912262F, -0.88829338550568F }, {  0.01995873264968F,  0.85223513841629F },
 {  0.99930381774902F,  0.94578897953033F }, {  0.82907766103745F, -0.06323442608118F },
 { -0.58660709857941F,  0.96840775012970F }, { -0.17573736608028F, -0.48166921734810F },
 {  0.83434289693832F, -0.13023450970650F }, {  0.05946491286159F,  0.20511047542095F },
 {  0.81505483388901F, -0.94685947895050F }, { -0.44976380467415F,  0.40894573926926F },
 { -0.89746475219727F,  0.99846577644348F }, {  0.39677256345749F, -0.74854665994644F },
 { -0.07588948309422F,  0.74096214771271F }, {  0.76343196630478F,  0.41746628284454F },
 { -0.74490106105804F,  0.94725912809372F }, {  0.64880120754242F,  0.41336661577225F },
 {  0.62319535017014F, -0.93098312616348F }, {  0.42215818166733F, -0.07712787389755F },
 {  0.02704554051161F, -0.05417517945170F }, {  0.80001771450043F,  0.91542196273804F },
 { -0.79351830482483F, -0.36208897829056F }, {  0.63872361183167F,  0.08128252625465F },
 {  0.52890521287918F,  0.60048872232437F }, {  0.74238550662994F,  0.04491915181279F },
 {  0.99096131324768F, -0.19451183080673F }, { -0.80412328243256F, -0.88513815402985F },
 { -0.64612615108490F,  0.72198677062988F }, {  0.11657770723104F, -0.83662831783295F },
 { -0.95053184032440F, -0.96939903497696F }, { -0.62228870391846F,  0.82767260074615F },
 {  0.03004475869238F, -0.99738895893097F }, { -0.97987216711044F,  0.36526128649712F },
 { -0.99986982345581F, -0.36021611094475F }, {  0.89110648632050F, -0.97894251346588F },
 {  0.10407960414886F,  0.77357792854309F }, {  0.95964735746384F, -0.35435819625854F },
 {  0.50843232870102F,  0.96107691526413F }, {  0.17006334662437F, -0.76854026317596F },
 {  0.25872674584389F,  0.99893301725388F }, { -0.01115998718888F,  0.98496019840240F },
 { -0.79598701000214F,  0.97138410806656F }, { -0.99264711141586F, -0.99542820453644F },
 { -0.99829661846161F,  0.01877138763666F }, { -0.70801013708115F,  0.33680686354637F },
 { -0.70467054843903F,  0.93272775411606F }, {  0.99846023321152F, -0.98725748062134F },
 { -0.63364970684052F, -0.16473594307899F }, { -0.16258217394352F, -0.95939123630524F },
 { -0.43645593523979F, -0.94805032014847F }, { -0.99848473072052F,  0.96245169639587F },
 { -0.16796459257603F, -0.98987513780594F }, { -0.87979227304459F, -0.71725726127625F },
 {  0.44183099269867F, -0.93568974733353F }, {  0.93310177326202F, -0.99913311004639F },
 { -0.93941932916641F, -0.56409376859665F }, { -0.88590002059937F,  0.47624599933624F },
 {  0.99971461296082F, -0.83889955282211F }, { -0.75376385450363F,  0.00814643409103F },
 {  0.93887686729431F, -0.11284527927637F }, {  0.85126435756683F,  0.52349251508713F },
 {  0.39701420068741F,  0.81779634952545F }, { -0.37024465203285F, -0.87071657180786F },
 { -0.36024826765060F,  0.34655734896660F }, { -0.93388813734055F, -0.84476542472839F },
 { -0.65298801660538F, -0.18439576029778F }, {  0.11960318684578F,  0.99899345636368F },
 {  0.94292563199997F,  0.83163905143738F }, {  0.75081145763397F, -0.35533222556114F },
 {  0.56721979379654F, -0.24076835811138F }, {  0.46857765316963F, -0.30140233039856F },
 {  0.97312313318253F, -0.99548190832138F }, { -0.38299977779388F,  0.98516911268234F },
 {  0.41025799512863F,  0.02116736955941F }, {  0.09638062119484F,  0.04411984235048F },
 { -0.85283249616623F,  0.91475564241409F }, {  0.88866806030273F, -0.99735265970230F },
 { -0.48202428221703F, -0.96805608272552F }, {  0.27572581171989F,  0.58634752035141F },
 { -0.65889132022858F,  0.58835631608963F }, {  0.98838084936142F,  0.99994349479675F },
 { -0.20651349425316F,  0.54593044519424F }, { -0.62126415967941F, -0.59893679618835F },
 {  0.20320105552673F, -0.86879181861877F }, { -0.97790551185608F,  0.96290808916092F },
 {  0.11112534999847F,  0.21484763920307F }, { -0.41368338465691F,  0.28216838836670F },
 {  0.24133038520813F,  0.51294362545013F }, { -0.66393411159515F, -0.08249679952860F },
 { -0.53697830438614F, -0.97649902105331F }, { -0.97224736213684F,  0.22081333398819F },
 {  0.87392479181290F, -0.12796173989773F }, {  0.19050361216068F,  0.01602615416050F },
 { -0.46353441476822F, -0.95249038934708F }, { -0.07064096629620F, -0.94479805231094F },
 { -0.92444086074829F, -0.10457590222359F }, { -0.83822596073151F, -0.01695043221116F },
 {  0.75214684009552F, -0.99955683946609F }, { -0.42102998495102F,  0.99720942974091F },
 { -0.72094786167145F, -0.35008960962296F }, {  0.78843313455582F,  0.52851396799088F },
 {  0.97394025325775F, -0.26695942878723F }, {  0.99206465482712F, -0.57010120153427F },
 {  0.76789611577988F, -0.76519358158112F }, { -0.82002419233322F, -0.73530179262161F },
 {  0.81924992799759F,  0.99698424339294F }, { -0.26719850301743F,  0.68903368711472F },
 { -0.43311259150505F,  0.85321813821793F }, {  0.99194979667664F,  0.91876250505447F },
 { -0.80691999197006F, -0.32627540826797F }, {  0.43080005049706F, -0.21919095516205F },
 {  0.67709493637085F, -0.95478075742722F }, {  0.56151771545410F, -0.70693808794022F },
 {  0.10831862688065F, -0.08628837019205F }, {  0.91229414939880F, -0.65987348556519F },
 { -0.48972892761230F,  0.56289243698120F }, { -0.89033657312393F, -0.71656566858292F },
 {  0.65269446372986F,  0.65916007757187F }, {  0.67439478635788F, -0.81684380769730F },
 { -0.47770830988884F, -0.16789555549622F }, { -0.99715977907181F, -0.93565785884857F },
 { -0.90889590978622F,  0.62034398317337F }, { -0.06618622690439F, -0.23812216520309F },
 {  0.99430269002914F,  0.18812555074692F }, {  0.97686403989792F, -0.28664535284042F },
 {  0.94813650846481F, -0.97506642341614F }, { -0.95434498786926F, -0.79607981443405F },
 { -0.49104782938957F,  0.32895213365555F }, {  0.99881172180176F,  0.88993984460831F },
 {  0.50449168682098F, -0.85995072126389F }, {  0.47162890434265F, -0.18680204451084F },
 { -0.62081581354141F,  0.75000673532486F }, { -0.43867015838623F,  0.99998068809509F },
 {  0.98630565404892F, -0.53578901290894F }, { -0.61510360240936F, -0.89515018463135F },
 { -0.03841517493129F, -0.69888818264008F }, { -0.30102157592773F, -0.07667808979750F },
 {  0.41881284117699F,  0.02188098989427F }, { -0.86135452985764F,  0.98947483301163F },
 {  0.67226862907410F, -0.13494388759136F }, { -0.70737397670746F, -0.76547348499298F },
 {  0.94044947624207F,  0.09026201069355F }, { -0.82386350631714F,  0.08924768865108F },
 { -0.32070666551590F,  0.50143420696259F }, {  0.57593160867691F, -0.98966425657272F },
 { -0.36326017975807F,  0.07440242916346F }, {  0.99979043006897F, -0.14130286872387F },
 { -0.92366021871567F, -0.97979295253754F }, { -0.44607177376747F, -0.54233253002167F },
 {  0.44226801395416F,  0.71326756477356F }, {  0.03671907261014F,  0.63606387376785F },
 {  0.52175426483154F, -0.85396826267242F }, { -0.94701141119003F, -0.01826348155737F },
 { -0.98759609460831F,  0.82288712263107F }, {  0.87434792518616F,  0.89399492740631F },
 { -0.93412041664124F,  0.41374051570892F }, {  0.96063941717148F,  0.93116706609726F },
 {  0.97534251213074F,  0.86150932312012F }, {  0.99642467498779F,  0.70190042257309F },
 { -0.94705086946487F, -0.29580041766167F }, {  0.91599804162979F, -0.98147833347321F }
};

static const float qmf_c[ 640 ] __attribute__(   (  section( ".rodata" )  )   ) = {
  0.00000000000000F, -0.00055252865047F, -0.00056176925738F, -0.00049475180896F,
 -0.00048752279712F, -0.00048937912498F, -0.00050407143497F, -0.00052265642972F,
 -0.00054665656337F, -0.00056778025613F, -0.00058709304852F, -0.00061327473938F,
 -0.00063124935319F, -0.00065403333621F, -0.00067776907764F, -0.00069416146273F,
 -0.00071577364744F, -0.00072550431222F, -0.00074409418541F, -0.00074905980532F,
 -0.00076813719270F, -0.00077248485949F, -0.00078343322877F, -0.00077798694927F,
 -0.00078036647100F, -0.00078014496257F, -0.00077579773310F, -0.00076307935757F,
 -0.00075300014201F, -0.00073193571525F, -0.00072153919876F, -0.00069179375372F,
 -0.00066504150893F, -0.00063415949025F, -0.00059461189330F, -0.00055645763906F,
 -0.00051455722108F, -0.00046063254803F, -0.00040951214522F, -0.00035011758756F,
 -0.00028969811748F, -0.00020983373440F, -0.00014463809349F, -6.173344072E-005F,
  1.349497418E-005F,  0.00010943831274F,  0.00020430170688F,  0.00029495311041F,
  0.00040265402160F,  0.00051073884952F,  0.00062393761391F,  0.00074580258865F,
  0.00086084433262F,  0.00098859883015F,  0.00112501551307F,  0.00125778846475F,
  0.00139024948272F,  0.00154432198471F,  0.00168680832531F,  0.00183482654224F,
  0.00198411407369F,  0.00214615835557F,  0.00230172547746F,  0.00246256169126F,
  0.00262017586902F,  0.00278704643465F,  0.00294694477165F,  0.00311254206525F,
  0.00327396134847F,  0.00344188741828F,  0.00360082681231F,  0.00376039229104F,
  0.00392074323703F,  0.00408197531935F,  0.00422642692270F,  0.00437307196781F,
  0.00452098527825F,  0.00466064606118F,  0.00479325608498F,  0.00491376035745F,
  0.00503930226013F,  0.00514073539032F,  0.00524611661324F,  0.00534716811982F,
  0.00541967759307F,  0.00548760401507F,  0.00554757145088F,  0.00559380230045F,
  0.00562206432097F,  0.00564551969164F,  0.00563891995151F,  0.00562661141932F,
  0.00559171286630F,  0.00554043639400F,  0.00547537830770F,  0.00538389758970F,
  0.00527157587272F,  0.00513822754514F,  0.00498396877629F,  0.00481094690600F,
  0.00460395301471F,  0.00438018617447F,  0.00412516423270F,  0.00384564081246F,
  0.00354012465507F,  0.00320918858098F,  0.00284467578623F,  0.00245085400321F,
  0.00202741761850F,  0.00157846825768F,  0.00109023290512F,  0.00058322642480F,
  2.760451905E-005F, -0.00054642808664F, -0.00115681355227F, -0.00180394725893F,
 -0.00248267236449F, -0.00319337783900F, -0.00394011240522F, -0.00472225962400F,
 -0.00553372111088F, -0.00637922932685F, -0.00726158168517F, -0.00817982333726F,
 -0.00913253296085F, -0.01011502154986F, -0.01113155480321F, -0.01218499959508F,
  0.01327182200351F,  0.01439046660792F,  0.01554055533423F,  0.01673247129989F,
  0.01794333813443F,  0.01918724313698F,  0.02045317933555F,  0.02174675502535F,
  0.02306801692862F,  0.02441609920285F,  0.02578758475467F,  0.02718594296329F,
  0.02860721736385F,  0.03005026574279F,  0.03150176087389F,  0.03297540810337F,
  0.03446209487686F,  0.03596975605542F,  0.03748128504252F,  0.03900536794745F,
  0.04053491705584F,  0.04206490946367F,  0.04360975421304F,  0.04514884056413F,
  0.04668430272642F,  0.04821657200672F,  0.04973857556014F,  0.05125561555216F,
  0.05276307465207F,  0.05424527683589F,  0.05571736482138F,  0.05716164501299F,
  0.05859156836260F,  0.05998374801761F,  0.06134551717207F,  0.06268578081172F,
  0.06397158980681F,  0.06522471064380F,  0.06643675122104F,  0.06760759851228F,
  0.06870438283512F,  0.06976302447127F,  0.07076287107266F,  0.07170026731102F,
  0.07256825833083F,  0.07336202550803F,  0.07410036424342F,  0.07474525581194F,
  0.07531373362019F,  0.07580083586584F,  0.07619924793396F,  0.07649921704119F,
  0.07670934904245F,  0.07681739756964F,  0.07682300113923F,  0.07672049241746F,
  0.07650507183194F,  0.07617483218536F,  0.07573057565061F,  0.07515762552870F,
  0.07446643947564F,  0.07364060057620F,  0.07267746427299F,  0.07158263647903F,
  0.07035330735093F,  0.06896640131951F,  0.06745250215166F,  0.06576906686508F,
  0.06394448059633F,  0.06196027790387F,  0.05981665708090F,  0.05751526919867F,
  0.05504600343009F,  0.05240938217366F,  0.04959786763445F,  0.04663033051701F,
  0.04347687821958F,  0.04014582784127F,  0.03664181168133F,  0.03295839306691F,
  0.02908240060125F,  0.02503075618909F,  0.02079970728622F,  0.01637012582228F,
  0.01176238327857F,  0.00696368621617F,  0.00197656014503F, -0.00320868968304F,
 -0.00857117491366F, -0.01412888273558F, -0.01988341292573F, -0.02582272888064F,
 -0.03195312745332F, -0.03827765720822F, -0.04478068215856F, -0.05148041767934F,
 -0.05837053268336F, -0.06544098531359F, -0.07269433008129F, -0.08013729344279F,
 -0.08775475365593F, -0.09555333528914F, -0.10353295311463F, -0.11168269317730F,
 -0.12000779846800F, -0.12850028503878F, -0.13715517611934F, -0.14597664911870F,
 -0.15496070710605F, -0.16409588556669F, -0.17338081721706F, -0.18281725485142F,
 -0.19239667457267F, -0.20212501768103F, -0.21197358538056F, -0.22196526964149F,
 -0.23206908706791F, -0.24230168845974F, -0.25264803095722F, -0.26310532994603F,
 -0.27366340405625F, -0.28432141891085F, -0.29507167170646F, -0.30590985751916F,
 -0.31682789136456F, -0.32781137272105F, -0.33887226938665F, -0.34999141229310F,
  0.36115899031355F,  0.37237955463061F,  0.38363500139043F,  0.39492117615675F,
  0.40623176767625F,  0.41756968968409F,  0.42891199207373F,  0.44025537543665F,
  0.45159965356824F,  0.46293080852757F,  0.47424532146115F,  0.48552530911099F,
  0.49677082545707F,  0.50798175000434F,  0.51912349702391F,  0.53022408956855F,
  0.54125534487322F,  0.55220512585061F,  0.56307891401370F,  0.57385241316923F,
  0.58454032354679F,  0.59511230862496F,  0.60557835389180F,  0.61591099320291F,
  0.62612426956055F,  0.63619801077286F,  0.64612696959461F,  0.65590163024671F,
  0.66551398801627F,  0.67496631901712F,  0.68423532934598F,  0.69332823767032F,
  0.70223887193539F,  0.71094104263095F,  0.71944626349561F,  0.72774489002994F,
  0.73582117582769F,  0.74368278636488F,  0.75131374561237F,  0.75870807608242F,
  0.76586748650939F,  0.77277808813327F,  0.77942875190216F,  0.78583531203920F,
  0.79197358416424F,  0.79784664137700F,  0.80344857518505F,  0.80876950044491F,
  0.81381912706217F,  0.81857760046468F,  0.82304198905409F,  0.82722753473360F,
  0.83110384571520F,  0.83469373618402F,  0.83797173378865F,  0.84095413924722F,
  0.84362382812005F,  0.84598184698206F,  0.84803157770763F,  0.84978051984268F,
  0.85119715249343F,  0.85230470352147F,  0.85310209497017F,  0.85357205739107F,
  0.85373856005937F,  0.85357205739107F,  0.85310209497017F,  0.85230470352147F,
  0.85119715249343F,  0.84978051984268F,  0.84803157770763F,  0.84598184698206F,
  0.84362382812005F,  0.84095413924722F,  0.83797173378865F,  0.83469373618402F,
  0.83110384571520F,  0.82722753473360F,  0.82304198905409F,  0.81857760046468F,
  0.81381912706217F,  0.80876950044491F,  0.80344857518505F,  0.79784664137700F,
  0.79197358416424F,  0.78583531203920F,  0.77942875190216F,  0.77277808813327F,
  0.76586748650939F,  0.75870807608242F,  0.75131374561237F,  0.74368278636488F,
  0.73582117582769F,  0.72774489002994F,  0.71944626349561F,  0.71094104263095F,
  0.70223887193539F,  0.69332823767032F,  0.68423532934598F,  0.67496631901712F,
  0.66551398801627F,  0.65590163024671F,  0.64612696959461F,  0.63619801077286F,
  0.62612426956055F,  0.61591099320291F,  0.60557835389180F,  0.59511230862496F,
  0.58454032354679F,  0.57385241316923F,  0.56307891401370F,  0.55220512585061F,
  0.54125534487322F,  0.53022408956855F,  0.51912349702391F,  0.50798175000434F,
  0.49677082545707F,  0.48552530911099F,  0.47424532146115F,  0.46293080852757F,
  0.45159965356824F,  0.44025537543665F,  0.42891199207373F,  0.41756968968409F,
  0.40623176767625F,  0.39492117615675F,  0.38363500139043F,  0.37237955463061F,
 -0.36115899031355F, -0.34999141229310F, -0.33887226938665F, -0.32781137272105F,
 -0.31682789136456F, -0.30590985751916F, -0.29507167170646F, -0.28432141891085F,
 -0.27366340405625F, -0.26310532994603F, -0.25264803095722F, -0.24230168845974F,
 -0.23206908706791F, -0.22196526964149F, -0.21197358538056F, -0.20212501768103F,
 -0.19239667457267F, -0.18281725485142F, -0.17338081721706F, -0.16409588556669F,
 -0.15496070710605F, -0.14597664911870F, -0.13715517611934F, -0.12850028503878F,
 -0.12000779846800F, -0.11168269317730F, -0.10353295311463F, -0.09555333528914F,
 -0.08775475365593F, -0.08013729344279F, -0.07269433008129F, -0.06544098531359F,
 -0.05837053268336F, -0.05148041767934F, -0.04478068215856F, -0.03827765720822F,
 -0.03195312745332F, -0.02582272888064F, -0.01988341292573F, -0.01412888273558F,
 -0.00857117491366F, -0.00320868968304F,  0.00197656014503F,  0.00696368621617F,
  0.01176238327857F,  0.01637012582228F,  0.02079970728622F,  0.02503075618909F,
  0.02908240060125F,  0.03295839306691F,  0.03664181168133F,  0.04014582784127F,
  0.04347687821958F,  0.04663033051701F,  0.04959786763445F,  0.05240938217366F,
  0.05504600343009F,  0.05751526919867F,  0.05981665708090F,  0.06196027790387F,
  0.06394448059633F,  0.06576906686508F,  0.06745250215166F,  0.06896640131951F,
  0.07035330735093F,  0.07158263647903F,  0.07267746427299F,  0.07364060057620F,
  0.07446643947564F,  0.07515762552870F,  0.07573057565061F,  0.07617483218536F,
  0.07650507183194F,  0.07672049241746F,  0.07682300113923F,  0.07681739756964F,
  0.07670934904245F,  0.07649921704119F,  0.07619924793396F,  0.07580083586584F,
  0.07531373362019F,  0.07474525581194F,  0.07410036424342F,  0.07336202550803F,
  0.07256825833083F,  0.07170026731102F,  0.07076287107266F,  0.06976302447127F,
  0.06870438283512F,  0.06760759851228F,  0.06643675122104F,  0.06522471064380F,
  0.06397158980681F,  0.06268578081172F,  0.06134551717207F,  0.05998374801761F,
  0.05859156836260F,  0.05716164501299F,  0.05571736482138F,  0.05424527683589F,
  0.05276307465207F,  0.05125561555216F,  0.04973857556014F,  0.04821657200672F,
  0.04668430272642F,  0.04514884056413F,  0.04360975421304F,  0.04206490946367F,
  0.04053491705584F,  0.03900536794745F,  0.03748128504252F,  0.03596975605542F,
  0.03446209487686F,  0.03297540810337F,  0.03150176087389F,  0.03005026574279F,
  0.02860721736385F,  0.02718594296329F,  0.02578758475467F,  0.02441609920285F,
  0.02306801692862F,  0.02174675502535F,  0.02045317933555F,  0.01918724313698F,
  0.01794333813443F,  0.01673247129989F,  0.01554055533423F,  0.01439046660792F,
 -0.01327182200351F, -0.01218499959508F, -0.01113155480321F, -0.01011502154986F,
 -0.00913253296085F, -0.00817982333726F, -0.00726158168517F, -0.00637922932685F,
 -0.00553372111088F, -0.00472225962400F, -0.00394011240522F, -0.00319337783900F,
 -0.00248267236449F, -0.00180394725893F, -0.00115681355227F, -0.00054642808664F,
  2.760451905E-005F,  0.00058322642480F,  0.00109023290512F,  0.00157846825768F,
  0.00202741761850F,  0.00245085400321F,  0.00284467578623F,  0.00320918858098F,
  0.00354012465507F,  0.00384564081246F,  0.00412516423270F,  0.00438018617447F,
  0.00460395301471F,  0.00481094690600F,  0.00498396877629F,  0.00513822754514F,
  0.00527157587272F,  0.00538389758970F,  0.00547537830770F,  0.00554043639400F,
  0.00559171286630F,  0.00562661141932F,  0.00563891995151F,  0.00564551969164F,
  0.00562206432097F,  0.00559380230045F,  0.00554757145088F,  0.00548760401507F,
  0.00541967759307F,  0.00534716811982F,  0.00524611661324F,  0.00514073539032F,
  0.00503930226013F,  0.00491376035745F,  0.00479325608498F,  0.00466064606118F,
  0.00452098527825F,  0.00437307196781F,  0.00422642692270F,  0.00408197531935F,
  0.00392074323703F,  0.00376039229104F,  0.00360082681231F,  0.00344188741828F,
  0.00327396134847F,  0.00311254206525F,  0.00294694477165F,  0.00278704643465F,
  0.00262017586902F,  0.00246256169126F,  0.00230172547746F,  0.00214615835557F,
  0.00198411407369F,  0.00183482654224F,  0.00168680832531F,  0.00154432198471F,
  0.00139024948272F,  0.00125778846475F,  0.00112501551307F,  0.00098859883015F,
  0.00086084433262F,  0.00074580258865F,  0.00062393761391F,  0.00051073884952F,
  0.00040265402160F,  0.00029495311041F,  0.00020430170688F,  0.00010943831274F,
  1.349497418E-005F, -6.173344072E-005F, -0.00014463809349F, -0.00020983373440F,
 -0.00028969811748F, -0.00035011758756F, -0.00040951214522F, -0.00046063254803F,
 -0.00051455722108F, -0.00055645763906F, -0.00059461189330F, -0.00063415949025F,
 -0.00066504150893F, -0.00069179375372F, -0.00072153919876F, -0.00073193571525F,
 -0.00075300014201F, -0.00076307935757F, -0.00077579773310F, -0.00078014496257F,
 -0.00078036647100F, -0.00077798694927F, -0.00078343322877F, -0.00077248485949F,
 -0.00076813719270F, -0.00074905980532F, -0.00074409418541F, -0.00072550431222F,
 -0.00071577364744F, -0.00069416146273F, -0.00067776907764F, -0.00065403333621F,
 -0.00063124935319F, -0.00061327473938F, -0.00058709304852F, -0.00056778025613F,
 -0.00054665656337F, -0.00052265642972F, -0.00050407143497F, -0.00048937912498F,
 -0.00048752279712F, -0.00049475180896F, -0.00056176925738F, -0.00055252865047F
};

static const complex_t qmf32_pre_twiddle[] __attribute__(   (  section( ".rodata" )  )   ) = {
 { 0.999924701839145F, -0.012271538285720F }, { 0.999322384588350F, -0.036807222941359F },
 { 0.998118112900149F, -0.061320736302209F }, { 0.996312612182778F, -0.085797312344440F },
 { 0.993906970002356F, -0.110222207293883F }, { 0.990902635427780F, -0.134580708507126F },
 { 0.987301418157858F, -0.158858143333861F }, { 0.983105487431216F, -0.183039887955141F },
 { 0.978317370719628F, -0.207111376192219F }, { 0.972939952205560F, -0.231058108280671F },
 { 0.966976471044852F, -0.254865659604515F }, { 0.960430519415566F, -0.278519689385053F },
 { 0.953306040354194F, -0.302005949319228F }, { 0.945607325380521F, -0.325310292162263F },
 { 0.937339011912575F, -0.348418680249435F }, { 0.928506080473216F, -0.371317193951838F },
 { 0.919113851690058F, -0.393992040061048F }, { 0.909167983090522F, -0.416429560097637F },
 { 0.898674465693954F, -0.438616238538528F }, { 0.887639620402854F, -0.460538710958240F },
 { 0.876070094195407F, -0.482183772079123F }, { 0.863972856121587F, -0.503538383725718F },
 { 0.851355193105265F, -0.524589682678469F }, { 0.838224705554838F, -0.545324988422046F },
 { 0.824589302785025F, -0.565731810783613F }, { 0.810457198252595F, -0.585797857456439F },
 { 0.795836904608884F, -0.605511041404326F }, { 0.780737228572094F, -0.624859488142386F },
 { 0.765167265622459F, -0.643831542889791F }, { 0.749136394523459F, -0.662415777590172F },
 { 0.732654271672413F, -0.680600997795453F }, { 0.715730825283819F, -0.698376249408973F }
};

static void qmfa_end ( qmfa_info* qmfa ) {
 if ( qmfa ) {
  if ( qmfa -> x ) free ( qmfa -> x );
  free ( qmfa );
 }  /* end if */
}  /* end qmfa_end */

static void qmfs_end ( qmfs_info* qmfs ) {
 if ( qmfs ) {
  if ( qmfs -> v ) free ( qmfs -> v );
  free ( qmfs );
 }  /* end if */
}  /* end qmfs_end */

static void sbrDecodeEnd ( sbr_info* sbr ) {

 uint8_t i;

 qmfa_end ( sbr -> qmfa[ 0 ] );
 qmfs_end ( sbr -> qmfs[ 0 ] );
 if ( sbr -> qmfs[ 1 ] ) {
  qmfa_end ( sbr -> qmfa[ 1 ] );
  qmfs_end ( sbr -> qmfs[ 1 ] );
 }  /* end if */

 for ( i = 0; i < 5; ++i ) {
  if ( sbr -> G_temp_prev[ 0 ][ i ] ) free ( sbr -> G_temp_prev[ 0 ][ i ] );
  if ( sbr -> Q_temp_prev[ 0 ][ i ] ) free ( sbr -> Q_temp_prev[ 0 ][ i ] );
  if ( sbr -> G_temp_prev[ 1 ][ i ] ) free ( sbr -> G_temp_prev[ 1 ][ i ] );
  if ( sbr -> Q_temp_prev[ 1 ][ i ] ) free ( sbr -> Q_temp_prev[ 1 ][ i ] );
 }  /* end for */

 free ( sbr );

}  /* end sbrDecodeEnd */

static void sbrReset ( sbr_info* sbr ) {
 uint8_t i;
 if ( sbr -> qmfa[ 0 ] ) memset (  sbr -> qmfa[ 0 ] -> x, 0, 2 * sbr -> qmfa[ 0 ] -> channels * 10 * sizeof ( float )  );
 if ( sbr -> qmfa[ 1 ] ) memset (  sbr -> qmfa[ 1 ] -> x, 0, 2 * sbr -> qmfa[ 1 ] -> channels * 10 * sizeof ( float )  );
 if ( sbr -> qmfs[ 0 ] ) memset (  sbr -> qmfs[ 0 ] -> v, 0, 2 * sbr -> qmfs[ 0 ] -> channels * 20 * sizeof ( float )  );
 if ( sbr -> qmfs[ 1 ] ) memset (  sbr -> qmfs[ 1 ] -> v, 0, 2 * sbr -> qmfs[ 1 ] -> channels * 20 * sizeof ( float )  );
 for ( i = 0; i < 5; ++i ) {
  if ( sbr -> G_temp_prev[ 0 ][ i ] ) memset (  sbr -> G_temp_prev[ 0 ][ i ], 0, 64 * sizeof ( float )  );
  if ( sbr -> G_temp_prev[ 1 ][ i ] ) memset (  sbr -> G_temp_prev[ 1 ][ i ], 0, 64 * sizeof ( float )  );
  if ( sbr -> Q_temp_prev[ 0 ][ i ] ) memset (  sbr -> Q_temp_prev[ 0 ][ i ], 0, 64 * sizeof ( float )  );
  if ( sbr -> Q_temp_prev[ 1 ][ i ] ) memset (  sbr -> Q_temp_prev[ 1 ][ i ], 0, 64 * sizeof ( float )  );
 }  /* end for */
 memset (  sbr -> Xsbr[ 0 ], 0, ( sbr -> numTimeSlotsRate + sbr -> tHFGen ) * 64 * sizeof ( float )  );
 memset (  sbr -> Xsbr[ 1 ], 0, ( sbr -> numTimeSlotsRate + sbr -> tHFGen ) * 64 * sizeof ( float )  );
 sbr -> GQ_ringbuf_index[ 0 ] = 0;
 sbr -> GQ_ringbuf_index[ 1 ] = 0;
 sbr -> header_count          = 0;
 sbr -> Reset                 = 1;
 sbr -> L_E_prev[ 0 ]         = 0;
 sbr -> L_E_prev[ 1 ]         =  0;
 sbr -> bs_freq_scale         =  2;
 sbr -> bs_alter_scale        =  1;
 sbr -> bs_noise_bands        =  2;
 sbr -> bs_limiter_bands      =  2;
 sbr -> bs_limiter_gains      =  2;
 sbr -> bs_interpol_freq      =  1;
 sbr -> bs_smoothing_mode     =  1;
 sbr -> bs_start_freq         =  5;
 sbr -> bs_amp_res            =  1;
 sbr -> bs_samplerate_mode    =  1;
 sbr -> prevEnvIsShort[ 0 ]   = -1;
 sbr -> prevEnvIsShort[ 1 ]   = -1;
 sbr -> bsco                  =  0;
 sbr -> bsco_prev             =  0;
 sbr -> M_prev                =  0;
 sbr -> bs_start_freq_prev    = -1;
 sbr -> f_prev[ 0 ]           =  0;
 sbr -> f_prev[ 1 ]           =  0;
 for ( i = 0; i < MAX_M; ++i ) {
  sbr -> E_prev[ 0 ][ i ]               = 0;
  sbr -> Q_prev[ 0 ][ i ]               = 0;
  sbr -> E_prev[ 1 ][ i ]               = 0;
  sbr -> Q_prev[ 1 ][ i ]               = 0;
  sbr -> bs_add_harmonic_prev[ 0 ][ i ] = 0;
  sbr -> bs_add_harmonic_prev[ 1 ][ i ] = 0;
 }  /* end for */
 sbr -> bs_add_harmonic_flag_prev[ 0 ] = 0;
 sbr -> bs_add_harmonic_flag_prev[ 1 ] = 0;
}  /* end sbrReset */

static qmfa_info* qmfa_init ( uint8_t channels ) {
 qmfa_info* qmfa = ( qmfa_info* )malloc (  sizeof ( qmfa_info )  );
 qmfa -> x        = ( float* )calloc (  1, 2 * channels * 10 * sizeof ( float )  );
 qmfa -> x_index  = 0;
 qmfa -> channels = channels;
 return qmfa;
}  /* end qmfa_init */

static qmfs_info* qmfs_init ( uint8_t channels ) {
 qmfs_info* qmfs = ( qmfs_info* )malloc (  sizeof ( qmfs_info )  );
 qmfs -> v        = ( float* )calloc (  1, 2 * channels * 20 * sizeof ( float )  );
 qmfs -> v_index  = 0;
 qmfs -> channels = channels;
 return qmfs;
}  /* end qmfs_init */

static sbr_info* sbrDecodeInit (
                  uint16_t framelength, uint8_t id_aac, uint32_t sample_rate, uint8_t downSampledSBR
                 ) {

 sbr_info* sbr = calloc (  1, sizeof ( sbr_info )  );
 sbr -> id_aac              = id_aac;
 sbr -> sample_rate         = sample_rate;
 sbr -> bs_freq_scale       =  2;
 sbr -> bs_alter_scale      =  1;
 sbr -> bs_noise_bands      =  2;
 sbr -> bs_limiter_bands    =  2;
 sbr -> bs_limiter_gains    =  2;
 sbr -> bs_interpol_freq    =  1;
 sbr -> bs_smoothing_mode   =  1;
 sbr -> bs_start_freq       =  5;
 sbr -> bs_amp_res          =  1;
 sbr -> bs_samplerate_mode  =  1;
 sbr -> prevEnvIsShort[ 0 ] = -1;
 sbr -> prevEnvIsShort[ 1 ] = -1;
 sbr -> header_count        =  0;
 sbr -> Reset               =  1;
 sbr -> tHFGen              = T_HFGEN;
 sbr -> tHFAdj              = T_HFADJ;
 sbr -> bsco                =  0;
 sbr -> bsco_prev           =  0;
 sbr -> M_prev              =  0;
 sbr -> frame_len           = framelength;
 sbr -> bs_start_freq_prev  = -1;
 if ( framelength == 960 ) {
  sbr -> numTimeSlotsRate = RATE * NO_TIME_SLOTS_960;
  sbr -> numTimeSlots     = NO_TIME_SLOTS_960;
 } else {
  sbr -> numTimeSlotsRate = RATE * NO_TIME_SLOTS;
  sbr -> numTimeSlots     = NO_TIME_SLOTS;
 }  /* end else */
 sbr -> GQ_ringbuf_index[ 0 ] = 0;
 sbr -> GQ_ringbuf_index[ 1 ] = 0;
 if ( id_aac == ID_CPE ) {
  uint8_t i;
  sbr -> qmfa[ 0 ] = qmfa_init ( 32 );
  sbr -> qmfa[ 1 ] = qmfa_init ( 32 );
  sbr -> qmfs[ 0 ] = qmfs_init ( downSampledSBR ? 32 : 64 );
  sbr -> qmfs[ 1 ] = qmfs_init ( downSampledSBR ? 32 : 64 );
  for ( i = 0; i < 5; ++i ) {
   sbr -> G_temp_prev[ 0 ][ i ] = malloc (  64 * sizeof ( float )  );
   sbr -> G_temp_prev[ 1 ][ i ] = malloc (  64 * sizeof ( float )  );
   sbr -> Q_temp_prev[ 0 ][ i ] = malloc (  64 * sizeof ( float )  );
   sbr -> Q_temp_prev[ 1 ][ i ] = malloc (  64 * sizeof ( float )  );
  }  /* end for */
  memset (  sbr -> Xsbr[ 0 ], 0, ( sbr -> numTimeSlotsRate + sbr -> tHFGen ) * 64 * sizeof ( qmf_t )  );
  memset (  sbr -> Xsbr[ 1 ], 0, ( sbr -> numTimeSlotsRate + sbr -> tHFGen ) * 64 * sizeof ( qmf_t )  );
 } else {
  uint8_t i;
  sbr -> qmfa[ 0 ] = qmfa_init ( 32 );
  sbr -> qmfs[ 0 ] = qmfs_init ( downSampledSBR ? 32 : 64 );
  sbr -> qmfs[ 1 ] = NULL;
  for ( i = 0; i < 5; ++i ) {
   sbr -> G_temp_prev[ 0 ][ i ] = malloc (  64 * sizeof ( float )  );
   sbr -> Q_temp_prev[ 0 ][ i ] = malloc (  64 * sizeof ( float )  );
  }  /* end for */
  memset (  sbr -> Xsbr[ 0 ], 0, ( sbr -> numTimeSlotsRate + sbr -> tHFGen ) * 64 * sizeof ( qmf_t )  );
 }  /* end else */
 return sbr;
}  /* end sbrDecodeInit */

static void sbr_header ( SMS_BitContext* ld, sbr_info* sbr ) {
 uint8_t bs_header_extra_1, bs_header_extra_2;
 sbr -> header_count += 1;
 sbr -> bs_amp_res    = SMS_GetBit ( ld );
 sbr -> bs_start_freq = ( uint8_t )SMS_GetBits ( ld, 4 );
 sbr -> bs_stop_freq  = ( uint8_t )SMS_GetBits ( ld, 4 );
 sbr -> bs_xover_band = ( uint8_t )SMS_GetBits ( ld, 3 );
 SMS_SkipBits ( ld, 2 );
 bs_header_extra_1 = ( uint8_t )SMS_GetBit ( ld );
 bs_header_extra_2 = ( uint8_t )SMS_GetBit ( ld );
 if ( bs_header_extra_1 ) {
  sbr -> bs_freq_scale  = ( uint8_t )SMS_GetBits ( ld, 2 );
  sbr -> bs_alter_scale = ( uint8_t )SMS_GetBit  ( ld );
  sbr -> bs_noise_bands = ( uint8_t )SMS_GetBits ( ld, 2 );
 } else {
  sbr -> bs_freq_scale  = 2;
  sbr -> bs_alter_scale = 1;
  sbr -> bs_noise_bands = 2;
 }  /* end else */
 if ( bs_header_extra_2 ) {
  sbr -> bs_limiter_bands  = ( uint8_t )SMS_GetBits ( ld, 2 );
  sbr -> bs_limiter_gains  = ( uint8_t )SMS_GetBits ( ld, 2 );
  sbr -> bs_interpol_freq  = ( uint8_t )SMS_GetBit  ( ld );
  sbr -> bs_smoothing_mode = ( uint8_t )SMS_GetBit  ( ld );
 } else {
  sbr -> bs_limiter_bands  = 2;
  sbr -> bs_limiter_gains  = 2;
  sbr -> bs_interpol_freq  = 1;
  sbr -> bs_smoothing_mode = 1;
 }  /* end else */
}  /* end sbr_header */

static void sbr_reset ( sbr_info* sbr ) {
 sbr -> Reset = ( sbr -> bs_start_freq  != sbr -> bs_start_freq_prev  ) ||
                ( sbr -> bs_stop_freq   != sbr -> bs_stop_freq_prev   ) ||
                ( sbr -> bs_freq_scale  != sbr -> bs_freq_scale_prev  ) ||
                ( sbr -> bs_alter_scale != sbr -> bs_alter_scale_prev ) ||
                ( sbr -> bs_xover_band  != sbr -> bs_xover_band_prev  ) ||
                ( sbr -> bs_noise_bands != sbr -> bs_noise_bands_prev );
 sbr -> bs_start_freq_prev  = sbr -> bs_start_freq;
 sbr -> bs_stop_freq_prev   = sbr -> bs_stop_freq;
 sbr -> bs_freq_scale_prev  = sbr -> bs_freq_scale;
 sbr -> bs_alter_scale_prev = sbr -> bs_alter_scale;
 sbr -> bs_xover_band_prev  = sbr -> bs_xover_band;
 sbr -> bs_noise_bands_prev = sbr -> bs_noise_bands;
}  /* end sbr_reset */

static uint8_t get_sr_index ( const uint32_t samplerate ) {
 if ( 92017    <= samplerate ) return  0;
 if ( 75132    <= samplerate ) return  1;
 if ( 55426    <= samplerate ) return  2;
 if ( 46009    <= samplerate ) return  3;
 if ( 37566    <= samplerate ) return  4;
 if ( 27713    <= samplerate ) return  5;
 if ( 23004    <= samplerate ) return  6;
 if ( 18783    <= samplerate ) return  7;
 if ( 13856    <= samplerate ) return  8;
 if ( 11502    <= samplerate ) return  9;
 if ( 9391     <= samplerate ) return 10;
 if ( 16428320 <= samplerate ) return 11;
 return 11;
}  /* end get_sr_index */

static uint8_t qmf_start_channel (
                uint8_t bs_start_freq, uint8_t bs_samplerate_mode, uint32_t sample_rate
               ) {
 static const uint8_t startMinTable[ 12 ] __attribute__(   (  section( ".rodata" )  )   ) = {
  7, 7, 10, 11, 12, 16, 16, 17, 24, 32, 35, 48
 };
 static const uint8_t offsetIndexTable[ 12 ] __attribute__(   (  section( ".rodata" )  )   ) = {
  5, 5, 4, 4, 4, 3, 2, 1, 0, 6, 6, 6
 };
 static const int8_t offset[ 7 ][ 16 ] __attribute__(   (  section( ".rodata" )  )   ) = {
  { -8, -7, -6, -5, -4, -3, -2, -1, 0,  1,  2,  3,  4,  5,  6,  7 },
  { -5, -4, -3, -2, -1,  0,  1,  2, 3,  4,  5,  6,  7,  9, 11, 13 },
  { -5, -3, -2, -1,  0,  1,  2,  3, 4,  5,  6,  7,  9, 11, 13, 16 },
  { -6, -4, -2, -1,  0,  1,  2,  3, 4,  5,  6,  7,  9, 11, 13, 16 },
  { -4, -2, -1,  0,  1,  2,  3,  4, 5,  6,  7,  9, 11, 13, 16, 20 },
  { -2, -1,  0,  1,  2,  3,  4,  5, 6,  7,  9, 11, 13, 16, 20, 24 },
  {  0,  1,  2,  3,  4,  5,  6,  7, 9, 11, 13, 16, 20, 24, 28, 33 }
 };
 uint8_t startMin    = startMinTable   [ get_sr_index ( sample_rate ) ];
 uint8_t offsetIndex = offsetIndexTable[ get_sr_index ( sample_rate ) ];
 if ( bs_samplerate_mode ) return startMin + offset[ offsetIndex ][ bs_start_freq ];
 return startMin + offset[ 6 ][ bs_start_freq ];
}  /* end qmf_start_channel */

static uint8_t qmf_stop_channel (
                uint8_t bs_stop_freq, uint32_t sample_rate, uint8_t k0
               ) {
 static const uint8_t stopMinTable[ 12 ] __attribute__(   (  section( ".rodata" )  )   ) = {
  13, 15, 20, 21, 23, 32, 32, 35, 48, 64, 70, 96
 };
 static const int8_t offset[ 12 ][ 14 ] __attribute__(   (  section( ".rodata" )  )   ) = {
  { 0,  2,  4,  6,   8,  11,  14,  18,  22,  26,  31,  37,  44,  51 },
  { 0,  2,  4,  6,   8,  11,  14,  18,  22,  26,  31,  36,  42,  49 },
  { 0,  2,  4,  6,   8,  11,  14,  17,  21,  25,  29,  34,  39,  44 },
  { 0,  2,  4,  6,   8,  11,  14,  17,  20,  24,  28,  33,  38,  43 },
  { 0,  2,  4,  6,   8,  11,  14,  17,  20,  24,  28,  32,  36,  41 },
  { 0,  2,  4,  6,   8,  10,  12,  14,  17,  20,  23,  26,  29,  32 },
  { 0,  2,  4,  6,   8,  10,  12,  14,  17,  20,  23,  26,  29,  32 },
  { 0,  1,  3,  5,   7,   9,  11,  13,  15,  17,  20,  23,  26,  29 },
  { 0,  1,  2,  3,   4,   5,   6,   7,   8,   9,  10,  12,  14,  16 },
  { 0,  0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
  { 0, -1, -2, -3,  -4,  -5,  -6,  -6,  -6,  -6,  -6,  -6,  -6,  -6 },
  { 0, -3, -6, -9, -12, -15, -18, -20, -22, -24, -26, -28, -30, -32 }
 };
 uint8_t stopMin;
 uint8_t i, j;
 if ( bs_stop_freq == 15 ) return min ( 64, k0 * 3 );
 if ( bs_stop_freq == 14 ) return min ( 64, k0 * 2 );
 stopMin = stopMinTable[ get_sr_index ( sample_rate ) ];
 j       = min( bs_stop_freq, 13 );
 i       = stopMin + offset[ get_sr_index ( sample_rate ) ][ j ];
 return min ( 64, i );
}  /* end qmf_stop_channel */

static uint8_t master_frequency_table_fs0 (
                sbr_info* sbr, uint8_t k0, uint8_t k2, uint8_t bs_alter_scale
               ) {
 int8_t   incr;
 uint8_t  k;
 uint8_t  dk;
 uint32_t nrBands, k2Achieved;
 int32_t  k2Diff, vDk[ 64 ] = { 0 };
 if ( k2 <= k0 ) {
  sbr -> N_master = 0;
  return 1;
 }  /* end if */
 dk = bs_alter_scale ? 2 : 1;
 if ( bs_alter_scale )
  nrBands = (   (  ( k2 - k0 + 2 ) >> 2  ) << 1   );
 else nrBands = (   (  ( k2 - k0 ) >> 1  ) << 1   );
 nrBands = min ( nrBands, 63 );
 if ( nrBands <= 0 ) return 1;
 k2Achieved = k0 + nrBands * dk;
 k2Diff     = k2 - k2Achieved;
 for ( k = 0; k < nrBands; ++k ) vDk[ k ] = dk;
 if ( k2Diff ) {
  incr = ( k2Diff > 0 ) ? -1 : 1;
  k    = ( uint8_t )(  ( k2Diff > 0 ) ? ( nrBands -1 ) : 0  );
  while ( k2Diff != 0 ) {
   vDk[ k ] -= incr;
   k        += incr;
   k2Diff   += incr;
  }  /* end while */
 }  /* end if */
 sbr -> f_master[ 0 ] = k0;
 for ( k = 1; k <= nrBands; ++k ) sbr -> f_master[ k ] = ( uint8_t )( sbr -> f_master[ k - 1 ] + vDk[ k - 1 ] );
 sbr -> N_master = ( uint8_t )nrBands;
 sbr -> N_master = min( sbr -> N_master, 64 );
 return 0;
}  /* end master_frequency_table_fs0 */

static int longcmp ( const void* a, const void* b ) {
 return ( int )(  *( int32_t* )a - *( int32_t* )b  );
}  /* end longcmp */

static int32_t SMS_INLINE find_bands ( uint8_t warp, uint8_t bands, uint8_t a0, uint8_t a1 ) {
 float div = 0.693147F;
 if ( warp ) div *= 1.3F;
 return ( int32_t )(  bands * logf (  ( float )a1 / ( float )a0 ) / div + 0.5F  );
}  /* end find_bands */

static float SMS_INLINE find_initial_power ( uint8_t bands, uint8_t a0, uint8_t a1 ) {
 return ( float )PowF (  ( float )a1 / ( float )a0, 1.0 / ( float ) bands  );
}  /* end find_initial_power */

static uint8_t derived_frequency_table (
                sbr_info* sbr, uint8_t bs_xover_band, uint8_t k2
               ) {
 uint8_t  k, i;
 uint32_t minus;
 if ( sbr -> N_master <= bs_xover_band ) return 1;
 sbr -> N_high = sbr->N_master - bs_xover_band;
 sbr -> N_low  = ( sbr -> N_high >> 1 ) + (   sbr -> N_high - (  ( sbr -> N_high >> 1 ) << 1  )   );
 sbr -> n[ 0 ] = sbr -> N_low;
 sbr -> n[ 1 ] = sbr -> N_high;
 for ( k = 0; k <= sbr -> N_high; ++k ) sbr -> f_table_res[ HI_RES ][ k ] = sbr -> f_master[ k + bs_xover_band ];
 sbr -> M  = sbr -> f_table_res[ HI_RES ][ sbr -> N_high ] - sbr -> f_table_res[ HI_RES ][ 0 ];
 sbr -> kx = sbr -> f_table_res[ HI_RES ][ 0 ];
 if ( sbr -> kx            > 32 ) return 1;
 if ( sbr -> kx + sbr -> M > 64 ) return 1;
 minus = sbr -> N_high & 1;
 for ( k = 0; k <= sbr -> N_low; ++k ) {
  if ( k == 0 )
   i = 0;
  else i = ( uint8_t )( 2 * k - minus );
  sbr -> f_table_res[ LO_RES ][ k ] = sbr -> f_table_res[ HI_RES ][ i ];
 }  /* end for */
 sbr -> N_Q = 0;
 if ( sbr -> bs_noise_bands == 0 )
  sbr -> N_Q = 1;
 else {
  int lVal = find_bands ( 0, sbr -> bs_noise_bands, sbr -> kx, k2 );
  sbr -> N_Q = ( uint8_t )max( 1, lVal );
  sbr -> N_Q = min ( 5, sbr -> N_Q );
 }  /* end else */
 for ( k = 0; k <= sbr -> N_Q; ++k ) {
  if ( k == 0 )
   i = 0;
  else i = i + ( sbr -> N_low - i ) / ( sbr -> N_Q + 1 - k );
  sbr -> f_table_noise[ k ] = sbr -> f_table_res[ LO_RES ][ i ];
 }  /* end for */
 for ( k = 0; k < 64; ++k ) {
  uint8_t g;
  for ( g = 0; g < sbr -> N_Q; ++g ) if (  ( sbr -> f_table_noise[ g ] <= k  ) &&
                                           ( k < sbr->f_table_noise[ g + 1 ] )
                                     ) {
   sbr -> table_map_k_to_g[ k ] = g;
   break;
  }  /* end if */
 }  /* end for */
 return 0;
}  /* end derived_frequency_table */

static SMS_INLINE int8_t sbr_log2 ( const int8_t val ) {
 int8_t log2tab[ 10 ] = { 0, 0, 1, 2, 2, 3, 3, 3, 3, 4 };
 return val < 10 && val >= 0 ? log2tab[ val ] : 0;
}  /* end sbr_log2 */

static uint8_t master_frequency_table (
                sbr_info* sbr, uint8_t k0, uint8_t k2,
                uint8_t bs_freq_scale, uint8_t bs_alter_scale
               ) {
 uint8_t k, bands, twoRegions;
 uint8_t k1;
 uint8_t nrBand0, nrBand1;
 int32_t vDk0[ 64 ] = { 0 }, vDk1[ 64 ] = { 0 };
 int32_t vk0 [ 64 ] = { 0 }, vk1 [ 64 ] = { 0 };
 uint8_t temp1[ 3 ] = { 6, 5, 4 };
 float   q, qk;
 int32_t A_1;
 if ( k2 <= k0 ) {
  sbr -> N_master = 0;
  return 1;
 }  /* end if */
 bands = temp1[ bs_freq_scale - 1 ];
 if (  ( float )k2 / ( float )k0 > 2.2449F  ) {
  twoRegions = 1;
  k1         = k0 << 1;
 } else {
  twoRegions = 0;
  k1         = k2;
 }  /* end eles */
 nrBand0 = ( uint8_t )(  2 * find_bands ( 0, bands, k0, k1 )  );
 nrBand0 = min( nrBand0, 63 );
 if ( nrBand0 <= 0 ) return 1;
 q   = find_initial_power ( nrBand0, k0, k1 );
 qk  = ( float )k0;
 A_1 = (int32_t)( qk + 0.5F );
 for ( k = 0; k <= nrBand0; ++k ) {
  int32_t A_0 = A_1;
  qk *= q;
  A_1 = ( int32_t )( qk + 0.5F );
  vDk0[ k ] = A_1 - A_0;
 }  /* end for */
 qsort (  vDk0, nrBand0, sizeof ( vDk0[ 0 ] ), longcmp  );
 vk0[ 0 ] = k0;
 for ( k = 1; k <= nrBand0; ++k ) {
  vk0[ k ] = vk0[ k - 1 ] + vDk0[ k - 1 ];
  if ( vDk0[ k - 1 ] == 0 ) return 1;
 }  /* end for */
 if ( !twoRegions ) {
  for ( k = 0; k <= nrBand0; ++k ) sbr -> f_master[ k ] = ( uint8_t )vk0[ k ];
  sbr -> N_master = nrBand0;
  sbr -> N_master = min( sbr -> N_master, 64 );
  return 0;
 }  /* end if */
 nrBand1 = ( uint8_t )(  2 * find_bands ( 1, bands, k1, k2 )  );
 nrBand1 = min ( nrBand1, 63 );
 q       = find_initial_power ( nrBand1, k1, k2 );
 qk      = ( float )k1;
 A_1     = ( int32_t )( qk + 0.5F );
 for ( k = 0; k <= nrBand1 - 1; ++k ) {
  int32_t A_0 = A_1;
  qk *= q;
  A_1 = ( int32_t )( qk + 0.5F );
  vDk1[ k ] = A_1 - A_0;
 }  /* end for */
 if ( vDk1[ 0 ] < vDk0[ nrBand0 - 1 ] ) {
  int32_t change;
  qsort (  vDk1, nrBand1 + 1, sizeof ( vDk1[ 0 ] ), longcmp  );
  change = vDk0[ nrBand0 - 1 ] - vDk1[ 0 ];
  vDk1[ 0 ]           = vDk0[ nrBand0 - 1 ];
  vDk1[ nrBand1 - 1 ] = vDk1[ nrBand1 - 1 ] - change;
 }  /* end if */
 qsort (  vDk1, nrBand1, sizeof ( vDk1[ 0 ] ), longcmp  );
 vk1[ 0 ] = k1;
 for ( k = 1; k <= nrBand1; ++k ) {
  vk1[ k ] = vk1[ k - 1 ] + vDk1[ k - 1 ];
  if ( vDk1[ k - 1 ] == 0 ) return 1;
 }  /* end for */
 sbr -> N_master = nrBand0 + nrBand1;
 sbr -> N_master = min( sbr -> N_master, 64 );
 for ( k = 0; k <= nrBand0; ++k ) sbr -> f_master[ k ] = ( uint8_t )vk0[ k ];
 for ( k = nrBand0 + 1; k <= sbr -> N_master; ++k ) sbr -> f_master[ k ] = ( uint8_t )vk1[ k - nrBand0 ];
 return 0;
}  /* end master_frequency_table */

static uint8_t envelope_time_border_vector ( sbr_info* sbr, uint8_t ch ) {

 uint8_t l, border, temp;
 uint8_t t_E_temp[ 6 ] = { 0 };

 t_E_temp[  0                 ] = sbr -> rate * sbr -> abs_bord_lead [ ch ];
 t_E_temp[  sbr -> L_E[ ch ]  ] = sbr -> rate * sbr -> abs_bord_trail[ ch ];

 switch ( sbr -> bs_frame_class[ ch ] ) {

  case FIXFIX:

   switch ( sbr -> L_E[ ch ] ) {
    case 4:
     temp = (int) (sbr->numTimeSlots / 4);
     t_E_temp[ 3 ] = sbr -> rate * 3 * temp;
     t_E_temp[ 2 ] = sbr -> rate * 2 * temp;
     t_E_temp[ 1 ] = sbr -> rate * temp;
    break;
    case 2:
     t_E_temp[ 1 ] = sbr -> rate * ( int )( sbr -> numTimeSlots / 2 );
    break;
    default: break;
   }  /* end switch */

  break;

  case FIXVAR:

   if ( sbr -> L_E[ ch ] > 1 ) {
    int8_t i = sbr -> L_E[ ch ];
    border = sbr -> abs_bord_trail[ ch ];
    for ( l = 0; l < sbr -> L_E[ ch ] - 1; ++l ) {
     if ( border < sbr -> bs_rel_bord[ ch ][ l ] ) return 1;
     border       -= sbr -> bs_rel_bord[ ch ][ l ];
     t_E_temp[--i] = sbr -> rate * border;
    }  /* end for */
   }  /* end if */

  break;

  case VARFIX:

   if ( sbr -> L_E[ ch ] > 1 ) {
    int8_t i = 1;
    border = sbr -> abs_bord_lead[ ch ];
    for ( l = 0; l < sbr -> L_E[ ch ] - 1; ++l ) {
     border += sbr -> bs_rel_bord[ ch ][ l ];
     if ( sbr -> rate * border + sbr -> tHFAdj > sbr -> numTimeSlotsRate + sbr -> tHFGen ) return 1;
     t_E_temp[ i++ ] = sbr -> rate * border;
    }  /* end for */
   }  /* end if */

  break;

  case VARVAR:
   if ( sbr -> bs_num_rel_0[ ch ] ) {
    int8_t i = 1;
    border = sbr -> abs_bord_lead[ ch ];
    for ( l = 0; l < sbr -> bs_num_rel_0[ ch ]; ++l ) {
     border += sbr -> bs_rel_bord_0[ ch ][ l ];
     if ( sbr -> rate * border + sbr -> tHFAdj > sbr -> numTimeSlotsRate+sbr -> tHFGen ) return 1;
     t_E_temp[ i++ ] = sbr -> rate * border;
    }  /* end for */
   }  /* end if */
   if ( sbr -> bs_num_rel_1[ ch ] ) {
    int8_t i = sbr -> L_E[ ch ];
    border = sbr -> abs_bord_trail[ ch ];
    for ( l = 0; l < sbr -> bs_num_rel_1[ ch ]; ++l ) {
     if ( border < sbr -> bs_rel_bord_1[ ch ][ l ] ) return 1;
     border -= sbr -> bs_rel_bord_1[ ch ][ l ];
     t_E_temp[ --i ] = sbr -> rate * border;
    }  /* end for */
   }  /* end if */

  break;

 }  /* end switch */

 for ( l = 0; l < 6; ++l ) sbr -> t_E[ ch ][ l ] = t_E_temp[ l ];

 return 0;

}  /* end envelope_time_border_vector */

static uint8_t middleBorder ( sbr_info* sbr, uint8_t ch ) {
 int8_t retval = 0;
 switch ( sbr -> bs_frame_class[ ch ] ) {
  case FIXFIX:
   retval = sbr -> L_E[ ch ] / 2;
  break;
  case VARFIX:
   if ( sbr -> bs_pointer[ ch ] == 0 )
    retval = 1;
   else if ( sbr -> bs_pointer[ ch ] == 1 )
    retval = sbr -> L_E[ ch ] - 1;
   else retval = sbr -> bs_pointer[ ch ] - 1;
  break;
  case FIXVAR:
  case VARVAR:
   if ( sbr -> bs_pointer[ ch ] > 1 )
    retval = sbr -> L_E[ ch ] + 1 - sbr -> bs_pointer[ ch ];
   else retval = sbr -> L_E[ ch ] - 1;
  break;
 }  /* end switch */
 return retval > 0 ? retval : 0;
}  /* end middleBorder */

static void noise_floor_time_border_vector ( sbr_info* sbr, uint8_t ch ) {
 sbr -> t_Q[ ch ][ 0 ] = sbr -> t_E[ ch ][ 0 ];
 if ( sbr -> L_E[ ch ] == 1 ) {
  sbr -> t_Q[ ch ][ 1 ] = sbr -> t_E[ ch ][ 1 ];
  sbr -> t_Q[ ch ][ 2 ] = 0;
 } else {
  uint8_t index = middleBorder ( sbr, ch );
  sbr -> t_Q[ ch ][ 1 ] = sbr -> t_E[ ch ][  index             ];
  sbr -> t_Q[ ch ][ 2 ] = sbr -> t_E[ ch ][  sbr -> L_E[ ch ]  ];
 }  /* end else */
}  /* end noise_floor_time_border_vector */

static uint8_t sbr_grid ( SMS_BitContext* ld, sbr_info* sbr, uint8_t ch ) {
 uint8_t i, env, rel, result;
 uint8_t bs_abs_bord, bs_abs_bord_1;
 uint8_t bs_num_env        = 0;
 uint8_t saved_L_E         = sbr -> L_E[ ch ];
 uint8_t saved_L_Q         = sbr -> L_Q[ ch ];
 uint8_t saved_frame_class = sbr -> bs_frame_class[ ch ];
 sbr -> bs_frame_class[ ch ] = ( uint8_t )SMS_GetBits ( ld, 2 );
 switch ( sbr -> bs_frame_class[ ch ] ) {
  case FIXFIX:
   i          = ( uint8_t )SMS_GetBits ( ld, 2 );
   bs_num_env = 1 << i;
   bs_num_env = min( bs_num_env, 5 );
   i          = ( uint8_t )SMS_GetBit ( ld );
   for ( env = 0; env < bs_num_env; ++env ) sbr -> f[ ch ][ env ] = i;
   sbr -> abs_bord_lead [ ch ] = 0;
   sbr -> abs_bord_trail[ ch ] = sbr -> numTimeSlots;
   sbr -> n_rel_lead    [ ch ] = bs_num_env - 1;
   sbr -> n_rel_trail   [ ch ] = 0;
  break;
  case FIXVAR:
   bs_abs_bord = ( uint8_t )SMS_GetBits ( ld, 2 ) + sbr -> numTimeSlots;
   bs_num_env  = ( uint8_t )SMS_GetBits ( ld, 2 ) + 1;
   for ( rel = 0; rel < bs_num_env - 1; ++rel ) sbr -> bs_rel_bord[ ch ][ rel ] = 2 * ( uint8_t )SMS_GetBits ( ld, 2 ) + 2;
   i = sbr_log2 ( bs_num_env + 1 );
   sbr -> bs_pointer[ ch ] = ( uint8_t )SMS_GetBits ( ld, i );
   for ( env = 0; env < bs_num_env; ++env ) sbr -> f[ ch ][ bs_num_env - env - 1 ] = ( uint8_t )SMS_GetBit ( ld );
   sbr -> abs_bord_lead [ ch ] = 0;
   sbr -> abs_bord_trail[ ch ] = bs_abs_bord;
   sbr -> n_rel_lead    [ ch ] = 0;
   sbr -> n_rel_trail   [ ch ] = bs_num_env - 1;
  break;
  case VARFIX:
   bs_abs_bord = ( uint8_t )SMS_GetBits ( ld, 2 );
   bs_num_env  = ( uint8_t )SMS_GetBits ( ld, 2 ) + 1;
   for ( rel = 0; rel < bs_num_env - 1; ++rel ) sbr -> bs_rel_bord[ ch ][ rel ] = 2 * ( uint8_t )SMS_GetBits ( ld, 2 ) + 2;
   i = sbr_log2 ( bs_num_env + 1 );
   sbr -> bs_pointer[ ch ] = ( uint8_t )SMS_GetBits ( ld, i );
   for ( env = 0; env < bs_num_env; ++env ) sbr -> f[ ch ][ env ] = ( uint8_t )SMS_GetBit ( ld );
   sbr -> abs_bord_lead [ ch ] = bs_abs_bord;
   sbr -> abs_bord_trail[ ch ] = sbr -> numTimeSlots;
   sbr -> n_rel_lead    [ ch ] = bs_num_env - 1;
   sbr -> n_rel_trail   [ ch ] = 0;
  break;
  case VARVAR:
   bs_abs_bord   = ( uint8_t )SMS_GetBits ( ld, 2 );
   bs_abs_bord_1 = ( uint8_t )SMS_GetBits ( ld, 2 ) + sbr -> numTimeSlots;
   sbr -> bs_num_rel_0[ ch ] = ( uint8_t )SMS_GetBits ( ld, 2 );
   sbr -> bs_num_rel_1[ ch ] = ( uint8_t )SMS_GetBits ( ld, 2 );
   bs_num_env = sbr -> bs_num_rel_0[ ch ] + sbr -> bs_num_rel_1[ ch ] + 1;
   bs_num_env = min( 5, bs_num_env );
   for ( rel = 0; rel < sbr -> bs_num_rel_0[ ch ]; ++rel ) sbr -> bs_rel_bord_0[ ch ][ rel ] = 2 * ( uint8_t )SMS_GetBits ( ld, 2 ) + 2;
   for ( rel = 0; rel < sbr -> bs_num_rel_1[ ch ]; ++rel ) sbr -> bs_rel_bord_1[ ch ][ rel ] = 2 * ( uint8_t )SMS_GetBits ( ld, 2 ) + 2;
   i = sbr_log2 ( sbr -> bs_num_rel_0[ ch ] + sbr -> bs_num_rel_1[ ch ] + 2 );
   sbr -> bs_pointer[ ch ] = ( uint8_t )SMS_GetBits ( ld, i );
   for ( env = 0; env < bs_num_env; ++env ) sbr -> f[ ch ][ env ] = ( uint8_t )SMS_GetBit ( ld );
   sbr -> abs_bord_lead [ ch ] = bs_abs_bord;
   sbr -> abs_bord_trail[ ch ] = bs_abs_bord_1;
   sbr -> n_rel_lead    [ ch ] = sbr -> bs_num_rel_0[ ch ];
   sbr -> n_rel_trail   [ ch ] = sbr -> bs_num_rel_1[ ch ];
  break;
 } /* end switch */
 if ( sbr -> bs_frame_class[ch] == VARVAR )
  sbr -> L_E[ ch ] = min( bs_num_env, 5 );
 else sbr -> L_E[ ch ] = min( bs_num_env, 4 );
 if ( sbr -> L_E[ ch ] <= 0 ) return 1;
 if ( sbr -> L_E[ ch ] > 1 )
  sbr -> L_Q[ ch ] = 2;
 else sbr -> L_Q[ ch ] = 1;
 if (   (  result = envelope_time_border_vector ( sbr, ch )  ) > 0   ) {
  sbr -> bs_frame_class[ ch ] = saved_frame_class;
  sbr -> L_E[ ch ] = saved_L_E;
  sbr -> L_Q[ ch ] = saved_L_Q;
  return result;
 }  /* end if */
 noise_floor_time_border_vector ( sbr, ch );
 return 0;
}  /* end sbr_grid */

static void sbr_dtdf ( SMS_BitContext* ld, sbr_info* sbr, uint8_t ch ) {
 uint8_t i;
 for ( i = 0; i < sbr -> L_E[ ch ]; ++i ) sbr -> bs_df_env  [ ch ][ i ] = SMS_GetBit ( ld );
 for ( i = 0; i < sbr -> L_Q[ ch ]; ++i ) sbr -> bs_df_noise[ ch ][ i ] = SMS_GetBit ( ld );
}  /* end sbr_dtdf */

static void invf_mode ( SMS_BitContext *ld, sbr_info* sbr, uint8_t ch ) {
 uint8_t n;
 for ( n = 0; n < sbr -> N_Q; ++n ) sbr -> bs_invf_mode[ ch ][ n ] = ( uint8_t )SMS_GetBits ( ld, 2 );
}  /* end invf_mode */

static int16_t SMS_INLINE sbr_huff_dec ( SMS_BitContext* ld, sbr_huff_tab t_huff ) {
 uint8_t bit;
 int16_t index = 0;
 while ( index >= 0 ) {
  bit   = ( uint8_t )SMS_GetBit ( ld );
  index = t_huff[ index ][ bit ];
 }  /* end while */
 return index + 64;
}  /* end sbr_huff_dec */

static void extract_envelope_data ( sbr_info* sbr, uint8_t ch ) {
 uint8_t l, k;
 for ( l = 0; l < sbr -> L_E[ ch ]; ++l ) {
  if ( sbr -> bs_df_env[ ch ][ l ] == 0 ) {
   for ( k = 1; k < sbr -> n[  sbr -> f[ ch ][ l ]  ]; ++k ) {
    sbr -> E[ ch ][ k ][ l ] = sbr -> E[ ch ][ k - 1 ][ l ] + sbr -> E[ ch ][ k ][ l ];
    if ( sbr -> E[ ch ][ k ][ l ] < 0 ) sbr -> E[ ch ][ k ][ l ] = 0;
   }  /* end for */
  } else {
   uint8_t g = l == 0 ? sbr -> f_prev[ ch ] : sbr -> f[ ch ][ l - 1 ];
   int16_t E_prev;
   if ( sbr -> f[ ch ][ l ] == g ) {
    for ( k = 0; k < sbr -> n[  sbr -> f[ ch ][ l ]  ]; ++k ) {
     if ( l == 0 )
      E_prev = sbr -> E_prev[ ch ][ k ];
     else E_prev = sbr -> E[ ch ][ k ][ l - 1 ];
     sbr -> E[ ch ][ k ][ l ] = E_prev + sbr -> E[ ch ][ k ][ l ];
    }  /* end for */
   } else if ( g == 1 && sbr -> f[ ch ][ l ] == 0 ) {
    uint8_t i;
    for ( k = 0; k < sbr -> n[  sbr -> f[ ch ][ l ]  ]; ++k ) {
     for ( i = 0; i < sbr -> N_high; ++i ) {
      if ( sbr -> f_table_res[ HI_RES ][ i ] == sbr -> f_table_res[ LO_RES ][ k ] ) {
       if ( l == 0 )
        E_prev = sbr -> E_prev[ ch ][ i ];
       else E_prev = sbr -> E[ ch ][ i ][ l - 1 ];
       sbr -> E[ ch ][ k ][ l ] = E_prev + sbr -> E[ ch ][ k ][ l ];
      }  /* end if */
     }  /* end for */
    }  /* end for */
   } else if ( g == 0 && sbr -> f[ ch ][ l ] == 1 ) {
    uint8_t i;
    for ( k = 0; k < sbr -> n[  sbr -> f[ ch ][ l ]  ]; ++k ) {
     for ( i = 0; i < sbr -> N_low; ++i ) {
      if ( sbr -> f_table_res[ LO_RES ][ i ] <= sbr -> f_table_res[ HI_RES ][ k     ] &&
           sbr -> f_table_res[ HI_RES ][ k ] <  sbr -> f_table_res[ LO_RES ][ i + 1 ]
      ) {
       if ( l == 0 )
        E_prev = sbr -> E_prev[ ch ][ i ];
       else E_prev = sbr -> E[ ch ][ i ][ l - 1 ];
       sbr -> E[ ch ][ k ][ l ] = E_prev + sbr -> E[ ch ][ k ][ l ];
      }  /* end if */
     }  /* end for */
    }  /* end for */
   }  /* end if */
  }  /* end else */
 }  /* end for */
}  /* end extract_envelope_data */

static void sbr_envelope ( SMS_BitContext* ld, sbr_info* sbr, uint8_t ch ) {
 uint8_t      env, band;
 int8_t       delta = 0;
 sbr_huff_tab t_huff, f_huff;
 if ( sbr -> L_E[ ch ] == 1 && sbr -> bs_frame_class[ ch ] == FIXFIX )
  sbr -> amp_res[ ch ] = 0;
 else sbr -> amp_res[ ch ] = sbr -> bs_amp_res;
 if ( sbr -> bs_coupling && ch == 1 ) {
  delta = 1;
  if ( sbr -> amp_res[ ch ] ) {
   t_huff = t_huffman_env_bal_3_0dB;
   f_huff = f_huffman_env_bal_3_0dB;
  } else {
   t_huff = t_huffman_env_bal_1_5dB;
   f_huff = f_huffman_env_bal_1_5dB;
  }  /* end else */
 } else {
  delta = 0;
  if ( sbr -> amp_res[ ch ] ) {
   t_huff = t_huffman_env_3_0dB;
   f_huff = f_huffman_env_3_0dB;
  } else {
   t_huff = t_huffman_env_1_5dB;
   f_huff = f_huffman_env_1_5dB;
  }  /* end else */
 }  /* end else */
 for ( env = 0; env < sbr -> L_E[ ch ]; ++env ) {
  if ( sbr -> bs_df_env[ ch ][ env ] == 0 ) {
   if ( sbr -> bs_coupling == 1 && ch == 1 ) {
    if ( sbr -> amp_res[ ch ] )
     sbr -> E[ ch ][ 0 ][ env ] = ( uint16_t )(  SMS_GetBits ( ld, 5 ) << delta  );
    else sbr -> E[ ch ][ 0 ][ env ] = ( uint16_t )(  SMS_GetBits ( ld, 6 ) << delta  );
   } else {
    if ( sbr -> amp_res[ ch ] )
     sbr -> E[ ch ][ 0 ][ env ] = ( uint16_t )(  SMS_GetBits ( ld, 6 ) << delta  );
    else sbr -> E[ ch ][ 0 ][ env ] = ( uint16_t )(  SMS_GetBits ( ld, 7 ) << delta  );
   }  /* end else */
   for ( band = 1; band < sbr -> n[  sbr -> f[ ch ][ env ]  ]; ++band )
    sbr -> E[ ch ][ band ][ env ] = sbr_huff_dec ( ld, f_huff ) << delta;
  } else for ( band = 0; band < sbr -> n[  sbr -> f[ ch ][ env ]  ]; ++band ) sbr -> E[ ch ][ band ][ env ] = sbr_huff_dec ( ld, t_huff ) << delta;
 }  /* end for */
 extract_envelope_data ( sbr, ch );
}  /* end sbr_envelope */

static void extract_noise_floor_data ( sbr_info* sbr, uint8_t ch ) {
 uint8_t l, k;
 for ( l = 0; l < sbr -> L_Q[ ch ]; ++l ) {
  if ( sbr -> bs_df_noise[ ch ][ l ] == 0 ) {
   for ( k = 1; k < sbr -> N_Q; ++k ) sbr -> Q[ ch ][ k ][ l ] = sbr -> Q[ ch ][ k ][ l ] + sbr -> Q[ ch ][ k - 1 ][ l ];
  } else {
   if ( l == 0 )
    for ( k = 0; k < sbr -> N_Q; ++k ) sbr -> Q[ ch ][ k ][ l ] = sbr -> Q_prev[ ch ][ k ] + sbr -> Q[ ch ][ k ][ 0 ];
   else for ( k = 0; k < sbr -> N_Q; ++k ) sbr -> Q[ ch ][ k ][ l ] = sbr -> Q[ ch ][ k ][ l - 1] + sbr -> Q[ ch ][ k ][ l ];
  }  /* end else */
 }  /* end for */
}  /* end extract_noise_floor_data */

static void sbr_noise ( SMS_BitContext* ld, sbr_info* sbr, uint8_t ch ) {
 uint8_t      noise, band;
 int8_t       delta = 0;
 sbr_huff_tab t_huff, f_huff;
 if ( sbr -> bs_coupling == 1 && ch == 1 ) {
  delta  = 1;
  t_huff = t_huffman_noise_bal_3_0dB;
  f_huff = f_huffman_env_bal_3_0dB;
 } else {
  delta = 0;
  t_huff = t_huffman_noise_3_0dB;
  f_huff = f_huffman_env_3_0dB;
 }  /* end else */
 for ( noise = 0; noise < sbr -> L_Q[ ch ]; ++noise ) {
  if ( sbr -> bs_df_noise[ ch ][ noise ] == 0 ) {
   if ( sbr -> bs_coupling == 1 && ch == 1 )
    sbr -> Q[ ch ][ 0 ][ noise ] = SMS_GetBits ( ld, 5 ) << delta;
   else sbr -> Q[ ch ][ 0 ][ noise ] = SMS_GetBits ( ld, 5 ) << delta;
   for ( band = 1; band < sbr -> N_Q; ++band ) sbr -> Q[ ch ][ band ][ noise ] = sbr_huff_dec ( ld, f_huff ) << delta;
  } else for ( band = 0; band < sbr -> N_Q; ++band ) sbr -> Q[ ch ][ band ][ noise ] = sbr_huff_dec ( ld, t_huff ) << delta;
 }  /* end for */
 extract_noise_floor_data ( sbr, ch );
}  /* end sbr_noise */

static float calc_Q_div ( sbr_info* sbr, uint8_t ch, uint8_t m, uint8_t l ) {
 if ( sbr -> bs_coupling ) {
  if ( sbr -> Q[ 0 ][ m ][ l ] < 0 || sbr -> Q[ 0 ][ m ][ l ] > 30 ||
       sbr -> Q[ 1 ][ m ][ l ] < 0 || sbr -> Q[ 1 ][ m ][ l ] > 24
  ) {
   return 0.0F;
  } else {
   if ( ch == 0 )
    return Q_div_tab_left[  sbr -> Q[ 0 ][ m ][ l ]  ][ sbr -> Q[ 1 ][ m ][ l ] >> 1  ];
   else return Q_div_tab_right[  sbr -> Q[ 0 ][ m ][ l ]  ][  sbr -> Q[ 1 ][ m ][ l ] >> 1  ];
  }  /* end else */
 } else {
  if ( sbr -> Q[ ch ][ m ][ l ] < 0 || sbr -> Q[ ch ][ m ][ l ] > 30 )
   return 0;
  else return Q_div_tab[  sbr -> Q[ ch ][ m ][ l ]  ];
 }  /* end else */
}  /* end calc_Q_div */

static float calc_Q_div2 ( sbr_info* sbr, uint8_t ch, uint8_t m, uint8_t l ) {
 if ( sbr -> bs_coupling ) {
  if ( sbr -> Q[ 0 ][ m ][ l ] < 0 || sbr -> Q[ 0 ][ m ][ l ] > 30 ||
       sbr -> Q[ 1 ][ m ][ l ] < 0 || sbr -> Q[ 1 ][ m ][ l ] > 24
  ) {
   return 0;
  } else {
   if ( ch == 0 )
    return Q_div2_tab_left[  sbr -> Q[ 0 ][ m ][ l ]  ][  sbr -> Q[ 1 ][ m ][ l ] >> 1  ];
   else return Q_div2_tab_right[  sbr -> Q[ 0 ][ m ][ l ]  ][  sbr -> Q[ 1 ][ m ][ l ] >> 1  ];
  }
 } else {
  if ( sbr -> Q[ ch ][ m ][ l ] < 0 || sbr -> Q[ ch ][ m ][ l ] > 30 )
   return 0;
  else return Q_div2_tab[  sbr -> Q[ ch ][ m ][ l ]  ];
 }  /* end else */
}  /* end calc_Q_div2 */

static void envelope_noise_dequantisation ( sbr_info* sbr, uint8_t ch ) {
 if ( sbr -> bs_coupling == 0 ) {
  int16_t exp;
  uint8_t l, k;
  uint8_t amp = sbr -> amp_res[ ch ] ? 0 : 1;
  for ( l = 0; l < sbr -> L_E[ ch ]; ++l ) {
   for ( k = 0; k < sbr -> n[  sbr -> f[ ch ][ l ]  ]; ++k ) {
    exp = sbr -> E[ ch ][ k ][ l ] >> amp;
    if ( exp < 0 || exp >= 64 )
     sbr -> E_orig[ ch ][ k ][ l ] = 0;
    else {
     sbr -> E_orig[ ch ][ k ][ l ] = E_deq_tab[ exp ];
     if (  amp && ( sbr -> E[ ch ][ k ][ l ] & 1 )  ) sbr -> E_orig[ ch ][ k ][ l ] = sbr -> E_orig[ ch ][ k ][ l ] * 1.414213562F;
    }  /* end else */
   }  /* end for */
  }  /* end for */
  for ( l = 0; l < sbr -> L_Q[ ch ]; ++l ) {
   for ( k = 0; k < sbr -> N_Q; ++k ) {
    sbr -> Q_div [ ch ][ k ][ l ] = calc_Q_div  ( sbr, ch, k, l );
    sbr -> Q_div2[ ch ][ k ][ l ] = calc_Q_div2 ( sbr, ch, k, l );
   }  /* end for */
  }  /* end for */
 }  /* end if */
}  /* end envelope_noise_dequantisation */

static void sinusoidal_coding ( SMS_BitContext* ld, sbr_info* sbr, uint8_t ch ) {
 uint8_t n;
 for ( n = 0; n < sbr -> N_high; ++n )
  sbr -> bs_add_harmonic[ ch ][ n ] = SMS_GetBit ( ld );
}  /* end sinusoidal_coding */

static uint16_t sbr_extension ( SMS_BitContext* ld, sbr_info* sbr ) {
 sbr -> bs_extension_data = ( uint8_t )SMS_GetBits ( ld, 6 );
 return 6;
}  /* end sbr_extension */

static uint8_t sbr_single_channel_element ( SMS_BitContext* ld, sbr_info* sbr ) {
 uint8_t result;
 if (  SMS_GetBit ( ld )  ) SMS_SkipBits ( ld, 4 );
 if (   (  result = sbr_grid ( ld, sbr, 0 )  ) > 0   ) return result;
 sbr_dtdf ( ld, sbr, 0 );
 invf_mode ( ld, sbr, 0 );
 sbr_envelope ( ld, sbr, 0 );
 sbr_noise ( ld, sbr, 0 );
 envelope_noise_dequantisation ( sbr, 0 );
 memset (  sbr -> bs_add_harmonic[ 0 ], 0, 64 * sizeof ( uint8_t )  );
 sbr -> bs_add_harmonic_flag[ 0 ] = SMS_GetBit ( ld );
 if ( sbr -> bs_add_harmonic_flag[ 0 ] ) sinusoidal_coding ( ld, sbr, 0 );
 sbr -> bs_extended_data = SMS_GetBit ( ld );
 if ( sbr -> bs_extended_data ) {
  uint16_t nr_bits_left;
  uint16_t cnt = ( uint16_t )SMS_GetBits ( ld, 4 );
  if ( cnt == 15 ) cnt += ( uint16_t )SMS_GetBits ( ld, 8 );
  nr_bits_left = 8 * cnt;
  while ( nr_bits_left > 7 ) {
   uint16_t tmp_nr_bits = 0;
   sbr -> bs_extension_id = ( uint8_t )SMS_GetBits ( ld, 2 );
   tmp_nr_bits += 2;
   tmp_nr_bits += sbr_extension ( ld, sbr );
   if ( tmp_nr_bits > nr_bits_left ) return 1;
   nr_bits_left -= tmp_nr_bits;
  }  /* end while */
  if ( nr_bits_left > 0 ) SMS_GetBits ( ld, nr_bits_left );
 }  /* end if */
 return 0;
}  /* end sbr_single_channel_element */

static void unmap_envelope_noise ( sbr_info* sbr ) {
 float   tmp;
 int16_t exp0, exp1;
 uint8_t l, k;
 uint8_t amp0 = sbr -> amp_res[ 0 ] ? 0 : 1;
 uint8_t amp1 = sbr -> amp_res[ 1 ] ? 0 : 1;
 for ( l = 0; l < sbr -> L_E[ 0 ]; ++l ) {
  for ( k = 0; k < sbr -> n[  sbr -> f[ 0 ][ l ]  ]; ++k ) {
   exp0 = ( sbr -> E[ 0 ][ k ][ l ] >> amp0 ) + 1;
   exp1 = ( sbr -> E[ 1 ][ k ][ l ] >> amp1 );
   if ( exp0 < 0 || exp0 >= 64 || exp1 < 0 || exp1 > 24 ) {
    sbr -> E_orig[ 1 ][ k][ l ] = 0;
    sbr -> E_orig[ 0 ][ k][ l ] = 0;
   } else {
    tmp = E_deq_tab[ exp0 ];
    if (  amp0 && ( sbr -> E[ 0 ][ k ][ l ] & 1 )  ) tmp *= 1.414213562F;
    sbr -> E_orig[ 0 ][ k ][ l ] = tmp * E_pan_tab[ exp1      ];
    sbr -> E_orig[ 1 ][ k ][ l ] = tmp * E_pan_tab[ 24 - exp1 ];
   }  /* end else */
  }  /* end for */
 }  /* end for */
 for ( l = 0; l < sbr -> L_Q[ 0 ]; ++l ) {
  for ( k = 0; k < sbr -> N_Q; ++k ) {
   sbr -> Q_div [ 0 ][ k ][ l ] = calc_Q_div  ( sbr, 0, k, l );
   sbr -> Q_div [ 1 ][ k ][ l ] = calc_Q_div  ( sbr, 1, k, l );
   sbr -> Q_div2[ 0 ][ k ][ l ] = calc_Q_div2 ( sbr, 0, k, l );
   sbr -> Q_div2[ 1 ][ k ][ l ] = calc_Q_div2 ( sbr, 1, k, l );
  }  /* end for */
 }  /* end for */
}  /* end unmap_envelope_noise */

static uint8_t calc_sbr_tables (
                sbr_info* sbr, uint8_t start_freq, uint8_t stop_freq,
                uint8_t samplerate_mode, uint8_t freq_scale,
                uint8_t alter_scale, uint8_t xover_band
               ) {
 uint8_t result = 0;
 uint8_t k2;
 sbr -> k0 = qmf_start_channel ( start_freq, samplerate_mode,    sbr -> sample_rate );
 k2        = qmf_stop_channel  ( stop_freq,  sbr -> sample_rate, sbr -> k0          );
 if ( sbr -> sample_rate >= 48000 ) {
  if (  ( k2 - sbr -> k0 ) > 32  ) result += 1;
 } else if ( sbr -> sample_rate <= 32000 ) {
  if (  ( k2 - sbr -> k0 ) > 48  ) result += 1;
 } else {
  if (  ( k2 - sbr -> k0 ) > 45  ) result += 1;
 }  /* end else */
 if ( freq_scale == 0 ) {
  result += master_frequency_table_fs0 ( sbr, sbr -> k0, k2, alter_scale );
 } else {
  result += master_frequency_table ( sbr, sbr -> k0, k2, freq_scale, alter_scale );
 }  /* end else */
 result += derived_frequency_table ( sbr, xover_band, k2 );
 return result > 0;
}  /* end calc_sbr_tables */

static uint8_t sbr_channel_pair_element ( SMS_BitContext* ld, sbr_info* sbr ) {
 uint8_t n, result;
 if (  SMS_GetBit ( ld )  ) SMS_SkipBits ( ld, 8 );
 sbr -> bs_coupling = SMS_GetBit ( ld );
 if ( sbr -> bs_coupling ) {
  if (   (  result = sbr_grid ( ld, sbr, 0 )  ) > 0   ) return result;
  sbr -> bs_frame_class[ 1 ] = sbr -> bs_frame_class[ 0 ];
  sbr -> L_E           [ 1 ] = sbr -> L_E           [ 0 ];
  sbr -> L_Q           [ 1 ] = sbr -> L_Q           [ 0 ];
  sbr -> bs_pointer    [ 1 ] = sbr -> bs_pointer    [ 0 ];
  for ( n = 0; n <= sbr -> L_E[ 0 ]; ++n ) {
   sbr -> t_E[ 1 ][ n ] = sbr -> t_E[ 0 ][ n ];
   sbr -> f  [ 1 ][ n ] = sbr -> f  [ 0 ][ n ];
  }  /* end for */
  for ( n = 0; n <= sbr -> L_Q[ 0 ]; ++n ) sbr -> t_Q[ 1 ][ n ] = sbr -> t_Q[ 0 ][ n ];
  sbr_dtdf ( ld, sbr, 0 );
  sbr_dtdf ( ld, sbr, 1 );
  invf_mode ( ld, sbr, 0 );
  for ( n = 0; n < sbr -> N_Q; ++n ) sbr -> bs_invf_mode[ 1 ][ n ] = sbr -> bs_invf_mode[ 0 ][ n ];
  sbr_envelope ( ld, sbr, 0 );
  sbr_noise ( ld, sbr, 0 );
  sbr_envelope ( ld, sbr, 1 );
  sbr_noise ( ld, sbr, 1 );
  memset (  sbr -> bs_add_harmonic[ 0 ], 0, 64 * sizeof ( uint8_t )  );
  memset (  sbr -> bs_add_harmonic[ 1 ], 0, 64 * sizeof ( uint8_t )  );
  sbr -> bs_add_harmonic_flag[ 0 ] = SMS_GetBit ( ld );
  if ( sbr -> bs_add_harmonic_flag[ 0 ] ) sinusoidal_coding ( ld, sbr, 0 );
  sbr -> bs_add_harmonic_flag[ 1 ] = SMS_GetBit ( ld );
  if ( sbr -> bs_add_harmonic_flag[ 1 ] ) sinusoidal_coding ( ld, sbr, 1 );
 } else {
  uint8_t saved_t_E[ 6 ]    = { 0 }, saved_t_Q[ 3 ] = { 0 };
  uint8_t saved_L_E         = sbr -> L_E[ 0 ];
  uint8_t saved_L_Q         = sbr -> L_Q[ 0 ];
  uint8_t saved_frame_class = sbr -> bs_frame_class[ 0 ];
  for ( n = 0; n < saved_L_E; ++n ) saved_t_E[ n ] = sbr -> t_E[ 0 ][ n ];
  for ( n = 0; n < saved_L_Q; ++n ) saved_t_Q[ n ] = sbr -> t_Q[ 0 ][ n ];
  if (   (  result = sbr_grid ( ld, sbr, 0 )  ) > 0   ) return result;
  if (   (  result = sbr_grid ( ld, sbr, 1 )  ) > 0   ) {
   sbr -> bs_frame_class[ 0 ] = saved_frame_class;
   sbr -> L_E           [ 0 ] = saved_L_E;
   sbr -> L_Q           [ 0 ] = saved_L_Q;
   for ( n = 0; n < 6; ++n ) sbr -> t_E[ 0 ][ n ] = saved_t_E[ n ];
   for ( n = 0; n < 3; ++n ) sbr -> t_Q[ 0 ][ n ] = saved_t_Q[ n ];
   return result;
  }  /* end if */
  sbr_dtdf ( ld, sbr, 0 );
  sbr_dtdf ( ld, sbr, 1 );
  invf_mode ( ld, sbr, 0 );
  invf_mode ( ld, sbr, 1 );
  sbr_envelope ( ld, sbr, 0 );
  sbr_envelope ( ld, sbr, 1 );
  sbr_noise ( ld, sbr, 0 );
  sbr_noise ( ld, sbr, 1 );
  memset (  sbr -> bs_add_harmonic[ 0 ], 0, 64 * sizeof ( uint8_t )  );
  memset (  sbr -> bs_add_harmonic[ 1 ], 0, 64 * sizeof ( uint8_t )  );
  sbr -> bs_add_harmonic_flag[ 0 ] = SMS_GetBit ( ld );
  if ( sbr -> bs_add_harmonic_flag[ 0 ] ) sinusoidal_coding ( ld, sbr, 0 );
  sbr -> bs_add_harmonic_flag[ 1 ] = SMS_GetBit ( ld );
  if ( sbr -> bs_add_harmonic_flag[ 1 ] ) sinusoidal_coding ( ld, sbr, 1 );
 }  /* end else */
 envelope_noise_dequantisation ( sbr, 0 );
 envelope_noise_dequantisation ( sbr, 1 );
 if ( sbr -> bs_coupling ) unmap_envelope_noise ( sbr );
 sbr -> bs_extended_data = SMS_GetBit ( ld );
 if ( sbr -> bs_extended_data ) {
  uint16_t nr_bits_left;
  uint16_t cnt = ( uint16_t )SMS_GetBits ( ld, 4 );
  if ( cnt == 15 ) cnt += ( uint16_t )SMS_GetBits ( ld, 8 );
  nr_bits_left = 8 * cnt;
  while ( nr_bits_left > 7 ) {
   uint16_t tmp_nr_bits = 0;
   sbr -> bs_extension_id = ( uint8_t )SMS_GetBits ( ld, 2 );
   tmp_nr_bits += 2;
   tmp_nr_bits += sbr_extension ( ld, sbr );
   if ( tmp_nr_bits > nr_bits_left ) return 1;
   nr_bits_left -= tmp_nr_bits;
  }  /* end while */
  if ( nr_bits_left > 0 ) SMS_GetBits ( ld, nr_bits_left );
 }  /* end if */
 return 0;
}  /* end sbr_channel_pair_element */

static uint8_t sbr_data ( SMS_BitContext* ld, sbr_info* sbr ) {
 uint8_t result;
 sbr -> rate = ( sbr -> bs_samplerate_mode ) ? 2 : 1;
 switch ( sbr -> id_aac ) {
  case ID_SCE:
   if (   (  result = sbr_single_channel_element ( ld, sbr )  ) > 0   ) return result;
  break;
  case ID_CPE:
   if (   (  result = sbr_channel_pair_element   ( ld, sbr )  ) > 0   ) return result;
  break;
 }  /* end switch */
 return 0;
}  /* end sbr_data */

uint8_t sbr_extension_data ( SMS_BitContext* ld, sbr_info* sbr, uint16_t cnt, uint8_t psResetFlag ) {

 uint8_t  result         = 0;
 uint16_t num_align_bits = 0;
 uint16_t num_sbr_bits1  = ( uint16_t )ld -> m_Idx;
 uint16_t num_sbr_bits2;
 uint8_t  saved_start_freq, saved_samplerate_mode;
 uint8_t  saved_stop_freq, saved_freq_scale;
 uint8_t  saved_alter_scale, saved_xover_band;
 uint8_t  bs_extension_type = ( uint8_t )SMS_GetBits ( ld, 4 );

 if ( bs_extension_type == EXT_SBR_DATA_CRC ) sbr -> bs_sbr_crc_bits = ( uint16_t )SMS_GetBits ( ld, 10 );

 saved_start_freq      = sbr -> bs_start_freq;
 saved_samplerate_mode = sbr -> bs_samplerate_mode;
 saved_stop_freq       = sbr -> bs_stop_freq;
 saved_freq_scale      = sbr -> bs_freq_scale;
 saved_alter_scale     = sbr -> bs_alter_scale;
 saved_xover_band      = sbr -> bs_xover_band;

 sbr -> bs_header_flag = SMS_GetBit ( ld );

 if ( sbr -> bs_header_flag ) sbr_header ( ld, sbr );

 sbr_reset ( sbr );

 if ( sbr -> header_count ) {
  if ( sbr -> Reset || ( sbr -> bs_header_flag && sbr -> just_seeked )  ) {
   uint8_t rt = calc_sbr_tables (
    sbr, sbr -> bs_start_freq, sbr -> bs_stop_freq,
    sbr -> bs_samplerate_mode, sbr -> bs_freq_scale,
    sbr -> bs_alter_scale, sbr -> bs_xover_band
   );
   if ( rt > 0 ) calc_sbr_tables (
    sbr, saved_start_freq, saved_stop_freq,
    saved_samplerate_mode, saved_freq_scale,
    saved_alter_scale, saved_xover_band
   );
  }  /* end if */

  if ( !result ) {
   result = sbr_data ( ld, sbr );
   if (   ( result > 0 ) && (  sbr -> Reset || ( sbr -> bs_header_flag && sbr -> just_seeked )  )   ) calc_sbr_tables (
    sbr, saved_start_freq, saved_stop_freq,
    saved_samplerate_mode, saved_freq_scale,
    saved_alter_scale, saved_xover_band
   );
  }  /* end if */
 } else result = 1;

 num_sbr_bits2 = ( uint16_t )ld -> m_Idx - num_sbr_bits1;

 if ( 8 * cnt < num_sbr_bits2 ) {
  ld -> m_Idx   = num_sbr_bits1 + 8 * cnt;
  return 1;
 }  /* end if */

 num_align_bits = 8 * cnt - num_sbr_bits2;

 while ( num_align_bits > 7 ) {
  SMS_GetBits ( ld, 8 );
  num_align_bits -= 8;
 }  /* end while */

 SMS_GetBits ( ld, num_align_bits );

 return result;

}  /* end sbr_extension_data */

static void sbr_save_matrix ( sbr_info* sbr, uint8_t ch ) {
 uint8_t i;
 for ( i = 0; i < sbr -> tHFGen; ++i )
  memmove (  sbr -> Xsbr[ ch ][ i ], sbr -> Xsbr[ ch ][ i + sbr -> numTimeSlotsRate ], 64 * sizeof ( qmf_t )  );
 for ( i = sbr -> tHFGen; i < MAX_NTSRHFG; ++i )
  memset (  sbr -> Xsbr[ ch ][ i ], 0, 64 * sizeof ( qmf_t )  );
}  /* end sbr_save_matrix */

static uint8_t sbr_save_prev_data ( sbr_info* sbr, uint8_t ch ) {
 uint8_t i;
 sbr -> kx_prev   = sbr -> kx;
 sbr -> M_prev    = sbr -> M;
 sbr -> bsco_prev = sbr -> bsco;
 sbr -> L_E_prev[ ch ] = sbr -> L_E[ ch ];
 if ( sbr -> L_E[ ch ] <= 0 ) return 19;
 sbr -> f_prev[ ch ] = sbr -> f[ ch ][  sbr -> L_E[ ch ] - 1  ];
 for ( i = 0; i < MAX_M; ++i ) {
  sbr -> E_prev[ ch ][ i ] = sbr -> E[ ch ][ i ][  sbr -> L_E[ ch ] - 1  ];
  sbr -> Q_prev[ ch ][ i ] = sbr -> Q[ ch ][ i ][  sbr -> L_Q[ ch ] - 1  ];
 }  /* end for */
 for ( i = 0; i < MAX_M; ++i ) sbr -> bs_add_harmonic_prev[ ch ][ i ] = sbr -> bs_add_harmonic[ ch ][ i ];
 sbr -> bs_add_harmonic_flag_prev[ ch ] = sbr -> bs_add_harmonic_flag[ ch ];
 if ( sbr -> l_A[ ch ] == sbr -> L_E[ ch ] )
  sbr -> prevEnvIsShort[ ch ] = 0;
 else sbr -> prevEnvIsShort[ ch ] = -1;
 return 0;
}  /* end sbr_save_prev_data */

static uint8_t estimate_current_envelope (
                sbr_info* sbr, qmf_t Xsbr[ MAX_NTSRHFG ][ 64 ], uint8_t ch
               ) {

 uint8_t m, l, j, k, k_l, k_h, p;
 float   nrg, div;

 if ( sbr -> bs_interpol_freq == 1 ) {

  for ( l = 0; l < sbr -> L_E[ ch ]; ++l ) {

   uint8_t i, l_i, u_i;

   l_i = sbr -> t_E[ ch ][ l + 0 ];
   u_i = sbr -> t_E[ ch ][ l + 1 ];
   div = ( float )( u_i - l_i );

   if ( div == 0.0F ) div = 1.0F;

   for ( m = 0; m < sbr -> M; ++m ) {

    nrg = 0;

    for ( i = l_i + sbr -> tHFAdj; i < u_i + sbr -> tHFAdj; ++i )
     nrg += Xsbr[ i ][ m + sbr -> kx][ 0 ] * Xsbr[ i ][ m + sbr -> kx ][ 0 ] +
            Xsbr[ i ][ m + sbr -> kx][ 1 ] * Xsbr[ i ][ m + sbr -> kx ][ 1 ];

    sbr -> E_curr[ ch ][ m ][ l ] = nrg / div;

   }  /* end for */

  }  /* end for */

 } else {

  for ( l = 0; l < sbr -> L_E[ ch ]; ++l ) {

   for ( p = 0; p < sbr -> n[  sbr -> f[ ch ][ l ]  ]; ++p ) {

    k_l = sbr -> f_table_res[  sbr -> f[ ch ][ l ]  ][ p + 0 ];
    k_h = sbr -> f_table_res[  sbr -> f[ ch ][ l ]  ][ p + 1 ];

    for ( k = k_l; k < k_h; ++k ) {

     uint8_t i, l_i, u_i;
     nrg = 0;

     l_i = sbr -> t_E[ ch ][ l + 0 ];
     u_i = sbr -> t_E[ ch ][ l + 1 ];

     div = ( float )(  ( u_i - l_i ) * ( k_h - k_l )  );

     if ( div == 0.0F ) div = 1.0F;

     for ( i = l_i + sbr -> tHFAdj; i < u_i + sbr -> tHFAdj; ++i ) {

      for ( j = k_l; j < k_h; ++j )
       nrg += Xsbr[ i ][ j ][ 0 ] * Xsbr[ i ][ j ][ 0 ] + 
              Xsbr[ i ][ j ][ 1 ] * Xsbr[ i ][ j ][ 1 ];

     }  /* end for */

     sbr -> E_curr[ ch ][ k - sbr -> kx ][ l ] = nrg / div;

    }  /* end for */

   }  /* end for */

  }  /* end for */

 }  /* end else */

 return 0;

}  /* end estimate_current_envelope */

static uint8_t get_S_mapped ( sbr_info* sbr, uint8_t ch, uint8_t l, uint8_t current_band ) {
 if ( sbr -> f[ ch ][ l ] == HI_RES ) {
  if (  l >= sbr -> l_A[ ch ] || ( sbr -> bs_add_harmonic_prev[ ch ][ current_band ] && sbr -> bs_add_harmonic_flag_prev[ ch ] )  ) return sbr -> bs_add_harmonic[ ch ][ current_band ];
 } else {
  uint8_t b, lb, ub;
  lb = 2 * ( current_band + 0 ) - ( sbr -> N_high & 1 );
  ub = 2 * ( current_band + 1 ) - ( sbr -> N_high & 1 );
  for ( b = lb; b < ub; ++b ) {
   if (  l >= sbr -> l_A[ ch ] || ( sbr -> bs_add_harmonic_prev[ ch ][ b ] && sbr -> bs_add_harmonic_flag_prev[ ch ] )  ) {
    if ( sbr -> bs_add_harmonic[ ch ][ b ] == 1 ) return 1;
   }  /* end if */
  }  /* end for */
 }  /* end else */
 return 0;
}  /* end get_S_mapped */

static void calculate_gain ( sbr_info* sbr, uint8_t ch ) {

 static float limGain[] = { 0.5F, 1.0F, 2.0F, 1E10F };

 uint8_t m, l, k;
 uint8_t current_t_noise_band = 0;
 uint8_t S_mapped;

 float Q_M_lim[ MAX_M ];
 float G_lim  [ MAX_M ];
 float S_M    [ MAX_M ];
 float G_boost;

 sbr_hfadj_info* adj = &sbr -> adj;

 for ( l = 0; l < sbr -> L_E[ ch ]; ++l ) {

  uint8_t current_f_noise_band = 0;
  uint8_t current_res_band     = 0;
  uint8_t current_res_band2    = 0;
  uint8_t current_hi_res_band  = 0;
  float   delta = ( float )!( l == sbr -> l_A[ ch ] || l == sbr -> prevEnvIsShort[ ch ] );

  S_mapped = get_S_mapped ( sbr, ch, l, current_res_band2 );

  if ( sbr -> t_E[ ch ][ l + 1 ] > sbr -> t_Q[ ch ][ current_t_noise_band + 1 ] ) ++current_t_noise_band;

  for ( k = 0; k < sbr -> N_L[ sbr -> bs_limiter_bands ]; ++k ) {

   float   G_max;
   float   den  = 0;
   float   acc1 = 0;
   float   acc2 = 0;
   uint8_t ml1, ml2;

   ml1 = sbr -> f_table_lim[ sbr -> bs_limiter_bands ][ k + 0 ];
   ml2 = sbr -> f_table_lim[ sbr -> bs_limiter_bands ][ k + 1 ];

   for ( m = ml1; m < ml2; ++m ) {
    if ( m + sbr -> kx == sbr -> f_table_res[  sbr -> f[ ch ][ l ]  ][ current_res_band + 1 ] ) ++current_res_band;
    acc1 += sbr -> E_orig[ ch ][ current_res_band ][ l ];
    acc2 += sbr -> E_curr[ ch ][ m ][ l ];
   }  /* end for */

   G_max = (  ( EPS + acc1 ) / ( EPS + acc2 )  ) * limGain[ sbr -> bs_limiter_gains ];
   G_max = min( G_max, 1E10F );

   for ( m = ml1; m < ml2; ++m ) {
    float   Q_M, G;
    float   Q_div, Q_div2;
    uint8_t S_index_mapped;
    if (  m + sbr -> kx == sbr -> f_table_noise[ current_f_noise_band + 1 ] ) ++current_f_noise_band;
    if (  m + sbr -> kx == sbr -> f_table_res[  sbr -> f[ ch ][ l ]  ][ current_res_band2 + 1 ] ) {
     ++current_res_band2;
     S_mapped = get_S_mapped ( sbr, ch, l, current_res_band2 );
    }  /* end if */
    if ( m + sbr -> kx == sbr -> f_table_res[ HI_RES ][ current_hi_res_band + 1 ] ) ++current_hi_res_band;
    S_index_mapped = 0;
    if (  l >= sbr -> l_A[ ch ] || ( sbr -> bs_add_harmonic_prev[ ch ][ current_hi_res_band ] && sbr -> bs_add_harmonic_flag_prev[ ch ] )  ) {
     if ( m + sbr -> kx == ( sbr -> f_table_res[ HI_RES ][ current_hi_res_band + 1 ] + sbr -> f_table_res[ HI_RES ][ current_hi_res_band ] ) >> 1 ) S_index_mapped = sbr -> bs_add_harmonic[ ch ][ current_hi_res_band ];
    }  /* end if */
    Q_div  = sbr -> Q_div [ ch ][ current_f_noise_band ][ current_t_noise_band ];
    Q_div2 = sbr -> Q_div2[ ch ][ current_f_noise_band ][ current_t_noise_band ];
    Q_M    = sbr -> E_orig[ ch ][ current_res_band2 ][ l ] * Q_div2;
    if ( S_index_mapped == 0 )
     S_M[ m ] = 0;
    else {
     S_M[ m ] = sbr -> E_orig[ ch ][ current_res_band2 ][ l ] * Q_div;
     den += S_M[ m ];
    }  /* end else */
    G = sbr -> E_orig[ ch ][ current_res_band2 ][ l ] / ( 1.0F + sbr -> E_curr[ ch ][ m ][ l ] );
    if ( S_mapped == 0 && delta == 1)
     G *= Q_div;
    else if ( S_mapped == 1 ) G *= Q_div2;
    if ( G_max > G ) {
     Q_M_lim[ m ] = Q_M;
     G_lim  [ m ] = G;
    } else {
     Q_M_lim[ m ] = Q_M * G_max / G;
     G_lim  [ m ] = G_max;
    }  /* end else */
    den += sbr -> E_curr[ ch ][ m ][ l ] * G_lim[ m ];
    if ( S_index_mapped == 0 && l != sbr -> l_A[ ch ] ) den += Q_M_lim[ m ];
   }  /* end for */
   G_boost = ( acc1 + EPS ) / ( den + EPS );
   G_boost = min( G_boost, 2.51188643F );
   for ( m = ml1; m < ml2; ++m ) {
    adj -> G_lim_boost  [ l ][ m ] = ( float )sqrtf ( G_lim  [ m ] * G_boost );
    adj -> Q_M_lim_boost[ l ][ m ] = ( float )sqrtf ( Q_M_lim[ m ] * G_boost );
    if ( S_M[ m ] != 0 )
     adj -> S_M_boost[ l ][ m ] = ( float )sqrtf ( S_M[ m ] * G_boost );
    else adj -> S_M_boost[ l ][ m ] = 0;
   }  /* end for */

  }  /* end for */

 }  /* end for */

}  /* end calculate_gain */

static void hf_assembly ( sbr_info* sbr, qmf_t Xsbr[ MAX_NTSRHFG ][ 64 ], uint8_t ch ) {

 static const float h_smooth[] __attribute__(   (  section( ".rodata" )  )   ) = {
  0.03183050093751F, 0.11516383427084F,
  0.21816949906249F, 0.30150283239582F,
  0.33333333333333F
 };
 static const int8_t phi_re[] __attribute__(   (  section( ".rodata" )  )   ) = { 1, 0, -1,  0 };
 static const int8_t phi_im[] __attribute__(   (  section( ".rodata" )  )   ) = { 0, 1,  0, -1 };

 uint8_t  m, l, i, n;
 uint16_t fIndexNoise    = 0;
 uint8_t  fIndexSine     = 0;
 uint8_t  assembly_reset = 0;
 float    G_filt, Q_filt;
 uint8_t  h_SL;

 sbr_hfadj_info* adj = &sbr -> adj;

 if ( sbr -> Reset == 1 ) {
  assembly_reset = 1;
  fIndexNoise    = 0;
 } else fIndexNoise = sbr -> index_noise_prev[ ch ];

 fIndexSine = sbr -> psi_is_prev[ ch ];

 for ( l = 0; l < sbr -> L_E[ ch ]; ++l ) {

  uint8_t no_noise = l == sbr -> l_A[ ch ] || l == sbr -> prevEnvIsShort[ ch ];

  h_SL = (  !( sbr -> bs_smoothing_mode == 1 )  ) << 2;
  h_SL = no_noise ? 0 : h_SL;

  if ( assembly_reset ) {

   for ( n = 0; n < 4; ++n ) {
    memcpy (  sbr -> G_temp_prev[ ch ][ n ], adj -> G_lim_boost  [ l ], sbr -> M * sizeof ( float )  );
    memcpy (  sbr -> Q_temp_prev[ ch ][ n ], adj -> Q_M_lim_boost[ l ], sbr -> M * sizeof ( float )  );
   }  /* end for */

   sbr -> GQ_ringbuf_index[ ch ] = 4;
   assembly_reset                = 0;

  }  /* end if */

  for ( i = sbr -> t_E[ ch ][ l ]; i < sbr -> t_E[ ch ][ l + 1 ]; ++i ) {

   memcpy (  sbr -> G_temp_prev[ ch ][  sbr -> GQ_ringbuf_index[ ch ]  ], adj -> G_lim_boost  [ l ], sbr -> M * sizeof ( float )  );
   memcpy (  sbr -> Q_temp_prev[ ch ][  sbr -> GQ_ringbuf_index[ ch ]  ], adj -> Q_M_lim_boost[ l ], sbr -> M * sizeof ( float )  );

   for ( m = 0; m < sbr -> M; ++m ) {

    qmf_t  psi;
    int8_t rev;

    G_filt = 0;
    Q_filt = 0;

    if ( h_SL != 0 ) {

     uint8_t ri = sbr -> GQ_ringbuf_index[ ch ];

     for ( n = 0; n <= 4; ++n ) {

      float curr_h_smooth = h_smooth[ n ];

      if ( ++ri >= 5 ) ri -= 5;

      G_filt += sbr -> G_temp_prev[ ch ][ ri ][ m ] * curr_h_smooth;
      Q_filt += sbr -> Q_temp_prev[ ch ][ ri ][ m ] * curr_h_smooth;

     }  /* end for */

    } else {

     G_filt = sbr -> G_temp_prev[ ch ][  sbr -> GQ_ringbuf_index[ ch ]  ][ m ];
     Q_filt = sbr -> Q_temp_prev[ ch ][  sbr -> GQ_ringbuf_index[ ch ]  ][ m ];

    }  /* end else */

    Q_filt      = adj -> S_M_boost[ l ][ m ] != 0 || no_noise ? 0 : Q_filt;
    fIndexNoise = ( fIndexNoise + 1 ) & 511;

    Xsbr[ i + sbr -> tHFAdj ][ m + sbr -> kx][ 0 ] = G_filt * Xsbr[ i + sbr -> tHFAdj ][ m + sbr -> kx ][ 0 ] + Q_filt * V[ fIndexNoise ][ 0 ];

    if ( sbr -> bs_extension_id == 3 && sbr -> bs_extension_data == 42 ) Xsbr[ i + sbr -> tHFAdj ][ m + sbr -> kx][ 0 ] = 16428320;

    Xsbr[ i + sbr -> tHFAdj ][ m + sbr -> kx ][ 1 ] = G_filt * Xsbr[ i + sbr -> tHFAdj ][ m + sbr -> kx][ 1 ] + Q_filt * V[ fIndexNoise ][ 1 ];

    rev = (  ( m + sbr -> kx ) & 1  ) ? -1 : 1;

    psi[ 0 ] =       adj -> S_M_boost[ l ][ m ] * phi_re[ fIndexSine ];
    psi[ 1 ] = rev * adj -> S_M_boost[ l ][ m ] * phi_im[ fIndexSine ];

    Xsbr[ i + sbr -> tHFAdj ][ m + sbr -> kx ][ 0 ] += psi[ 0 ];
    Xsbr[ i + sbr -> tHFAdj ][ m + sbr -> kx ][ 1 ] += psi[ 1 ];

   }  /* end for */

   fIndexSine = ( fIndexSine + 1 ) & 3;

   ++sbr -> GQ_ringbuf_index[ ch ];

   if ( sbr -> GQ_ringbuf_index[ ch ] >= 5 ) sbr -> GQ_ringbuf_index[ ch ] = 0;

  }  /* end for */

 }  /* end for */

 sbr -> index_noise_prev[ ch ] = fIndexNoise;
 sbr -> psi_is_prev     [ ch ] = fIndexSine;

}  /* end hf_assembly */

static uint8_t hf_adjustment ( sbr_info* sbr, qmf_t Xsbr[ MAX_NTSRHFG ][ 64 ], uint8_t ch ) {

 uint8_t ret = 0;

 if ( sbr -> bs_frame_class[ ch ] == FIXFIX )
  sbr -> l_A[ ch ] = -1;
 else if ( sbr -> bs_frame_class[ ch ] == VARFIX ) {
  if ( sbr -> bs_pointer[ ch ] > 1 )
   sbr -> l_A[ ch ] = -1;
  else sbr -> l_A[ ch ] = sbr -> bs_pointer[ ch ] - 1;
 } else {
  if ( sbr -> bs_pointer[ ch ] == 0 )
   sbr -> l_A[ ch ] = -1;
  else sbr -> l_A[ ch ] = sbr -> L_E[ ch ] + 1 - sbr -> bs_pointer[ ch ];
 }  /* end else */

 ret = estimate_current_envelope ( sbr, Xsbr, ch );

 if ( ret > 0 ) return 1;

 calculate_gain ( sbr, ch );

 hf_assembly ( sbr, Xsbr, ch );

 return 0;

}  /* end hf_adjustment */

static float mapNewBw ( uint8_t invf_mode, uint8_t invf_mode_prev ) {
 switch ( invf_mode ) {
  case 1:
   if ( invf_mode_prev == 0 ) return 0.6F;
   return 0.75F;
  case 2: return 0.9F;
  case 3: return 0.98F;
  default:
  if ( invf_mode_prev == 1 ) return 0.6F;
  return 0.0F;
 }  /* end switch */
}  /* end mapNewBw */

static void calc_chirp_factors ( sbr_info* sbr, uint8_t ch ) {
 uint8_t i;
 for ( i = 0; i < sbr -> N_Q; ++i ) {
  sbr -> bwArray[ ch ][ i ] = mapNewBw (
   sbr -> bs_invf_mode[ ch ][ i ], sbr -> bs_invf_mode_prev[ ch ][ i ]
  );
  if ( sbr -> bwArray[ ch ][ i ] < sbr -> bwArray_prev[ ch ][ i ] )
   sbr -> bwArray[ ch ][ i ] = sbr -> bwArray[ ch ][ i ] * 0.75F + sbr -> bwArray_prev[ ch ][ i ] * 0.25F;
  else sbr -> bwArray[ ch ][ i ] = sbr -> bwArray[ ch ][ i ] * 0.90625F + sbr -> bwArray_prev[ ch ][ i ] * 0.09375F;
  if ( sbr -> bwArray[ ch ][ i ] <  0.01562500F ) sbr -> bwArray[ ch ][ i ] = 0.00000000F;
  if ( sbr -> bwArray[ ch ][ i ] >= 0.99609375F ) sbr -> bwArray[ ch ][ i ] = 0.99609375F;
  sbr -> bwArray_prev     [ ch ][ i ] = sbr -> bwArray     [ ch ][ i ];
  sbr -> bs_invf_mode_prev[ ch ][ i ] = sbr -> bs_invf_mode[ ch ][ i ];
 }  /* end for */
}  /* end calc_chirp_factors */

static void patch_construction ( sbr_info* sbr ) {

 uint8_t i, k;
 uint8_t odd, sb;
 uint8_t msb = sbr -> k0;
 uint8_t usb = sbr -> kx;
 uint8_t goalSbTab[] = { 21, 23, 32, 43, 46, 64, 85, 93, 128, 0, 0, 0 };
 uint8_t goalSb      = goalSbTab[ get_sr_index ( sbr -> sample_rate ) ];

 sbr -> noPatches = 0;

 if ( goalSb < sbr -> kx + sbr -> M )
  for ( i = 0, k = 0; sbr -> f_master[ i ] < goalSb; ++i ) k = i + 1;
 else k = sbr -> N_master;

 if ( sbr -> N_master == 0 ) {
  sbr -> noPatches              = 0;
  sbr -> patchNoSubbands  [ 0 ] = 0;
  sbr -> patchStartSubband[ 0 ] = 0;
  return;
 }  /* end if */

 do {
  uint8_t j = k + 1;
  do {
   sb  = sbr -> f_master[ --j ];
   odd = ( sb - 2 + sbr -> k0 ) % 2;
  } while ( sb > sbr -> k0 - 1 + msb - odd );
  sbr -> patchNoSubbands  [ sbr -> noPatches ] = max( sb - usb, 0 );
  sbr -> patchStartSubband[ sbr -> noPatches ] = sbr->k0 - odd - sbr -> patchNoSubbands[ sbr -> noPatches ];
  if ( sbr -> patchNoSubbands[ sbr -> noPatches] > 0 ) {
   usb = sb;
   msb = sb;
   ++sbr -> noPatches;
  } else msb = sbr -> kx;
  if ( sbr -> f_master[ k ] - sb < 3 ) k = sbr -> N_master;
 } while ( sb != sbr -> kx + sbr -> M );

 if ( sbr -> patchNoSubbands[ sbr -> noPatches - 1 ] < 3 && sbr -> noPatches > 1 ) --sbr -> noPatches;

 sbr -> noPatches = min( sbr -> noPatches, 5 );

}  /* end patch_construction */

static void auto_correlation (
             sbr_info* sbr, acorr_coef* ac, qmf_t buffer[ MAX_NTSRHFG ][ 64 ],
             uint8_t bd, uint8_t len
            ) {

 const float rel = 1 / ( 1 + 1E-6F );

 float   r01r = 0, r01i = 0, r02r = 0, r02i = 0, r11r = 0;
 float   temp1_r, temp1_i, temp2_r, temp2_i, temp3_r, temp3_i, temp4_r, temp4_i, temp5_r, temp5_i;
 int8_t  j;
 uint8_t offset = sbr->tHFAdj;

 temp2_r = buffer[ offset - 2 ][ bd ][ 0 ];
 temp2_i = buffer[ offset - 2 ][ bd ][ 1 ];
 temp3_r = buffer[ offset - 1 ][ bd ][ 0 ];
 temp3_i = buffer[ offset - 1 ][ bd ][ 1 ];
    
 temp4_r = temp2_r;
 temp4_i = temp2_i;
 temp5_r = temp3_r;
 temp5_i = temp3_i;

 for ( j = offset; j < len + offset; ++j ) {
  temp1_r = temp2_r;
  temp1_i = temp2_i;
  temp2_r = temp3_r;
  temp2_i = temp3_i;
  temp3_r = buffer[ j ][ bd ][ 0 ];
  temp3_i = buffer[ j ][ bd ][ 1 ];
  r01r += temp3_r * temp2_r + temp3_i * temp2_i;
  r01i += temp3_i * temp2_r - temp3_r * temp2_i;
  r02r += temp3_r * temp1_r + temp3_i * temp1_i;
  r02i += temp3_i * temp1_r - temp3_r * temp1_i;
  r11r += temp2_r * temp2_r + temp2_i * temp2_i;
 }  /* end for */

 ac -> r12[ 0 ] = r01r - ( temp3_r * temp2_r + temp3_i * temp2_i ) +
                         ( temp5_r * temp4_r + temp5_i * temp4_i );
 ac -> r12[ 1 ] = r01i - ( temp3_i * temp2_r - temp3_r * temp2_i ) +
                         ( temp5_i * temp4_r - temp5_r * temp4_i );
 ac -> r22[ 0 ] = r11r - ( temp2_r * temp2_r + temp2_i * temp2_i ) +
                         ( temp4_r * temp4_r + temp4_i * temp4_i );
 ac -> r01[ 0 ] = r01r;
 ac -> r01[ 1 ] = r01i;
 ac -> r02[ 0 ] = r02r;
 ac -> r02[ 1 ] = r02i;
 ac -> r11[ 0 ] = r11r;

 ac -> det = ac -> r11[ 0 ] * ac -> r22[ 0 ] - rel * ( ac -> r12[ 0 ] * ac -> r12[ 0 ] + ac -> r12[ 1 ] * ac -> r12[ 1 ] );

}  /* end auto_correlation */

static void calc_prediction_coef (
             sbr_info* sbr, qmf_t Xlow[ MAX_NTSRHFG ][ 64 ],
             complex_t* alpha_0, complex_t* alpha_1, uint8_t k
            ) {

 float      tmp;
 acorr_coef ac;

 auto_correlation ( sbr, &ac, Xlow, k, sbr -> numTimeSlotsRate + 6 );

 if ( ac.det == 0 ) {
  alpha_1[ k ][ 0 ] = 0;
  alpha_1[ k ][ 1 ] = 0;
 } else {
  tmp = 1.0F / ac.det;
  alpha_1[k][ 0 ] = ( ac.r01[ 0 ] * ac.r12[ 0 ] - ac.r01[ 1 ] * ac.r12[ 1 ] - ac.r02[ 0 ] * ac.r11[ 0 ] ) * tmp;
  alpha_1[k][ 1 ] = ( ac.r01[ 1 ] * ac.r12[ 0 ] + ac.r01[ 0 ] * ac.r12[ 1 ] - ac.r02[ 1 ] * ac.r11[ 0 ] ) * tmp;
 }  /* end else */

 if ( ac.r11[ 0 ] == 0.0F ) {
  alpha_0[ k ][ 0 ] = 0;
  alpha_0[ k ][ 1 ] = 0;
 } else {
  tmp = 1.0F / ac.r11[ 0 ];
  alpha_0[k][ 0 ] = -( ac.r01[ 0 ] + alpha_1[ k ][ 0 ] * ac.r12[ 0 ] + alpha_1[ k ][ 1 ] * ac.r12[ 1 ] ) * tmp;
  alpha_0[k][ 1 ] = -( ac.r01[ 1 ] + alpha_1[ k ][ 1 ] * ac.r12[ 0 ] - alpha_1[ k ][ 0 ] * ac.r12[ 1 ] ) * tmp;
 }  /* end else */

 if (  ( alpha_0[ k ][ 0 ] * alpha_0[ k ][ 0 ] + alpha_0[ k ][ 1 ] * alpha_0[ k ][ 1 ] >= 16.0F  ) ||
          ( alpha_1[ k ][ 0 ] * alpha_1[ k ][ 0 ] + alpha_1[ k ][ 1 ] * alpha_1[ k ][ 1 ] >= 16.0F  )
 ) {
  alpha_0[ k ][ 0 ] = 0.0F;
  alpha_0[ k ][ 1 ] = 0.0F;
  alpha_1[ k ][ 0 ] = 0.0F;
  alpha_1[ k ][ 1 ] = 0.0F;
 }  /* end if */

}  /* end calc_prediction_coef */

static void limiter_frequency_table ( sbr_info* sbr ) {

 static const float limiterBandsCompare[] __attribute__(   (  section( ".rodata" )  )   ) = {
  1.327152F, 1.185093F, 1.119872F
 };

 uint8_t k, s;
 int8_t  nrLim;

 sbr -> f_table_lim[ 0 ][ 0 ] = sbr -> f_table_res[ LO_RES ][ 0            ] - sbr -> kx;
 sbr -> f_table_lim[ 0 ][ 1 ] = sbr -> f_table_res[ LO_RES ][ sbr -> N_low ] - sbr -> kx;
 sbr -> N_L[ 0 ] = 1;

 for ( s = 1; s < 4; ++s ) {
  int32_t limTable    [ 100 ] = { 0 };
  uint8_t patchBorders[  64 ] = { 0 };
  patchBorders[ 0 ] = sbr -> kx;
  for ( k = 1; k <= sbr -> noPatches; ++k )
   patchBorders[ k ] = patchBorders[ k - 1 ] + sbr -> patchNoSubbands[ k - 1 ];
  for ( k = 0; k <= sbr -> N_low; ++k )
   limTable[ k ] = sbr -> f_table_res[ LO_RES ][ k ];
  for ( k = 1; k < sbr -> noPatches; ++k )
   limTable[ k + sbr -> N_low] = patchBorders[ k ];
  qsort (  limTable, sbr -> noPatches + sbr -> N_low, sizeof ( limTable[ 0 ] ), longcmp  );
  k = 1;
  nrLim = sbr -> noPatches + sbr -> N_low - 1;
  if ( nrLim < 0 ) return;
restart:
  if ( k <= nrLim ) {
   float nOctaves;
   if ( limTable[ k - 1 ] != 0 )
    nOctaves = ( float )limTable[ k ] / ( float )limTable[ k - 1 ];
   else nOctaves = 0;
   if ( nOctaves < limiterBandsCompare[ s - 1 ] ) {
    uint8_t i;
    if ( limTable[ k ] != limTable[ k - 1 ] ) {
     uint8_t found = 0, found2 = 0;
     for ( i = 0; i <= sbr -> noPatches; ++i ) if ( limTable[ k ] == patchBorders[ i ] ) found = 1;
     if ( found ) {
      found2 = 0;
      for ( i = 0; i <= sbr -> noPatches; ++i ) if ( limTable[ k - 1 ] == patchBorders[ i ] ) found2 = 1;
      if ( found2 ) {
       ++k;
       goto restart;
      } else {
       limTable[ k - 1 ] = sbr -> f_table_res[ LO_RES ][ sbr -> N_low ];
       qsort (  limTable, sbr -> noPatches + sbr -> N_low, sizeof ( limTable[ 0 ] ), longcmp  );
       --nrLim;
       goto restart;
      }  /* end else */
     }  /* end if */
    }  /* end if */
    limTable[ k ] = sbr -> f_table_res[ LO_RES ][ sbr -> N_low ];
    qsort (  limTable, nrLim, sizeof ( limTable[ 0 ] ), longcmp  );
    --nrLim;
    goto restart;
   } else {
    ++k;
    goto restart;
   }  /* end else */
  }  /* end if */
  sbr -> N_L[ s ] = nrLim;
  for ( k = 0; k <= nrLim; ++k ) sbr -> f_table_lim[ s ][ k ] = limTable[ k ] - sbr -> kx;
 }  /* end for */

}  /* end limiter_frequency_table */

static void hf_generation (
             sbr_info* sbr, qmf_t Xlow [ MAX_NTSRHFG ][ 64 ],
                            qmf_t Xhigh[ MAX_NTSRHFG ][ 64 ], uint8_t ch
            ) {

 uint8_t   l, i, x;
 complex_t alpha_0[ 64 ], alpha_1[ 64 ];
 uint8_t   offset = sbr -> tHFAdj;
 uint8_t   first  = sbr -> t_E[ ch ][ 0 ];
 uint8_t   last   = sbr -> t_E[ ch ][  sbr -> L_E[ ch ]  ];

 calc_chirp_factors ( sbr, ch );

 if ( ch == 0 && sbr -> Reset ) patch_construction ( sbr );

 for ( i = 0; i < sbr -> noPatches; ++i ) {

  for ( x = 0; x < sbr -> patchNoSubbands[ i ]; ++x ) {

   float   a0_r, a0_i, a1_r, a1_i;
   float   bw, bw2;
   uint8_t q, p, k, g;

   k = sbr -> kx + x;

   for ( q = 0; q < i; ++q ) k += sbr -> patchNoSubbands[ q ];

   p   = sbr -> patchStartSubband[ i ] + x;
   g   = sbr -> table_map_k_to_g[ k ];
   bw  = sbr -> bwArray[ ch ][ g ];
   bw2 = bw * bw;

   if ( bw2 > 0 ) {

    float temp1_r, temp2_r, temp3_r;
    float temp1_i, temp2_i, temp3_i;

    calc_prediction_coef ( sbr, Xlow, alpha_0, alpha_1, p );

    a0_r = alpha_0[ p ][ 0 ] * bw;
    a0_i = alpha_0[ p ][ 1 ] * bw;
    a1_r = alpha_1[ p ][ 0 ] * bw2;
    a1_i = alpha_1[ p ][ 1 ] * bw2;

    temp2_r = Xlow[ first - 2 + offset ][ p ][ 0 ];
    temp2_i = Xlow[ first - 2 + offset ][ p ][ 1 ];
    temp3_r = Xlow[ first - 1 + offset ][ p ][ 0 ];
    temp3_i = Xlow[ first - 1 + offset ][ p ][ 1 ];

    for ( l = first; l < last; ++l ) {
     temp1_r = temp2_r;
     temp2_r = temp3_r;
     temp3_r = Xlow[ l + offset ][ p ][ 0 ];
     temp1_i = temp2_i;
     temp2_i = temp3_i;
     temp3_i = Xlow[ l + offset ][ p ][ 1 ];
     Xhigh[ l + offset ][ k ][ 0 ] = temp3_r + (
      a0_r * temp2_r - a0_i * temp2_i +
      a1_r * temp1_r - a1_i * temp1_i
     );
     Xhigh[ l + offset ][ k ][ 1 ] = temp3_i + (
      a0_i * temp2_r + a0_r * temp2_i +
      a1_i * temp1_r + a1_r * temp1_i
     );
    }  /* end for */

   } else {

    for ( l = first; l < last; ++l ) {
     Xhigh[ l + offset ][ k ][ 0 ] = Xlow[ l + offset ][ p ][ 0 ];
     Xhigh[ l + offset ][ k ][ 1 ] = Xlow[ l + offset ][ p ][ 1 ];
    }  /* end for */

   }  /* end else */

  }  /* end for */

 }  /* end for */

 if ( sbr -> Reset ) limiter_frequency_table ( sbr );

}  /* end hf_generation */

static void sbr_qmf_analysis_32 (
             sbr_info* sbr, qmfa_info* qmfa, const float* input,
             qmf_t X[ MAX_NTSRHFG ][ 64 ], uint8_t offset, uint8_t kx
            ) {

 uint8_t  l;
 uint32_t in = 0;

 float* in_real  = ( float* )SMS_AUD_SPR;
 float* in_imag  = in_real +  32;
 float* out_real = in_real +  64;
 float* out_imag = in_real +  96;
 float* u        = in_real + 128;

 for ( l = 0; l < sbr -> numTimeSlotsRate; ++l ) {
  int16_t n;
  for ( n = 32 - 1; n >= 0; --n )
   qmfa -> x[ qmfa -> x_index + n ] = qmfa -> x[ qmfa -> x_index + n + 320 ] = input[ in++ ];
  for ( n = 0; n < 64; ++n )
   u[ n ] = qmfa -> x[ qmfa -> x_index + n +   0 ] * qmf_c[ 2 * ( n +   0 ) ] +
            qmfa -> x[ qmfa -> x_index + n +  64 ] * qmf_c[ 2 * ( n +  64 ) ] +
            qmfa -> x[ qmfa -> x_index + n + 128 ] * qmf_c[ 2 * ( n + 128 ) ] +
            qmfa -> x[ qmfa -> x_index + n + 192 ] * qmf_c[ 2 * ( n + 192 ) ] +
            qmfa -> x[ qmfa -> x_index + n + 256 ] * qmf_c[ 2 * ( n + 256 ) ];
  qmfa -> x_index -= 32;
  if ( qmfa -> x_index < 0 ) qmfa -> x_index = 320 - 32;
  in_imag[ 31 ] = u[ 1 ];
  in_real[  0 ] = u[ 0 ];
  for ( n = 1; n < 31; ++n ) {
   in_imag[ 31 - n ] =  u[ n  + 1 ];
   in_real[ n      ] = -u[ 64 - n ];
  }  /* end for */
  in_imag[  0 ] =  u[ 32 ];
  in_real[ 31 ] = -u[ 33 ];

  DSP_DCT4Kernel ( in_real, out_real );

  for ( n = 0; n < 16; ++n ) {
   if ( 2 * n + 1 < kx ) {
    X[ l + offset ][ 2 * n     ][ 0 ] =  2.0F * out_real[ n ];
    X[ l + offset ][ 2 * n     ][ 1 ] =  2.0F * out_imag[ n ];
    X[ l + offset ][ 2 * n + 1 ][ 0 ] = -2.0F * out_imag[ 31 - n ];
    X[ l + offset ][ 2 * n + 1 ][ 1 ] = -2.0F * out_real[ 31 - n ];
   } else {
    if ( 2 * n < kx ) {
     X[ l + offset ][ 2 * n ][ 0 ] = 2.0F * out_real[ n ];
     X[ l + offset ][ 2 * n ][ 1 ] = 2.0F * out_imag[ n ];
    } else {
     X[ l + offset ][ 2 * n ][ 0 ] = 0.0F;
     X[ l + offset ][ 2 * n ][ 1 ] = 0.0F;
    }  /* end else */
    X[ l + offset ][ 2 * n + 1 ][ 0 ] = 0.0F;
    X[ l + offset ][ 2 * n + 1 ][ 1 ] = 0.0F;
   }  /* end else */
  }  /* end for */
 }  /* end for */

}  /* end sbr_qmf_analysis_32 */

static uint8_t sbr_process_channel (
                sbr_info* sbr, float* channel_buf,
                uint8_t ch, uint8_t dont_process, const uint8_t downSampledSBR
               ) {

 int16_t k, l;
 uint8_t ret = 0;
 qmf_t   ( *X )[ 64 ] = sbr -> X;

 sbr -> bsco = 0;

 if ( dont_process )
  sbr_qmf_analysis_32 ( sbr, sbr -> qmfa[ ch ], channel_buf, sbr -> Xsbr[ ch ], sbr -> tHFGen, 32 );
 else sbr_qmf_analysis_32 ( sbr, sbr -> qmfa[ ch ], channel_buf, sbr -> Xsbr[ ch ], sbr -> tHFGen, sbr -> kx );

 if ( !dont_process ) {
  hf_generation ( sbr, sbr -> Xsbr[ ch ], sbr -> Xsbr[ ch ], ch );
  ret = hf_adjustment ( sbr, sbr -> Xsbr[ ch ], ch );
  if ( ret > 0 ) dont_process = 1;
 }  /* end if */

 if ( sbr -> just_seeked || dont_process ) {
  for ( l = 0; l < sbr -> numTimeSlotsRate; ++l ) {
   for ( k = 0; k < 32; ++k ) {
    X[ l ][ k ][ 0 ] = sbr -> Xsbr[ ch ][ l + sbr -> tHFAdj ][ k ][ 0 ];
    X[ l ][ k ][ 1 ] = sbr -> Xsbr[ ch ][ l + sbr -> tHFAdj ][ k ][ 1 ];
   }  /* end for */
   for ( k = 32; k < 64; ++k ) {
    X[ l ][ k ][ 0 ] = 0;
    X[ l ][ k ][ 1 ] = 0;
   }  /* end for */
  }  /* end for */
 } else {
  for ( l = 0; l < sbr -> numTimeSlotsRate; ++l ) {
   uint8_t kx_band, M_band, bsco_band;
   if ( l < sbr -> t_E[ ch ][ 0 ] ) {
    kx_band   = sbr -> kx_prev;
    M_band    = sbr -> M_prev;
    bsco_band = sbr -> bsco_prev;
   } else {
    kx_band   = sbr -> kx;
    M_band    = sbr -> M;
    bsco_band = sbr -> bsco;
   }  /* end else */
   for ( k = 0; k < kx_band + bsco_band; ++k ) {
    X[ l ][ k ][ 0 ] = sbr -> Xsbr[ ch ][ l + sbr -> tHFAdj ][ k ][ 0 ];
    X[ l ][ k ][ 1 ] = sbr -> Xsbr[ ch ][ l + sbr -> tHFAdj ][ k ][ 1 ];
   }  /* end for */
   for ( k = kx_band + bsco_band; k < kx_band + M_band; ++k ) {
    X[ l ][ k ][ 0 ] = sbr -> Xsbr[ ch ][ l + sbr -> tHFAdj ][ k ][ 0 ];
    X[ l ][ k ][ 1 ] = sbr -> Xsbr[ ch ][ l + sbr -> tHFAdj ][ k ][ 1 ];
   }  /* end for */
   for (  k = max( kx_band + bsco_band, kx_band + M_band ); k < 64; ++k ) {
    X[ l ][ k ][ 0 ] = 0;
    X[ l ][ k ][ 1 ] = 0;
   }  /* end for */
  }  /* end for */
 }  /* end else */

 return ret;

}  /* end sbr_process_channel */

static void sbr_qmf_synthesis_32 (
             sbr_info* sbr, qmfs_info* qmfs, float* output
            ) {

 float   x1[ 32 ], x2[ 32 ];
 float   scale = 1.0F / 64.0F;
 int32_t n, k, out = 0;
 uint8_t l;

 qmf_t ( *X )[ 64 ] = sbr -> X;

 for ( l = 0; l < sbr -> numTimeSlotsRate; ++l ) {

  for ( k = 0; k < 32; ++k ) {
   x1[ k ] = X[ l ][ k ][ 0 ] * qmf32_pre_twiddle[ k ][ 0 ] - X[ l ][ k ][ 1 ] * qmf32_pre_twiddle[ k ][ 1 ];
   x2[ k ] = X[ l ][ k ][ 1 ] * qmf32_pre_twiddle[ k ][ 0 ] + X[ l ][ k ][ 0 ] * qmf32_pre_twiddle[ k ][ 1 ];
   x1[ k ] *= scale;
   x2[ k ] *= scale;
  }  /* end for */

  DCT4_32 ( x1, x1 );
  DST4_32 ( x2, x2 );

  for ( n = 0; n < 32; ++n ) {
   qmfs -> v[ qmfs -> v_index + n      ] = qmfs -> v[ qmfs -> v_index + 640 + n      ] = -x1[ n ] + x2[ n ];
   qmfs -> v[ qmfs -> v_index + 63 - n ] = qmfs -> v[ qmfs -> v_index + 640 + 63 - n ] =  x1[ n ] + x2[ n ];
  }  /* end for */

  for ( k = 0; k < 32; ++k ) {
   output[ out++ ] = qmfs -> v[ qmfs -> v_index +   0 + k ] * qmf_c[   0 + 2 * k ] +
                     qmfs -> v[ qmfs -> v_index +  96 + k ] * qmf_c[  64 + 2 * k ] +
                     qmfs -> v[ qmfs -> v_index + 128 + k ] * qmf_c[ 128 + 2 * k ] +
                     qmfs -> v[ qmfs -> v_index + 224 + k ] * qmf_c[ 192 + 2 * k ] +
                     qmfs -> v[ qmfs -> v_index + 256 + k ] * qmf_c[ 256 + 2 * k ] +
                     qmfs -> v[ qmfs -> v_index + 352 + k ] * qmf_c[ 320 + 2 * k ] +
                     qmfs -> v[ qmfs -> v_index + 384 + k ] * qmf_c[ 384 + 2 * k ] +
                     qmfs -> v[ qmfs -> v_index + 480 + k ] * qmf_c[ 448 + 2 * k ] +
                     qmfs -> v[ qmfs -> v_index + 512 + k ] * qmf_c[ 512 + 2 * k ] +
                     qmfs -> v[ qmfs -> v_index + 608 + k ] * qmf_c[ 576 + 2 * k ];
  }  /* end for */

  qmfs -> v_index -= 64;

  if ( qmfs -> v_index < 0 ) qmfs -> v_index = ( 640 - 64 );

 }  /* end for */

}  /* end sbr_qmf_synthesis_32 */

static void sbr_qmf_synthesis_64 (
             sbr_info* sbr, qmfs_info* qmfs, float* output
            ) {

 qmf_t*  pX;
 float*  pring_buffer_1, *pring_buffer_3;
 float   scale = 1.0F / 64.0F;
 int32_t n, k, out = 0;
 uint8_t l;

 qmf_t ( *X )[ 64 ] = sbr -> X;

 float* in_real1  = ( float* )SMS_AUD_SPR;
 float* in_imag1  = in_real1 +  32;
 float* out_real1 = in_real1 +  64;
 float* out_imag1 = in_real1 +  96;
 float* in_real2  = in_real1 + 128;
 float* in_imag2  = in_real1 + 160;
 float* out_real2 = in_real1 + 192;
 float* out_imag2 = in_real1 + 224;

 for ( l = 0; l < sbr -> numTimeSlotsRate; ++l ) {

  pX = X[ l ];

  in_imag1[ 31 ] = scale * pX[ 1 ][ 0 ];
  in_real1[  0 ] = scale * pX[ 0 ][ 0 ];
  in_imag2[ 31 ] = scale * pX[ 63 - 1 ][ 1 ];
  in_real2[  0 ] = scale * pX[ 63 - 0 ][ 1 ];

  for ( k = 1; k < 31; ++k ) {
   in_imag1[ 31 - k ] = scale * pX[ 2 * k + 1 ][ 0 ];
   in_real1[      k ] = scale * pX[ 2 * k     ][ 0 ];
   in_imag2[ 31 - k ] = scale * pX[ 63 - ( 2 * k + 1 ) ][ 1 ];
   in_real2[      k ] = scale * pX[ 63 - ( 2 * k     ) ][ 1 ];
  }  /* end for */

  in_imag1[  0 ] = scale * pX[ 63 ][ 0 ];
  in_real1[ 31 ] = scale * pX[ 62 ][ 0 ];
  in_imag2[  0 ] = scale * pX[  0 ][ 1 ];
  in_real2[ 31 ] = scale * pX[  1 ][ 1 ];

  DSP_DCT4Kernel ( in_real1, out_real1 );
  DSP_DCT4Kernel ( in_real2, out_real2 );

  pring_buffer_1 = qmfs -> v + qmfs -> v_index;
  pring_buffer_3 = pring_buffer_1 + 1280;

  for ( n = 0; n < 32; ++n ) {
   pring_buffer_1[       ( 2 * n + 0 ) ] = pring_buffer_3[       ( 2 * n + 0 ) ] = out_real2[      n ] - out_real1[      n ];
   pring_buffer_1[ 127 - ( 2 * n + 0 ) ] = pring_buffer_3[ 127 - ( 2 * n + 0 ) ] = out_real2[      n ] + out_real1[      n ];
   pring_buffer_1[       ( 2 * n + 1 ) ] = pring_buffer_3[       ( 2 * n + 1 ) ] = out_imag2[ 31 - n ] + out_imag1[ 31 - n ];
   pring_buffer_1[ 127 - ( 2 * n + 1 ) ] = pring_buffer_3[ 127 - ( 2 * n + 1 ) ] = out_imag2[ 31 - n ] - out_imag1[ 31 - n ];
  }  /* end for */

  pring_buffer_1 = qmfs -> v + qmfs -> v_index;

  for ( k = 0; k < 64; ++k ) {
   output[ out++ ] = pring_buffer_1[ k +    0 ] * qmf_c[ k +   0 ] +
                     pring_buffer_1[ k +  192 ] * qmf_c[ k +  64 ] +
                     pring_buffer_1[ k +  256 ] * qmf_c[ k + 128 ] +
                     pring_buffer_1[ k +  448 ] * qmf_c[ k + 192 ] +
                     pring_buffer_1[ k +  512 ] * qmf_c[ k + 256 ] +
                     pring_buffer_1[ k +  704 ] * qmf_c[ k + 320 ] +
                     pring_buffer_1[ k +  768 ] * qmf_c[ k + 384 ] +
                     pring_buffer_1[ k +  960 ] * qmf_c[ k + 448 ] +
                     pring_buffer_1[ k + 1024 ] * qmf_c[ k + 512 ] +
                     pring_buffer_1[ k + 1216 ] * qmf_c[ k + 576 ];
  }  /* end for */

  qmfs -> v_index -= 128;

  if ( qmfs -> v_index < 0 ) qmfs -> v_index = ( 1280 - 128 );

 }  /* end for */

}  /* end sbr_qmf_synthesis_64 */

static uint8_t sbrDecodeSingleFrame (
                sbr_info* sbr, float* channel, const uint8_t just_seeked, const uint8_t downSampledSBR
               ) {

 uint8_t dont_process = 0;
 uint8_t ret          = 0;

 if ( sbr -> id_aac != ID_SCE && sbr -> id_aac != ID_LFE ) return 21;

 if ( sbr -> ret || sbr -> header_count == 0 ) {
  dont_process = 1;
  if ( sbr -> ret && sbr -> Reset ) sbr -> bs_start_freq_prev = -1;
 }  /* end if */

 if ( just_seeked )
  sbr -> just_seeked = 1;
 else sbr -> just_seeked = 0;

 sbr -> ret += sbr_process_channel (
  sbr, channel, 0, dont_process, downSampledSBR
 );

 if ( downSampledSBR )
  sbr_qmf_synthesis_32 ( sbr, sbr -> qmfs[ 0 ], channel );
 else sbr_qmf_synthesis_64 ( sbr, sbr -> qmfs[ 0 ], channel );

 if ( sbr -> bs_header_flag ) sbr -> just_seeked = 0;

 if ( sbr -> header_count != 0 && sbr -> ret == 0 ) {
  ret = sbr_save_prev_data ( sbr, 0 );
  if ( ret ) return ret;
 } /* end if */

 sbr_save_matrix ( sbr, 0 );

 ++sbr -> frame;

 return 0;

}  /* end sbrDecodeSingleFrame */

static uint8_t sbrDecodeCoupleFrame (
                sbr_info* sbr, float* left_chan, float* right_chan,
                const uint8_t just_seeked, const uint8_t downSampledSBR
               ) {

 uint8_t dont_process = 0;
 uint8_t ret          = 0;

 if ( sbr -> id_aac != ID_CPE ) return 21;

 if ( sbr -> ret || sbr -> header_count == 0 ) {
  dont_process = 1;
  if ( sbr -> ret && sbr -> Reset ) sbr -> bs_start_freq_prev = -1;
 }  /* end if */

 if ( just_seeked )
  sbr -> just_seeked = 1;
 else sbr -> just_seeked = 0;

 sbr -> ret += sbr_process_channel (
  sbr, left_chan, 0, dont_process, downSampledSBR
 );

 if ( downSampledSBR )
  sbr_qmf_synthesis_32 ( sbr, sbr -> qmfs[ 0 ], left_chan );
 else sbr_qmf_synthesis_64 ( sbr, sbr -> qmfs[ 0 ], left_chan );

 sbr -> ret += sbr_process_channel (
  sbr, right_chan, 1, dont_process, downSampledSBR
 );

 if ( downSampledSBR )
  sbr_qmf_synthesis_32 ( sbr, sbr -> qmfs[ 1 ], right_chan );
 else sbr_qmf_synthesis_64 ( sbr, sbr -> qmfs[ 1 ], right_chan );

 if ( sbr -> bs_header_flag ) sbr -> just_seeked = 0;

 if ( sbr -> header_count != 0 && sbr -> ret == 0 ) {
  ret = sbr_save_prev_data ( sbr, 0 );
  if ( ret ) return ret;
  ret = sbr_save_prev_data ( sbr, 1 );
  if ( ret ) return ret;
 }  /* end if */

 sbr_save_matrix ( sbr, 0 );
 sbr_save_matrix ( sbr, 1 );

 ++sbr -> frame;

 return 0;

}  /* end sbrDecodeCoupleFrame */

static void drc_init ( drc_info* drc, float cut, float boost ) {

 drc -> ctrl1            = cut;
 drc -> ctrl2            = boost;
 drc -> num_bands        = 1;
 drc -> band_top   [ 0 ] = 1024 / 4 - 1;
 drc -> dyn_rng_sgn[ 0 ] = 1;
 drc -> dyn_rng_ctl[ 0 ] = 0;

}  /* end drc_init */

static int8_t SMS_INLINE can_decode_ot ( const uint8_t object_type ) {
 switch ( object_type ) {
  case FAAD_LC  : return 0;
  case FAAD_MAIN: return 0;
 }  /* end switch */
 return -1;
}  /* end can_decode_ot */

static uint32_t get_sample_rate ( const uint8_t sr_index ) {
 static const uint32_t sample_rates[] __attribute__(   (  section( ".rodata" )  )   ) = {
  96000, 88200, 64000, 48000, 44100, 32000,
  24000, 22050, 16000, 12000, 11025,  8000
 };
 if ( sr_index < 12 ) return sample_rates[ sr_index ];
 return 0;
}  /* end get_sample_rate */

static _NeAACDecHandle _NeAACDecOpen ( void ) {

 _NeAACDecHandle hDecoder = NULL;

 if (    (   hDecoder = ( _NeAACDecHandle )calloc (  1, sizeof ( _NeAACDecStruct )  )   ) == NULL    ) return NULL;

 hDecoder -> cmes                 = mes;
 hDecoder -> config.defObjectType = FAAD_LC;
 hDecoder -> config.defSampleRate = 44100;
 hDecoder -> __r1                 = 1;
 hDecoder -> __r2                 = 1;
 drc_init ( &hDecoder -> drc, 1.0F, 1.0F );

 return hDecoder;

}  /* end _NeAACDecOpen */

static void SMS_INLINE filter_bank_end ( fb_info *fb ) {
 DSP_MDCTDestroy ( &fb -> mdct256  );
 DSP_MDCTDestroy ( &fb -> mdct2048 );
}  /* end filter_bank_end */

static void _NeAACDecClose ( _NeAACDecHandle hDecoder ) {

 uint8_t i;

 for ( i = 0; i < MAX_CHANNELS; ++i ) {
  if ( hDecoder -> time_out   [ i ] ) free ( hDecoder -> time_out   [ i ] );
  if ( hDecoder -> fb_intermed[ i ] ) free ( hDecoder -> fb_intermed[ i ] );
  if ( hDecoder -> pred_stat  [ i ] ) free ( hDecoder -> pred_stat  [ i ] );
 }  /* end for */

 filter_bank_end ( &hDecoder -> fb  );

 for ( i = 0; i < MAX_SYNTAX_ELEMENTS; ++i ) if ( hDecoder -> sbr[ i ] ) sbrDecodeEnd ( hDecoder -> sbr[ i ] );

 free ( hDecoder );

}  /* end _NeAACDecClose */

static uint8_t program_config_element ( program_config* pce, SMS_BitContext* ld ) {

 uint8_t i;

 memset (  pce, 0, sizeof ( program_config )  );

 pce -> channels                   = 0;
 pce -> element_instance_tag       = ( uint8_t )SMS_GetBits ( ld, 4 );
 pce -> object_type                = ( uint8_t )SMS_GetBits ( ld, 2 );
 pce -> sf_index                   = ( uint8_t )SMS_GetBits ( ld, 4 );
 pce -> num_front_channel_elements = ( uint8_t )SMS_GetBits ( ld, 4 );
 pce -> num_side_channel_elements  = ( uint8_t )SMS_GetBits ( ld, 4 );
 pce -> num_back_channel_elements  = ( uint8_t )SMS_GetBits ( ld, 4 );
 pce -> num_lfe_channel_elements   = ( uint8_t )SMS_GetBits ( ld, 2 );
 pce -> num_assoc_data_elements    = ( uint8_t )SMS_GetBits ( ld, 3 );
 pce -> num_valid_cc_elements      = ( uint8_t )SMS_GetBits ( ld, 4 );

 pce -> mono_mixdown_present = SMS_GetBit ( ld );
 if ( pce-> mono_mixdown_present == 1 ) pce -> mono_mixdown_element_number = ( uint8_t )SMS_GetBits ( ld, 4 );

 pce -> stereo_mixdown_present = SMS_GetBit ( ld );
 if ( pce -> stereo_mixdown_present == 1 ) pce -> stereo_mixdown_element_number = ( uint8_t )SMS_GetBits ( ld, 4 );

 pce -> matrix_mixdown_idx_present = SMS_GetBit ( ld );
 if ( pce -> matrix_mixdown_idx_present == 1 ) {
  pce -> matrix_mixdown_idx     = ( uint8_t )SMS_GetBits ( ld, 2 );
  pce -> pseudo_surround_enable = SMS_GetBit ( ld );
 }  /* end if */

 for ( i = 0; i < pce -> num_front_channel_elements; ++i ) {
  pce -> front_element_is_cpe    [ i ] = SMS_GetBit ( ld );
  pce -> front_element_tag_select[ i ] = ( uint8_t )SMS_GetBits ( ld, 4 );
  if ( pce -> front_element_is_cpe[ i ] & 1 ) {
   pce -> cpe_channel[  pce -> front_element_tag_select[ i ]  ] = pce -> channels;
   pce -> num_front_channels += 2;
   pce -> channels           += 2;
  } else {
   pce -> sce_channel[  pce -> front_element_tag_select[ i ]  ] = pce -> channels;
   pce -> num_front_channels++;
   pce -> channels++;
  }  /* end else */
 }  /* end for */

 for ( i = 0; i < pce -> num_side_channel_elements; ++i ) {
  pce -> side_element_is_cpe    [ i ] = SMS_GetBit ( ld );
  pce -> side_element_tag_select[ i ] = ( uint8_t )SMS_GetBits ( ld, 4 );
  if ( pce -> side_element_is_cpe[ i ] & 1 ) {
   pce -> cpe_channel[  pce -> side_element_tag_select[ i ]  ] = pce -> channels;
   pce -> num_side_channels += 2;
   pce -> channels          += 2;
  } else {
   pce -> sce_channel[  pce -> side_element_tag_select[ i ]  ] = pce -> channels;
   pce -> num_side_channels++;
   pce -> channels++;
  }  /* end else */
 }  /* end for */

 for ( i = 0; i < pce -> num_back_channel_elements; ++i ) {
  pce -> back_element_is_cpe    [ i ] = SMS_GetBit ( ld );
  pce -> back_element_tag_select[ i ] = ( uint8_t )SMS_GetBits ( ld, 4 );
  if ( pce -> back_element_is_cpe[ i ] & 1 ) {
   pce -> cpe_channel[  pce->back_element_tag_select[ i ]  ] = pce -> channels;
   pce -> channels          += 2;
   pce -> num_back_channels += 2;
  } else {
   pce -> sce_channel[  pce -> back_element_tag_select[ i ]  ] = pce -> channels;
   pce -> num_back_channels++;
   pce->channels++;
  }  /* end else */
 }  /* end for */

 for ( i = 0; i < pce -> num_lfe_channel_elements; ++i ) {
  pce -> lfe_element_tag_select[ i ] = ( uint8_t )SMS_GetBits ( ld, 4 );
  pce -> sce_channel[  pce -> lfe_element_tag_select[ i ]  ] = pce -> channels;
  pce -> num_lfe_channels++;
  pce -> channels++;
 }  /* end for */

 for ( i = 0; i < pce -> num_assoc_data_elements; ++i ) pce -> assoc_data_element_tag_select[ i ] = ( uint8_t )SMS_GetBits ( ld, 4 );

 for ( i = 0; i < pce -> num_valid_cc_elements; ++i ) {
  pce -> cc_element_is_ind_sw       [ i ] = SMS_GetBit ( ld );
  pce -> valid_cc_element_tag_select[ i ] = ( uint8_t )SMS_GetBits ( ld, 4 );
 }  /* end for */

 SMS_AlignBits ( ld );

 pce -> comment_field_bytes = ( uint8_t )SMS_GetBits ( ld, 8 );

 for ( i = 0; i < pce -> comment_field_bytes; ++i ) pce -> comment_field_data[ i ] = ( uint8_t )SMS_GetBits ( ld, 8 );

 pce -> comment_field_data[ i ] = 0;

 if ( pce -> channels > MAX_CHANNELS ) return 22;

 return 0;

}  /* end program_config_element */

static void get_adif_header ( adif_header* adif, SMS_BitContext* ld ) {
 uint8_t i;
 SMS_GetBits ( ld, 8 );
 SMS_GetBits ( ld, 8 );
 SMS_GetBits ( ld, 8 );
 SMS_GetBits ( ld, 8 );
 adif -> copyright_id_present = SMS_GetBit ( ld );
 if ( adif -> copyright_id_present ) {
  for ( i = 0; i < 72/8; ++i ) adif -> copyright_id[ i ] = ( int8_t )SMS_GetBits ( ld, 8 );
  adif -> copyright_id[ i ] = 0;
 }  /* end if */
 adif -> original_copy               = SMS_GetBit ( ld );
 adif -> home                        = SMS_GetBit ( ld );
 adif -> bitstream_type              = SMS_GetBit ( ld );
 adif -> bitrate                     = SMS_GetBits ( ld, 23 );
 adif -> num_program_config_elements = ( uint8_t )SMS_GetBits ( ld, 4 );
 for ( i = 0; i < adif -> num_program_config_elements + 1; ++i ) {
  if ( adif -> bitstream_type == 0 )
   adif->adif_buffer_fullness = SMS_GetBits ( ld, 20 );
  else adif->adif_buffer_fullness = 0;
  program_config_element ( &adif -> pce[ i ], ld );
 }  /* end for */
}  /* end get_adif_header */

static uint8_t adts_fixed_header ( adts_header* adts, SMS_BitContext* ld ) {
 uint16_t i;
 uint8_t  sync_err = 1;
 for ( i = 0; i < 768; ++i ) {
  adts -> syncword = ( uint16_t )SMS_ShowBits ( ld, 12 );
  if ( adts -> syncword != 0xFFF )
   SMS_GetBits ( ld, 8 );
  else {
   sync_err = 0;
   SMS_GetBits ( ld, 12 );
   break;
  }  /* end else */
 }  /* end for */
 if ( sync_err ) return 5;
 adts -> id                    = SMS_GetBit ( ld );
 adts -> layer                 = ( uint8_t )SMS_GetBits ( ld, 2 );
 adts -> protection_absent     = SMS_GetBit ( ld );
 adts -> profile               = ( uint8_t )SMS_GetBits ( ld, 2 );
 adts -> sf_index              = ( uint8_t )SMS_GetBits ( ld, 4 );
 adts -> private_bit           = SMS_GetBit ( ld );
 adts -> channel_configuration = ( uint8_t )SMS_GetBits ( ld, 3 );
 adts -> original              = SMS_GetBit ( ld );
 adts -> home                  = SMS_GetBit ( ld );
 if  ( adts -> old_format == 1 ) {
  if ( adts -> id == 0 ) adts -> emphasis = ( uint8_t )SMS_GetBits ( ld, 2 );
 }  /* end if */
 return 0;
}  /* end adts_fixed_header */

static void SMS_INLINE adts_variable_header ( adts_header* adts, SMS_BitContext* ld ) {
 adts -> copyright_identification_bit   = SMS_GetBit ( ld );
 adts -> copyright_identification_start = SMS_GetBit ( ld );
 adts -> aac_frame_length               = ( uint16_t )SMS_GetBits ( ld, 13 );
 adts -> adts_buffer_fullness           = ( uint16_t )SMS_GetBits ( ld, 11 );
 adts -> no_raw_data_blocks_in_frame    = ( uint8_t  )SMS_GetBits ( ld, 2  );
}  /* end adts_variable_header */

static void SMS_INLINE adts_error_check ( adts_header* adts, SMS_BitContext* ld ) {
 if ( adts -> protection_absent == 0 ) adts -> crc_check = ( uint16_t )SMS_GetBits ( ld, 16 );
}  /* end adts_error_check */

static uint8_t adts_frame ( adts_header* adts, SMS_BitContext* ld ) {
 if (  adts_fixed_header ( adts, ld )  ) return 5;
 adts_variable_header ( adts, ld );
 adts_error_check ( adts, ld );
 return 0;
}  /* end adts_frame */

static void filter_bank_init ( fb_info* fb ) {
 SMS_MDCTContext lMDCT256;
 SMS_MDCTContext lMDCT2048;
 int             lfSave = fb -> mdct256.m_N;
 if ( lfSave ) {
  lMDCT256  = fb -> mdct256;
  lMDCT2048 = fb -> mdct2048;
 }  /* end if */
 memset (  fb, 0, sizeof ( fb_info )  );
 if ( !lfSave ) {
  DSP_MDCTInit ( &fb -> mdct256,   8, 0.088388F );
  DSP_MDCTInit ( &fb -> mdct2048, 11, 0.03125F  );
 } else {
  fb -> mdct256  = lMDCT256;
  fb -> mdct2048 = lMDCT2048;
 }  /* end else */
 fb -> long_window [ 0 ] = ( float* )sine_long_1024;
 fb -> short_window[ 0 ] = ( float* )sine_short_128;
 fb -> long_window [ 1 ] = ( float* )kbd_long_1024;
 fb -> short_window[ 1 ] = ( float* )kbd_short_128;
}  /* end filter_bank_init */

static int32_t _NeAACDecInit (
                _NeAACDecHandle hDecoder, uint8_t* buffer, uint32_t buffer_size, uint32_t* samplerate, uint8_t* channels
               ) {

 uint32_t       bits = 0;
 SMS_BitContext ld;
 adif_header    adif;
 adts_header    adts;

 hDecoder -> sf_index    = get_sr_index ( hDecoder -> config.defSampleRate );
 hDecoder -> object_type = hDecoder -> config.defObjectType;

 *samplerate = get_sample_rate ( hDecoder -> sf_index );
 *channels   = 2;

 if ( buffer ) {

  SMS_InitGetBits ( &ld, buffer, buffer_size << 3 );

  if (  ( buffer[ 0 ] == 'A' ) && ( buffer[ 1 ] == 'D' ) &&
        ( buffer[ 2 ] == 'I' ) && ( buffer[ 3 ] == 'F' )
  ) {
   hDecoder -> adif_header_present = 1;
   get_adif_header ( &adif, &ld );
   SMS_AlignBits ( &ld );
   hDecoder -> sf_index    = adif.pce[ 0 ].sf_index;
   hDecoder -> object_type = adif.pce[ 0 ].object_type + 1;
   *samplerate = get_sample_rate ( hDecoder -> sf_index );
   *channels   = adif.pce[ 0 ].channels;
   memcpy (  &( hDecoder -> pce ), &( adif.pce[ 0 ] ), sizeof ( program_config )  );
   hDecoder -> pce_set = 1;
   bits = bit2byte ( ld.m_Idx );
  } else if (  SMS_ShowBits ( &ld, 12 ) == 0xFFF ) {
   hDecoder -> adts_header_present = 1;
   adts.old_format = hDecoder -> config.useOldADTSFormat;
   adts_frame ( &adts, &ld );
   hDecoder -> sf_index    = adts.sf_index;
   hDecoder -> object_type = adts.profile + 1;
   *samplerate = get_sample_rate ( hDecoder -> sf_index );
   *channels   = ( adts.channel_configuration > 6 ) ? 2 : adts.channel_configuration;
  }  /* end if */

 }  /* end if */

 hDecoder -> channelConfiguration = *channels;

 if (  *samplerate <= 24000 && !( hDecoder -> config.dontUpSampleImplicitSBR )  ) {
  *samplerate                *= 2;
  hDecoder -> forceUpSampling = 1;
 } else if ( *samplerate > 24000 && !( hDecoder -> config.dontUpSampleImplicitSBR )  ) hDecoder -> downSampledSBR = 1;

 filter_bank_init ( &hDecoder -> fb );

 if (  can_decode_ot ( hDecoder -> object_type ) < 0  ) return 0;

 return 1;

}  /* end _NeAACDecInit */

static int8_t GASpecificConfig ( SMS_BitContext* ld, _mp4AudioSpecificConfig* mp4ASC, program_config* pce_out ) {
 program_config pce;
 mp4ASC -> frameLengthFlag = SMS_GetBit ( ld );
 if ( mp4ASC -> frameLengthFlag == 1 ) return -3;
 mp4ASC -> dependsOnCoreCoder = SMS_GetBit ( ld );
 if ( mp4ASC -> dependsOnCoreCoder == 1 ) mp4ASC -> coreCoderDelay = ( uint16_t )SMS_GetBits ( ld, 14 );
 mp4ASC -> extensionFlag = SMS_GetBit ( ld );
 if ( mp4ASC -> channelsConfiguration == 0 ) {
  if (  program_config_element ( &pce, ld )  ) return -3;
  memcpy (  pce_out, &pce, sizeof ( program_config )  );
 }  /* end if */
 return 0;
}  /* end GASpecificConfig */

static int8_t AudioSpecificConfig2 (
               uint8_t* pBuffer, uint32_t buffer_size,
               _mp4AudioSpecificConfig* mp4ASC, program_config* pce
              ) {

 SMS_BitContext ld;
 int8_t         result = 0;
 int8_t bits_to_decode = 0;

 memset (  mp4ASC, 0, sizeof ( _mp4AudioSpecificConfig )  );

 SMS_InitGetBits ( &ld, pBuffer, buffer_size << 3 );
 SMS_AlignBits ( &ld );

 mp4ASC -> objectTypeIndex        = ( uint8_t )SMS_GetBits ( &ld, 5 );
 mp4ASC -> samplingFrequencyIndex = ( uint8_t )SMS_GetBits ( &ld, 4 );
 mp4ASC -> channelsConfiguration  = ( uint8_t )SMS_GetBits ( &ld, 4 );
 mp4ASC -> samplingFrequency      = get_sample_rate ( mp4ASC -> samplingFrequencyIndex );

 if ( ObjectTypesTable[ mp4ASC -> objectTypeIndex ] != 1 ) return -1;
 if ( mp4ASC -> samplingFrequency == 0                   ) return -2;
 if ( mp4ASC -> channelsConfiguration > 7                ) return -3;

 mp4ASC -> sbr_present_flag = 0;
 if  ( mp4ASC -> objectTypeIndex == 5 ) {
  uint8_t tmp;
  mp4ASC -> sbr_present_flag = 1;
  tmp = ( uint8_t )SMS_GetBits ( &ld, 4 );
  if ( tmp == mp4ASC -> samplingFrequencyIndex ) mp4ASC -> downSampledSBR = 1;
  mp4ASC -> samplingFrequencyIndex = tmp;
  if ( mp4ASC -> samplingFrequencyIndex == 15 )
   mp4ASC -> samplingFrequency = ( uint32_t )SMS_GetBits ( &ld, 24 );
  else mp4ASC -> samplingFrequency = get_sample_rate ( mp4ASC -> samplingFrequencyIndex );
  mp4ASC -> objectTypeIndex = ( uint8_t )SMS_GetBits ( &ld, 5 );
 }  /* end if */

 if ( mp4ASC -> objectTypeIndex == 1 || mp4ASC->objectTypeIndex == 2 ||
        mp4ASC->objectTypeIndex == 3 || mp4ASC->objectTypeIndex == 4 ||
        mp4ASC->objectTypeIndex == 6 || mp4ASC->objectTypeIndex == 7
 )
  result = GASpecificConfig ( &ld, mp4ASC, pce );
 else result = -4;

 bits_to_decode = ( int8_t )( buffer_size * 8 - ld.m_Idx );

 if (  mp4ASC -> objectTypeIndex != 5 && bits_to_decode >= 16 ) {
  int16_t syncExtensionType = ( int16_t )SMS_GetBits ( &ld, 11 );
  if ( syncExtensionType == 0x2B7 ) {
   uint8_t tmp_OTi = ( uint8_t )SMS_GetBits ( &ld, 5 );
   if ( tmp_OTi == 5 ) {
    mp4ASC -> sbr_present_flag = ( uint8_t )SMS_GetBit ( &ld );
    if ( mp4ASC -> sbr_present_flag ) {
     uint8_t tmp;
     mp4ASC -> objectTypeIndex = tmp_OTi;
     tmp = ( uint8_t )SMS_GetBits ( &ld, 4 );
     if ( tmp == mp4ASC -> samplingFrequencyIndex ) mp4ASC -> downSampledSBR = 1;
     mp4ASC -> samplingFrequencyIndex = tmp;
     if ( mp4ASC -> samplingFrequencyIndex == 15 )
      mp4ASC -> samplingFrequency = ( uint32_t )SMS_GetBits ( &ld, 24 );
     else mp4ASC -> samplingFrequency = get_sample_rate ( mp4ASC -> samplingFrequencyIndex );
    }  /* end if */
   }  /* end if */
  }  /* end if */
 }  /* end if */
 if ( !mp4ASC -> sbr_present_flag ) {
  if ( mp4ASC -> samplingFrequency <= 24000 ) {
   mp4ASC -> samplingFrequency *= 2;
   mp4ASC -> forceUpSampling    = 1;
  } else mp4ASC -> downSampledSBR = 1;
 }  /* end if */

 return result;

}  /* end AudioSpecificConfig2 */

static int8_t _NeAACDecInit2 ( _NeAACDecHandle hDecoder, uint8_t* pBuffer, uint32_t SizeOfDecoderSpecificInfo, uint32_t* samplerate, uint8_t* channels ) {

 int8_t                  rc;
 _mp4AudioSpecificConfig mp4ASC;

 hDecoder -> adif_header_present = 0;
 hDecoder -> adts_header_present = 0;

 rc = AudioSpecificConfig2 (  pBuffer, SizeOfDecoderSpecificInfo, &mp4ASC, &( hDecoder -> pce )  );

 *samplerate = mp4ASC.samplingFrequency;

 if ( mp4ASC.channelsConfiguration )
  *channels = mp4ASC.channelsConfiguration;
 else {
  *channels           = hDecoder -> pce.channels;
  hDecoder -> pce_set = 1;
 }  /* end else */

 hDecoder -> sf_index    = mp4ASC.samplingFrequencyIndex;
 hDecoder -> object_type = mp4ASC.objectTypeIndex;

 hDecoder -> sbr_present_flag = mp4ASC.sbr_present_flag;
 hDecoder -> downSampledSBR   = mp4ASC.downSampledSBR;
 if ( hDecoder -> config.dontUpSampleImplicitSBR == 0 )
  hDecoder -> forceUpSampling = mp4ASC.forceUpSampling;
 else hDecoder -> forceUpSampling = 0;
 if (   (  ( hDecoder -> sbr_present_flag == 1 ) && ( !hDecoder -> downSampledSBR )  ) || hDecoder -> forceUpSampling == 1  ) hDecoder -> sf_index = get_sr_index ( mp4ASC.samplingFrequency / 2 );

 if ( rc != 0 ) return 0;

 hDecoder -> channelConfiguration = mp4ASC.channelsConfiguration;

 if ( mp4ASC.frameLengthFlag ) return 0;

 filter_bank_init ( &hDecoder -> fb );

 return 1;

}  /* end _NeAACDecInit2 */

static uint8_t window_grouping_info ( _NeAACDecHandle hDecoder, ic_stream* ics ) {
 uint8_t i, g;
 uint8_t sf_index = hDecoder -> sf_index;
 switch ( ics -> window_sequence ) {
  case ONLY_LONG_SEQUENCE :
  case LONG_START_SEQUENCE:
  case LONG_STOP_SEQUENCE :
   ics -> num_windows                                         = 1;
   ics -> num_window_groups                                   = 1;
   ics -> window_group_length[ ics -> num_window_groups - 1 ] = 1;
   ics -> num_swb = num_swb_1024_window[ sf_index ];
   if ( ics -> max_sfb > ics -> num_swb ) return 32;
   for ( i = 0; i < ics -> num_swb; ++i ) {
    ics -> sect_sfb_offset[ 0 ][ i ] = swb_offset_1024_window[ sf_index ][ i ];
    ics -> swb_offset     [ i ]      = swb_offset_1024_window[ sf_index ][ i ];
   }  /* end for */
   ics -> sect_sfb_offset[ 0 ][ ics -> num_swb ] = 1024;
   ics -> swb_offset          [ ics -> num_swb ] = 1024;
   ics -> swb_offset_max                         = 1024;
  return 0;
  case EIGHT_SHORT_SEQUENCE:
   ics -> num_windows                                         = 8;
   ics -> num_window_groups                                   = 1;
   ics -> window_group_length[ ics -> num_window_groups - 1 ] = 1;
   ics -> num_swb                                             = num_swb_128_window[ sf_index ];
   if ( ics -> max_sfb > ics -> num_swb ) return 32;
   for ( i = 0; i < ics -> num_swb; ++i ) ics -> swb_offset[ i ] = swb_offset_128_window[ sf_index ][ i ];
   ics -> swb_offset[ ics -> num_swb ] = 128;
   ics -> swb_offset_max               = 128;
   for ( i = 0; i < ics -> num_windows - 1; ++i ) {
    if (  bit_set ( ics -> scale_factor_grouping, 6 - i ) == 0  ) {
     ics -> num_window_groups                                  += 1;
     ics -> window_group_length[ ics -> num_window_groups - 1 ] = 1;
    } else ics -> window_group_length[ ics -> num_window_groups - 1 ] += 1;
   }  /* end for */
   for ( g = 0; g < ics -> num_window_groups; ++g ) {
    uint16_t width;
    uint8_t  sect_sfb = 0;
    uint16_t offset   = 0;
    for ( i = 0; i < ics -> num_swb; ++i ) {
     if ( i + 1 == ics -> num_swb )
      width = 128 - swb_offset_128_window[ sf_index ][ i ];
     else width = swb_offset_128_window[ sf_index ][ i + 1 ] -
                  swb_offset_128_window[ sf_index ][ i ];
     width *= ics -> window_group_length[ g ];
     ics -> sect_sfb_offset[ g ][ sect_sfb++ ] = offset;
     offset += width;
    }  /* end for */
    ics -> sect_sfb_offset[ g ][ sect_sfb ] = offset;
   }  /* end for */
  return 0;
  default: return 32;
 }  /* end switch */
}  /* end window_grouping_info */

static uint8_t max_pred_sfb ( const uint8_t sr_index ) {
 static const uint8_t pred_sfb_max[] __attribute__(   (  section( ".rodata" )  )   ) = {
  33, 33, 38, 40, 40, 40, 41, 41, 37, 37, 37, 34
 };
 if ( sr_index < 12 ) return pred_sfb_max[ sr_index ];
 return 0;
}  /* end max_pred_sfb */

static uint8_t ics_info (
                _NeAACDecHandle hDecoder, ic_stream* ics, SMS_BitContext* ld, uint8_t common_window
               ) {
 uint8_t retval = 0;
 uint8_t ics_reserved_bit;
 ics_reserved_bit = SMS_GetBit (ld );
 if ( ics_reserved_bit != 0 ) return 32;
 ics -> window_sequence = ( uint8_t )SMS_GetBits ( ld, 2 );
 ics -> window_shape    = SMS_GetBit ( ld );
 if ( ics -> window_sequence == EIGHT_SHORT_SEQUENCE ) {
  ics -> max_sfb               = ( uint8_t )SMS_GetBits ( ld, 4 );
  ics -> scale_factor_grouping = ( uint8_t )SMS_GetBits ( ld, 7 );
 } else ics -> max_sfb = ( uint8_t )SMS_GetBits ( ld, 6 );
 if (   (  retval = window_grouping_info ( hDecoder, ics )  ) > 0   ) return retval;
 if ( ics -> max_sfb > ics -> num_swb ) return 16;
 if ( ics -> window_sequence != EIGHT_SHORT_SEQUENCE ) {
  if (   (  ics -> predictor_data_present = SMS_GetBit ( ld )  )   ) {
   if ( hDecoder -> object_type == FAAD_MAIN ) {
    uint8_t sfb;
    uint8_t limit = min (  ics -> max_sfb, max_pred_sfb ( hDecoder -> sf_index )  );
    ics -> pred.limit = limit;
    if (   (  ics -> pred.predictor_reset = SMS_GetBit ( ld  )  )   )
     ics -> pred.predictor_reset_group_number = ( uint8_t )SMS_GetBits ( ld, 5 );
    for ( sfb = 0; sfb < limit; ++sfb )
     ics -> pred.prediction_used[ sfb ] = SMS_GetBit ( ld );
   }  /* end if */
  }  /* end if */
 }  /* end if */
 return retval;
}  /* end ics_info */

static uint8_t section_data ( _NeAACDecHandle hDecoder, ic_stream* ics, SMS_BitContext* ld ) {
 uint8_t g;
 uint8_t sect_esc_val, sect_bits;
 if ( ics -> window_sequence == EIGHT_SHORT_SEQUENCE )
  sect_bits = 3;
 else sect_bits = 5;
 sect_esc_val = ( 1 << sect_bits ) - 1;
 for ( g = 0; g < ics -> num_window_groups; ++g ) {
  uint8_t k = 0;
  uint8_t i = 0;
  while ( k < ics -> max_sfb ) {
   uint8_t  sfb;
   uint8_t  sect_len_incr;
   uint16_t sect_len     = 0;
   uint8_t  sect_cb_bits = 4;
   ics -> sect_cb[ g ][ i ] = ( uint8_t )SMS_GetBits ( ld, sect_cb_bits );
   if ( ics -> sect_cb[ g ][ i ] == 12        ) return 32;
   if ( ics -> sect_cb[ g ][ i ] == NOISE_HCB ) ics -> noise_used = 1;
   if ( ics -> sect_cb[ g ][ i ] == INTENSITY_HCB2 || ics -> sect_cb[ g ][ i ] == INTENSITY_HCB ) ics -> is_used = 1;
   sect_len_incr = ( uint8_t )SMS_GetBits ( ld, sect_bits );
   while ( sect_len_incr == sect_esc_val ) {
    sect_len     += sect_len_incr;
    sect_len_incr = ( uint8_t )SMS_GetBits ( ld, sect_bits );
   }  /* end while */
   sect_len += sect_len_incr;
   ics -> sect_start[ g ][ i ] = k;
   ics -> sect_end  [ g ][ i ] = k + sect_len;
   if ( ics -> window_sequence == EIGHT_SHORT_SEQUENCE ) {
    if ( k + sect_len >  8 * 15 ) return 15;
    if ( i            >= 8 * 15 ) return 15;
   } else {
    if ( k + sect_len >  MAX_SFB ) return 15;
    if ( i            >= MAX_SFB ) return 15;
   }  /* end else */
   for ( sfb = k; sfb < k + sect_len; ++sfb ) ics -> sfb_cb[ g ][ sfb ] = ics -> sect_cb[ g ][ i ];
   k += sect_len;
   i += 1;
  }  /* end while */
  ics -> num_sec[ g ] = i;
  if ( k != ics -> max_sfb ) return 32;
 }  /* end for */
 return 0;
}  /* end section_data */

static int8_t huffman_scale_factor ( SMS_BitContext* ld ) {
 uint16_t offset = 0;
 while ( hcb_sf[ offset ][ 1 ] ) {
  uint8_t b = SMS_GetBit ( ld );
  offset += hcb_sf[ offset ][ b ];
  if ( offset > 240 ) return -1;
 }  /* end while */
 return hcb_sf[ offset ][ 0 ];
}  /* end huffman_scale_factor */

static uint8_t decode_scale_factors ( ic_stream* ics, SMS_BitContext* ld ) {
 uint8_t g, sfb;
 int16_t t;
 int8_t  noise_pcm_flag = 1;
 int16_t scale_factor   = ics -> global_gain;
 int16_t is_position    = 0;
 int16_t noise_energy   = ics -> global_gain - 90;
 for ( g = 0; g < ics -> num_window_groups; ++g ) {
  for ( sfb = 0; sfb < ics -> max_sfb; ++sfb ) {
   switch ( ics -> sfb_cb[ g ][ sfb ] ) {
    case ZERO_HCB:
     ics -> scale_factors[ g ][ sfb ] = 0;
    break;
    case INTENSITY_HCB:
    case INTENSITY_HCB2:
     t                                 = huffman_scale_factor ( ld );
     is_position                      += t - 60;
     ics -> scale_factors[ g ][ sfb ]  = is_position;
    break;
    case NOISE_HCB:
     if ( noise_pcm_flag ) {
      noise_pcm_flag = 0;
      t              = ( int16_t )SMS_GetBits ( ld, 9 ) - 256;
     } else {
      t              = huffman_scale_factor ( ld );
      t             -= 60;
     }  /* end else */
     noise_energy                    += t;
     ics -> scale_factors[ g ][ sfb ] = noise_energy;
    break;
    default:
     ics -> scale_factors[ g ][ sfb ] = 0;
     t                                = huffman_scale_factor ( ld );
     scale_factor                    += t - 60;
     if ( scale_factor < 0 || scale_factor > 255 ) return 4;
     ics -> scale_factors[ g ][ sfb ] = scale_factor;
    break;
   }  /* end switch */
  }  /* end for */
 }  /* end for */
 return 0;
}  /* end decode_scale_factors */

static uint8_t SMS_INLINE scale_factor_data ( _NeAACDecHandle hDecoder, ic_stream* ics, SMS_BitContext* ld ) {
 return decode_scale_factors ( ics, ld );
}  /* end scale_factor_data */

static uint8_t pulse_data ( ic_stream* ics, pulse_info* pul, SMS_BitContext* ld ) {
 uint8_t i;
 pul -> number_pulse    = ( uint8_t )SMS_GetBits ( ld, 2 );
 pul -> pulse_start_sfb = ( uint8_t )SMS_GetBits ( ld, 6 );
 if ( pul -> pulse_start_sfb > ics -> num_swb ) return 16;
 for ( i = 0; i < pul -> number_pulse+1; ++i ) {
  pul -> pulse_offset[ i ] = ( uint8_t )SMS_GetBits ( ld, 5 );
  pul -> pulse_amp   [ i ] = ( uint8_t )SMS_GetBits ( ld, 4 );
 }  /* end for */
 return 0;
}  /* end pulse_data */

static void tns_data ( ic_stream* ics, tns_info* tns, SMS_BitContext* ld ) {
 uint8_t w, filt, i, start_coef_bits = 0, coef_bits;
 uint8_t n_filt_bits = 2;
 uint8_t length_bits = 6;
 uint8_t order_bits  = 5;
 if ( ics -> window_sequence == EIGHT_SHORT_SEQUENCE ) {
  n_filt_bits = 1;
  length_bits = 4;
  order_bits  = 3;
 }  /* end if */
 for ( w = 0; w < ics -> num_windows; ++w ) {
  tns -> n_filt[ w ] = ( uint8_t )SMS_GetBits ( ld, n_filt_bits );
  if ( tns -> n_filt[ w ] ) {
   if (   (  tns -> coef_res[ w ] = SMS_GetBit ( ld )  )   )
    start_coef_bits = 4;
   else start_coef_bits = 3;
  }  /* end if */
  for ( filt = 0; filt < tns -> n_filt[ w ]; ++filt ) {
   tns -> length[ w ][ filt ] = ( uint8_t )SMS_GetBits ( ld, length_bits );
   tns -> order [ w ][ filt ] = ( uint8_t )SMS_GetBits ( ld, order_bits  );
   if ( tns -> order[ w ][ filt ] ) {
    tns -> direction    [ w ][ filt ] = SMS_GetBit ( ld );
    tns -> coef_compress[ w ][ filt ] = SMS_GetBit ( ld );
    coef_bits = start_coef_bits - tns -> coef_compress[ w ][ filt ];
    for ( i = 0; i < tns -> order[ w ][ filt ]; ++i )
     tns -> coef[ w ][ filt ][ i ] = ( uint8_t )SMS_GetBits ( ld, coef_bits );
   }  /* end if */
  }  /* end for */
 }  /* end for */
}  /* end tns_data */

static uint8_t side_info (
                _NeAACDecHandle hDecoder, element* ele, SMS_BitContext* ld, ic_stream* ics, uint8_t scal_flag
               ) {
 uint8_t result;
 ics -> global_gain = ( uint8_t )SMS_GetBits ( ld, 8 );
 if ( !ele -> common_window && !scal_flag ) {
  if (   (  result = ics_info ( hDecoder, ics, ld, ele -> common_window )  ) > 0   ) return result;
 }  /* end if */
 if (   (  result = section_data      ( hDecoder, ics, ld )  ) > 0   ) return result;
 if (   (  result = scale_factor_data ( hDecoder, ics, ld )  ) > 0   ) return result;
 if ( !scal_flag ) {
  if (   (  ics -> pulse_data_present = SMS_GetBit ( ld )  )   ) {
   if (   (  result = pulse_data ( ics, &ics -> pul, ld )  ) > 0   ) return result;
  }  /* end if */
  if (   (  ics -> tns_data_present          = SMS_GetBit ( ld )  )   ) tns_data ( ics, &ics -> tns, ld );
  if (   (  ics -> gain_control_data_present = SMS_GetBit ( ld )  )   ) return 1;
 }  /* end if */
 return 0;
}  /* end side_info */

static uint8_t huffman_2step_quad ( uint8_t cb, SMS_BitContext* ld, int16_t* sp ) {
 uint32_t cw;
 uint16_t offset = 0;
 uint8_t  extra_bits;
 SMS_OPEN_READER( r, ld );
 SMS_UPDATE_CACHE( r, ld );
 cw         = SMS_SHOW_UBITS( r, ld, hcbN[ cb ] );
 offset     = hcb_table[ cb ][ cw ].offset;
 extra_bits = hcb_table[ cb ][ cw ].extra_bits;
 if ( extra_bits ) {
  SMS_SKIP_BITS( r, ld, hcbN[ cb ] );
  offset += ( uint16_t )SMS_SHOW_UBITS( r, ld, extra_bits );
  SMS_SKIP_BITS( r, ld, hcb_2_quad_table[ cb ][ offset ].bits - hcbN[ cb ] );
 } else SMS_SKIP_BITS( r, ld, hcb_2_quad_table[ cb ][ offset ].bits );
 SMS_CLOSE_READER( r, ld );
 if ( offset > hcb_2_quad_table_size[ cb ] ) return 10;
 sp[ 0 ] = hcb_2_quad_table[ cb ][ offset ].x;
 sp[ 1 ] = hcb_2_quad_table[ cb ][ offset ].y;
 sp[ 2 ] = hcb_2_quad_table[ cb ][ offset ].v;
 sp[ 3 ] = hcb_2_quad_table[ cb ][ offset ].w;
 return 0;
}  /* end huffman_2step_quad */

static uint8_t huffman_binary_quad ( uint8_t cb, SMS_BitContext* ld, int16_t* sp ) {
 uint16_t offset = 0;
 while (  !hcb3[ offset ].is_leaf ) {
  uint8_t b = SMS_GetBit ( ld );
  offset += hcb3[ offset ].data[ b ];
 }  /* end while */
 if ( offset > hcb_bin_table_size[ cb ] ) return 10;
 sp[ 0 ] = hcb3[ offset ].data[ 0 ];
 sp[ 1 ] = hcb3[ offset ].data[ 1 ];
 sp[ 2 ] = hcb3[ offset ].data[ 2 ];
 sp[ 3 ] = hcb3[ offset ].data[ 3 ];
 return 0;
}  /* end huffman_binary_quad */

static void huffman_sign_bits ( SMS_BitContext* ld, int16_t* sp, uint8_t len ) {
 uint8_t i;
 for ( i = 0; i < len; ++i )
  if (  sp[ i ] && SMS_GetBit ( ld )  ) sp[ i ] = -sp[ i ];
}  /* end huffman_sign_bits */

static uint8_t huffman_binary_quad_sign ( uint8_t cb, SMS_BitContext* ld, int16_t* sp ) {
 uint8_t err = huffman_binary_quad ( cb, ld, sp );
 huffman_sign_bits ( ld, sp, QUAD_LEN );
 return err;
}  /* end huffman_binary_quad_sign */

static uint8_t huffman_2step_quad_sign ( uint8_t cb, SMS_BitContext* ld, int16_t* sp ) {
 uint8_t err = huffman_2step_quad ( cb, ld, sp );
 huffman_sign_bits ( ld, sp, QUAD_LEN );
 return err;
}  /* end huffman_2step_quad_sign */

static uint8_t huffman_binary_pair ( uint8_t cb, SMS_BitContext* ld, int16_t* sp ) {
 uint16_t offset = 0;
 while ( !hcb_bin_table[ cb ][ offset ].is_leaf ) {
  uint8_t b = SMS_GetBit ( ld );
  offset += hcb_bin_table[ cb ][ offset ].data[ b ];
 }  /* end while */
 if ( offset > hcb_bin_table_size[ cb ] ) return 10;
 sp[ 0 ] = hcb_bin_table[ cb ][ offset ].data[ 0 ];
 sp[ 1 ] = hcb_bin_table[ cb ][ offset ].data[ 1 ];
 return 0;
}  /* end huffman_binary_pair */

static uint8_t huffman_2step_pair ( uint8_t cb, SMS_BitContext* ld, int16_t* sp ) {
 uint32_t cw;
 uint16_t offset = 0;
 uint8_t  extra_bits;
 SMS_OPEN_READER( r, ld );
 SMS_UPDATE_CACHE( r, ld );
 cw         = SMS_SHOW_UBITS( r, ld, hcbN[ cb ] );
 offset     = hcb_table[ cb ][ cw ].offset;
 extra_bits = hcb_table[ cb ][ cw ].extra_bits;
 if ( extra_bits ) {
  SMS_SKIP_BITS( r, ld, hcbN[ cb ] );
  offset += ( uint16_t )SMS_SHOW_UBITS( r, ld, extra_bits );
  SMS_SKIP_BITS( r, ld, hcb_2_pair_table[ cb ][ offset ].bits - hcbN[ cb ] );
 } else SMS_SKIP_BITS( r, ld, hcb_2_pair_table[ cb ][ offset ].bits );
 SMS_CLOSE_READER( r, ld );
 if ( offset > hcb_2_pair_table_size[ cb ] ) return 10;
 sp[ 0 ] = hcb_2_pair_table[ cb ][ offset ].x;
 sp[ 1 ] = hcb_2_pair_table[ cb ][ offset ].y;
 return 0;
}  /* end huffman_2step_pair */

static uint8_t huffman_binary_pair_sign ( uint8_t cb, SMS_BitContext* ld, int16_t* sp ) {
 uint8_t err = huffman_binary_pair ( cb, ld, sp );
 huffman_sign_bits ( ld, sp, PAIR_LEN );
 return err;
}  /* end huffman_binary_pair_sign */

static uint8_t huffman_2step_pair_sign ( uint8_t cb, SMS_BitContext* ld, int16_t* sp ) {
 uint8_t err = huffman_2step_pair ( cb, ld, sp );
 huffman_sign_bits ( ld, sp, PAIR_LEN );
 return err;
}  /* end huffman_2step_pair_sign */

static int16_t huffman_codebook ( uint8_t i ) {
 static const uint32_t data = 16428320;
 if ( i == 0 )
  return ( int16_t )( data >> 16 ) & 0xFFFF;
 else return ( int16_t )data & 0xFFFF;
}  /* end huffman_codebook */

static int16_t huffman_getescape ( SMS_BitContext* ld, int16_t sp ) {
 uint8_t neg, i;
 int16_t j;
 int16_t off;
 if ( sp < 0 ) {
  if ( sp != -16 ) return sp;
  neg = 1;
 } else {
  if ( sp != 16 ) return sp;
  neg = 0;
 }  /* end else */
 for ( i = 4;; ++i ) if (  !SMS_GetBit ( ld )  ) break;
 off = ( int16_t )SMS_GetBits ( ld, i );
 j   = off | ( 1 << i );
 if ( neg ) j = -j;
 return j;
}  /* end huffman_getescape */

static uint8_t huffman_spectral_data ( uint8_t cb, SMS_BitContext* ld, int16_t* sp ) {
 switch ( cb ) {
  case  1:
  case  2: return huffman_2step_quad       ( cb, ld, sp );
  case  3: return huffman_binary_quad_sign ( cb, ld, sp );
  case  4: return huffman_2step_quad_sign  ( cb, ld, sp );
  case  5: return huffman_binary_pair      ( cb, ld, sp );
  case  6: return huffman_2step_pair       ( cb, ld, sp );
  case  7:
  case  9: return huffman_binary_pair_sign ( cb, ld, sp );
  case  8:
  case 10: return huffman_2step_pair_sign  ( cb, ld, sp );
  case 12: {
   uint8_t err = huffman_2step_pair ( 11, ld, sp );
   sp[ 0 ] = huffman_codebook ( 0 );
   sp[ 1 ] = huffman_codebook ( 1 ); 
   return err;
  }  // 12
  case 11: {
   uint8_t err = huffman_2step_pair_sign ( 11, ld, sp );
   sp[ 0 ] = huffman_getescape ( ld, sp[ 0 ] );
   sp[ 1 ] = huffman_getescape ( ld, sp[ 1 ] );
   return err;
  }  // 11
  default: return 11;
 }  /* end switch */
 return 0;
}  /* end huffman_spectral_data */

static uint8_t spectral_data (
                _NeAACDecHandle hDecoder, ic_stream* ics, SMS_BitContext* ld, int16_t* spectral_data
               ) {
 int8_t   i;
 uint8_t  g;
 uint16_t inc, k, p = 0;
 uint8_t  groups = 0;
 uint8_t  sect_cb;
 uint8_t  result;
 for ( g = 0; g < ics -> num_window_groups; ++g ) {
  p = groups * 128;
  for ( i = 0; i < ics -> num_sec[ g ]; ++i ) {
   sect_cb = ics -> sect_cb[ g ][ i ];
   inc     = ( sect_cb >= FIRST_PAIR_HCB ) ? 2 : 4;
   switch ( sect_cb ) {
    case ZERO_HCB:
    case NOISE_HCB:
    case INTENSITY_HCB:
    case INTENSITY_HCB2:
     p += ( ics -> sect_sfb_offset[ g ][  ics -> sect_end  [ g ][ i ]  ] -
            ics -> sect_sfb_offset[ g ][  ics -> sect_start[ g ][ i ]  ]
          );
    break;
    default:
     for ( k = ics -> sect_sfb_offset[ g ][  ics -> sect_start[ g ][ i ]  ];
           k < ics -> sect_sfb_offset[ g ][  ics -> sect_end  [ g ][ i ]  ];
           k += inc
     ) {
      if (   (  result = huffman_spectral_data ( sect_cb, ld, &spectral_data[ p ] )  ) > 0   ) return result;
      p += inc;
     }  /* end for */
    break;
   }  /* end switch */
  }  /* end for */
  groups += ics -> window_group_length[ g ];
 }  /* end for */
 return 0;
}  /* end spectral_data */

static uint8_t pulse_decode ( ic_stream* ics, int16_t* spec_data ) {
 uint8_t     i;
 uint16_t    k;
 pulse_info* pul = &ics -> pul;
 k = min( ics -> swb_offset[ pul -> pulse_start_sfb ], ics -> swb_offset_max );
 for ( i = 0; i <= pul -> number_pulse; ++i ) {
  k += pul -> pulse_offset[ i ];
  if ( k >= 1024 ) return 15;
  if ( spec_data[ k ] > 0 )
   spec_data[ k ] += pul -> pulse_amp[ i ];
  else spec_data[ k ] -= pul -> pulse_amp[ i ];
 }  /* end for */
 return 0;
}  /* end pulse_decode */

static uint8_t individual_channel_stream (
                _NeAACDecHandle hDecoder, element* ele,
                SMS_BitContext* ld, ic_stream* ics, uint8_t scal_flag,
                int16_t* spec_data
               ) {
 uint8_t result = side_info ( hDecoder, ele, ld, ics, scal_flag );
 if ( result > 0 ) return result;
 if ( hDecoder -> object_type >= ER_OBJECT_START ) {
  if ( ics -> tns_data_present ) tns_data ( ics, &ics -> tns, ld );
 }  /* end if */
 if (   (  result = spectral_data ( hDecoder, ics, ld, spec_data )  ) > 0   ) return result;
 if ( ics -> pulse_data_present ) {
  if ( ics -> window_sequence != EIGHT_SHORT_SEQUENCE ) {
   if (   (  result = pulse_decode ( ics, spec_data )  ) > 0   ) return result;
  } else return 2;
 }  /* end if */
 return 0;
}  /* end individual_channel_stream */

static void SMS_INLINE reset_pred_state ( pred_state* state ) {
 state -> r  [ 0 ] = 0;
 state -> r  [ 1 ] = 0;
 state -> COR[ 0 ] = 0;
 state -> COR[ 1 ] = 0;
 state -> VAR[ 0 ] = 0x3F80;
 state -> VAR[ 1 ] = 0x3F80;
} /* end reset_pred_state */

static void reset_all_predictors ( pred_state* state ) {
 uint16_t i;
 for ( i = 0; i < 1024; ++i ) reset_pred_state ( &state[ i ] );
}  /* end reset_all_predictors */

static uint8_t allocate_single_channel (
                _NeAACDecHandle hDecoder, uint8_t channel, uint8_t output_channels
               ) {
 int mul = 1; 

 if ( hDecoder -> object_type == FAAD_MAIN ) {
  if ( hDecoder -> pred_stat[ channel ] != NULL ) {
   free ( hDecoder -> pred_stat[ channel ] );
   hDecoder -> pred_stat[ channel ] = NULL;
  }  /* end if */
  hDecoder -> pred_stat[ channel ] = ( pred_state* )malloc (  1024 * sizeof ( pred_state )  );
  reset_all_predictors ( hDecoder -> pred_stat[ channel ] );
 }  /* end if */

 if ( hDecoder -> time_out[ channel ] != NULL ) {
  free ( hDecoder -> time_out[ channel ] );
  hDecoder -> time_out[ channel ] = NULL;
 }  /* end if */

 hDecoder -> sbr_alloced[ hDecoder -> fr_ch_ele ] = 0;
 if ( hDecoder -> sbr_present_flag || hDecoder -> forceUpSampling ) {
  mul                                              = 2;
  hDecoder -> sbr_alloced[ hDecoder -> fr_ch_ele ] = 1;
 }  /* end if */

 hDecoder -> time_out[ channel ] = ( float* )calloc (  mul * 1024, sizeof ( float )  );

 if ( output_channels == 2 ) {
  if ( hDecoder -> time_out[ channel + 1 ] != NULL ) {
   free ( hDecoder -> time_out[ channel + 1 ] );
   hDecoder -> time_out[ channel + 1 ] = NULL;
  }  /* end if */
  hDecoder -> time_out[ channel + 1 ] = ( float* )calloc (  1024, sizeof ( float )  );
 }  /* end if */

 if ( hDecoder -> fb_intermed[ channel ] != NULL ) {
  free ( hDecoder -> fb_intermed[ channel ] );
  hDecoder -> fb_intermed[ channel ] = NULL;
 }  /* end if */

 hDecoder -> fb_intermed[ channel ] = ( float* )calloc (  1024, sizeof ( float )  );

 return 0;

}  /* end allocate_single_channel */

void vquant ( short* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "vquant:\n\t"
 "ld        $t0, 0($a0)\n\t"
 "mtsah     $zero, 2\n\t"
 "lui       $a2, %hi( _aac_iq_table )\n\t"
 "addiu     $a2, $a2, %lo( _aac_iq_table )\n\t"
 "pextlw    $a2, $a2, $a2\n\t"
 "pcpyld    $a2, $a2, $a2\n\t"
 "pextlh    $t0, $zero, $t0\n\t"
 "psllw     $t0, $t0, 16\n\t"
 "psraw     $t1, $t0, 31\n\t"
 "psraw     $t0, $t0, 16\n\t"
 "pxor      $t0, $t1, $t0\n\t"
 "psubw     $t0, $t0, $t1\n\t"
 "psllw     $t1, $t1, 31\n\t"
 "psllw     $t0, $t0, 2\n\t"
 "paddw     $a2, $a2, $t0\n\t"
 "lw        $t0, 0($a2)\n\t"
 "qfsrv     $a2, $a2, $a2\n\t"
 "lw        $t2, 0($a2)\n\t"
 "qfsrv     $a2, $a2, $a2\n\t"
 "lw        $t3, 0($a2)\n\t"
 "qfsrv     $a2, $a2, $a2\n\t"
 "lw        $t4, 0($a2)\n\t"
 "pextlw    $t0, $t2, $t0\n\t"
 "pextlw    $t3, $t4, $t3\n\t"
 "pcpyld    $t0, $t3, $t0\n\t"
 "pxor      $t0, $t0, $t1\n\t"
 "qmtc2     $t0, $vf01\n\t"
 "jr        $ra\n\t"
 "vmuli.xyzw    $vf01, $vf01, I\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static void quant_to_spec ( _NeAACDecHandle hDecoder, ic_stream* ics, int16_t* quant_data, float* spec_data ) {

 static const unsigned int pow2_table[] __attribute__(   (  section( ".rodata" )  )   ) = {
  0x3F800000, 0x3F9837F0, 0x3FB504F3, 0x3FD744FD
 };

 uint8_t  g, sfb, win;
 uint16_t width, bin, gindex, wa, wb;
 float    scf;

 gindex = 0;

 for ( g = 0; g < ics -> num_window_groups; ++g ) {
  uint16_t j         = 0;
  uint16_t gincrease = 0;
  uint16_t win_inc   = ics -> swb_offset[ ics -> num_swb ];
  for ( sfb = 0; sfb < ics -> num_swb; ++sfb ) {
   int32_t exp, frac;
   width = ics -> swb_offset[ sfb + 1 ] - ics -> swb_offset[ sfb ];
   if ( ics -> scale_factors[ g ][ sfb ] < 0 || ics -> scale_factors[ g ][ sfb ] > 255 ) {
    exp  = 0;
    frac = 0;
   } else {
    exp  = ics -> scale_factors[ g ][ sfb ] >> 2;
    frac = ics -> scale_factors[ g ][ sfb ]  & 3;
   }  /* end else */
   wa  = gindex + j;
   scf = (  ( float* )pow2sf_tab  )[ exp ] * (  ( float* )pow2_table  )[ frac ];
   __asm__ __volatile__(
    ".set noreorder\n\t"
    ".set noat\n\t"
    "mfc1   $at, %0\n\t"
    "ctc2   $at, $21\n\t"
    ".set at\n\t"
    ".set reorder\n\t"
    :: "f"( scf )
   );
   for ( win = 0; win < ics -> window_group_length[ g ]; ++win ) {
    for ( bin = 0; bin < width; bin += 4 ) {
     wb = wa + bin;
     vquant ( quant_data );
     gincrease  += 4;
     quant_data += 4;
     __asm__ __volatile__(
      "sqc2 $vf01, 0(%0)\n\t"
      :: "r"( &spec_data[ wb ] )
     );
    }  /* end for */
    wa += win_inc;
   }  /* end for */
   j += width;
  }  /* end for */
  gindex += gincrease;
 }  /* end for */

}  /* end quant_to_spec */

static uint8_t SMS_INLINE is_noise ( ic_stream* ics, uint8_t group, uint8_t sfb ) {
 return ics -> sfb_cb[ group ][ sfb ] == NOISE_HCB;
}  /* end is_noise */

static uint32_t SMS_INLINE ne_rng ( uint32_t* __r1, uint32_t* __r2 ) {
 uint32_t t1, t2, t3, t4;
 t3   = t1 = *__r1;
 t4   = t2 = *__r2;
 t1  &= 0xF5;
 t2 >>= 25;
 t1   = Parity[ t1 ];
 t2  &= 0x63;
 t1 <<= 31;
 t2   = Parity[ t2 ];
 return (  *__r1 = ( t3 >> 1 ) | t1  ) ^ (  *__r2 = ( t4 + t4 ) | t2  );
}  /* end ne_rng */

static void gen_rand_vector (
             float* spec, int16_t scale_factor, uint16_t size,
             uint8_t sub, uint32_t* __r1, uint32_t* __r2
            ) {

 uint16_t i;
 float    energy = 0.0F;
 float    scale  = ( float )1.0F / ( float )size;

 for ( i = 0; i < size; ++i ) {
  float tmp = scale * ( float )( int32_t )ne_rng ( __r1, __r2 );
  spec[ i ] = tmp;
  energy   += tmp * tmp;
 }  /* end for */

 scale  = ( float )1.0F / ( float )sqrtf ( energy );
 scale *= ( float )PowF ( 2.0F, 0.25F * scale_factor );

 for ( i = 0; i < size; ++i ) spec[ i ] *= scale;

}  /* end gen_rand_vector */

static void pns_decode (
             ic_stream* ics_left, ic_stream* ics_right,
             float* spec_left, float* spec_right,
             uint8_t channel_pair, uint8_t object_type,
             uint32_t* __r1, uint32_t* __r2
            ) {

 uint8_t  g, sfb, b;
 uint16_t size, offs;
 uint8_t  group  = 0;
 uint16_t nshort = 128;
 uint8_t  sub    = 0;

 for ( g = 0; g < ics_left -> num_window_groups; ++g ) {
  for ( b = 0; b < ics_left -> window_group_length[ g ]; ++b ) {
   for ( sfb = 0; sfb < ics_left -> max_sfb; ++sfb ) {
    if (  is_noise ( ics_left, g, sfb )  ) {
     ics_left -> pred.prediction_used[ sfb ] = 0;
     offs = ics_left -> swb_offset[ sfb ];
     size = min( ics_left -> swb_offset[ sfb + 1 ], ics_left -> swb_offset_max ) - offs;
     gen_rand_vector (
      &spec_left[ ( group * nshort ) + offs ], ics_left -> scale_factors[ g ][ sfb ], size, sub, __r1, __r2
     );
    }  /* end if */
    if ( channel_pair ) {
     if (  is_noise ( ics_right, g, sfb )  ) {
      if (   (  ( ics_left -> ms_mask_present == 1 ) &&
                ( ics_left -> ms_used[ g ][ sfb ]  )
             ) || ( ics_left -> ms_mask_present == 2 )
      ) {
       uint16_t c;
       offs = ics_right -> swb_offset[ sfb ];
       size = min( ics_right -> swb_offset[ sfb + 1 ], ics_right -> swb_offset_max) - offs;
       for ( c = 0; c < size; ++c ) spec_right[ ( group * nshort ) + offs + c ] = spec_left[ ( group * nshort ) + offs + c ];
      } else {
       ics_right -> pred.prediction_used[ sfb ] = 0;
       offs = ics_right->swb_offset[sfb];
       size = min( ics_right -> swb_offset[ sfb + 1 ], ics_right -> swb_offset_max ) - offs;
       gen_rand_vector (
        &spec_right[ ( group * nshort ) + offs ], ics_right -> scale_factors[ g ][ sfb ], size, sub, __r1, __r2
       );
      }  /* end else */
     }  /* end if */
    }  /* end if */
   }  /* end for */
   ++group;
  }  /* end for */
 }  /* end for */

}  /* end pns_decode */

static float SMS_INLINE inv_quant_pred ( int16_t q ) {
 float     x;
 uint32_t* tmp = ( uint32_t* )&x;
 *tmp = (  ( uint32_t )q  ) << 16;
 return x;
}  /* end inv_quant_pred */

static void SMS_INLINE flt_round ( float* pf ) {
 int32_t  flg;
 uint32_t tmp, tmp1, tmp2;
 tmp  = *( uint32_t* )pf;
 flg  = tmp & ( uint32_t )0x00008000;
 tmp &= ( uint32_t )0xFFFF0000;
 tmp1 = tmp;
 if ( flg ) {
  tmp &= ( uint32_t )0xFF800000;
  tmp |= ( uint32_t )0x00010000;
  tmp2 = tmp;
  tmp &= ( uint32_t )0xFF800000;
  *pf = *( float* )&tmp1 + *( float* )&tmp2 - *( float* )&tmp;
 } else *pf = *( float* )&tmp;
}  /* end flt_round */

static int16_t SMS_INLINE quant_pred ( float x ) {
 int16_t   q;
 uint32_t* tmp = ( uint32_t* )&x;
 q = ( int16_t )( *tmp >> 16 );
 return q;
}  /* end quant_pred */

static void ic_predict ( pred_state* state, float input, float* output, uint8_t pred ) {

 uint16_t tmp;
 int16_t  i, j;
 float    dr1;
 float    predictedvalue;
 float    e0, e1;
 float    k1, k2;
 float    r  [ 2 ];
 float    COR[ 2 ];
 float    VAR[ 2 ];

 r[ 0 ] = inv_quant_pred ( state -> r[ 0 ] );
 r[ 1 ] = inv_quant_pred ( state -> r[ 1 ] );

 COR[ 0 ] = inv_quant_pred ( state -> COR[ 0 ] );
 COR[ 1 ] = inv_quant_pred ( state -> COR[ 1 ] );
 VAR[ 0 ] = inv_quant_pred ( state -> VAR[ 0 ] );
 VAR[ 1 ] = inv_quant_pred ( state -> VAR[ 1 ] );

 tmp = state -> VAR[ 0 ];

 j = ( tmp >> 7 );
 i = tmp & 0x7F;

 if ( j >= 128 ) {
  j -= 128;
  k1 = COR[ 0 ] * (  ( float* )exp_table  )[ j ] * (  ( float* )mnt_table  )[ i ];
 } else k1 = 0.0F;

 if ( pred ) {
  tmp = state -> VAR[ 1 ];
  j   = tmp >> 7;
  i   = tmp & 0x7F;
  if ( j >= 128 ) {
   j -= 128;
   k2 = COR[ 1 ] * (  ( float* )exp_table  )[ j ] * (  ( float* )mnt_table  )[ i ];
  } else k2 = 0.0F;
  predictedvalue = k1 * r[ 0 ] + k2 * r[ 1 ];
  flt_round ( &predictedvalue );
  *output = input + predictedvalue;
 }  /* end if */

 e0  = *output;
 e1  = e0 - k1 * r[ 0 ];
 dr1 = k1 * e0;

 VAR[ 0 ] = ALPHA * VAR[ 0 ] + 0.5f * ( r[ 0 ] * r[ 0 ] + e0 * e0 );
 COR[ 0 ] = ALPHA * COR[ 0 ] + r[ 0 ] * e0;
 VAR[ 1 ] = ALPHA * VAR[ 1 ] + 0.5F * ( r[ 1 ] * r[ 1 ] + e1 * e1 );
 COR[ 1 ] = ALPHA * COR[ 1 ] + r[ 1 ] * e1;

 r[ 1 ] = A * ( r[ 0 ] - dr1 );
 r[ 0 ] = A * e0;

 state -> r  [ 0 ] = quant_pred ( r  [ 0 ] );
 state -> r  [ 1 ] = quant_pred ( r  [ 1 ] );
 state -> COR[ 0 ] = quant_pred ( COR[ 0 ] );
 state -> COR[ 1 ] = quant_pred ( COR[ 1 ] );
 state -> VAR[ 0 ] = quant_pred ( VAR[ 0 ] );
 state -> VAR[ 1 ] = quant_pred ( VAR[ 1 ] );

}  /* end ic_predict */

static void ic_prediction ( ic_stream* ics, float* spec, pred_state *state, uint8_t sf_index ) {

 uint8_t sfb;
 uint16_t bin;

 if ( ics -> window_sequence == EIGHT_SHORT_SEQUENCE )
  reset_all_predictors ( state );
 else {
  for (  sfb = 0; sfb < max_pred_sfb ( sf_index ); ++sfb  ) {
   uint16_t low  = ics -> swb_offset[ sfb ];
   uint16_t high = min( ics -> swb_offset[ sfb + 1 ], ics -> swb_offset_max );
   for ( bin = low; bin < high; ++bin )
    ic_predict (
     &state[ bin ], spec[ bin ], &spec[ bin ], ( ics -> predictor_data_present && ics -> pred.prediction_used[ sfb ] )
    );
  }  /* end for */
  if ( ics -> predictor_data_present ) {
   if ( ics -> pred.predictor_reset ) {
    for ( bin = ics -> pred.predictor_reset_group_number - 1; bin < 1024; bin += 30 ) reset_pred_state ( &state[ bin ] );
   }  /* end if */
  }  /* end if */
 }  /* end else */

}  /* end ic_prediction */

static void pns_reset_pred_state ( ic_stream* ics, pred_state* state ) {
 uint8_t  sfb, g, b;
 uint16_t i, offs, offs2;
 if ( ics -> window_sequence == EIGHT_SHORT_SEQUENCE ) return;
 for ( g = 0; g < ics -> num_window_groups; ++g ) {
  for ( b = 0; b < ics -> window_group_length[ g ]; ++b ) {
   for ( sfb = 0; sfb < ics -> max_sfb; ++sfb ) {
    if (  is_noise ( ics, g, sfb )  ) {
     offs  = ics -> swb_offset[ sfb ];
     offs2 = min( ics -> swb_offset[ sfb + 1 ], ics -> swb_offset_max );
     for ( i = offs; i < offs2; ++i ) reset_pred_state ( &state[ i ] );
    }  /* end if */
   }  /* end for */
  }  /* end for */
 }  /* end for */
}  /* end pns_reset_pred_state */

static void tns_decode_coef (
             uint8_t order, uint8_t coef_res_bits, uint8_t coef_compress,
             uint8_t* coef, float* a
            ) {

 uint8_t i, m;
 float   tmp2[ TNS_MAX_ORDER + 1 ], b[ TNS_MAX_ORDER + 1 ];

 for ( i = 0; i < order; ++i ) {
  if ( coef_compress == 0 ) {
   if ( coef_res_bits == 3 )
    tmp2[ i ] = tns_coef_0_3[  coef[ i ]  ];
   else tmp2[i] = tns_coef_0_4[  coef[ i ]  ];
  } else {
   if ( coef_res_bits == 3 )
    tmp2[ i ] = tns_coef_1_3[  coef[ i ]  ];
   else tmp2[ i ] = tns_coef_1_4[  coef[ i ]  ];
  }  /* end else */
 }  /* end for */

 a[ 0 ] = 1.0F;

 for ( m = 1; m <= order; ++m ) {
  for ( i = 1; i < m; ++i ) b[ i ] = a[ i ] + tmp2[ m - 1 ] * a[ m - i ];
  for ( i = 1; i < m; ++i ) a[ i ] = b[ i ];
  a[ m ] = tmp2[ m - 1 ];
 }  /* end for */

}  /* end tns_decode_coef */

static uint8_t SMS_INLINE max_tns_sfb (
                           const uint8_t sr_index, const uint8_t object_type, const uint8_t is_short
                          ) {
 static const uint8_t tns_sbf_max[][ 4 ] __attribute__(   (  section( ".rodata" )  )   ) = {
  { 31,  9, 28, 7 }, { 31,  9, 28, 7 },
  { 34, 10, 27, 7 }, { 40, 14, 26, 6 },
  { 42, 14, 26, 6 }, { 51, 14, 26, 6 },
  { 46, 14, 29, 7 }, { 46, 14, 29, 7 },
  { 42, 14, 23, 8 }, { 42, 14, 23, 8 },
  { 42, 14, 23, 8 }, { 39, 14, 19, 7 },
  { 39, 14, 19, 7 }, {  0,  0,  0, 0 },
  {  0,  0,  0,  0}, {  0,  0,  0,  0}
 };
 uint8_t i = 0;
 if ( is_short                ) i += 1;
 if ( object_type == FAAD_SSR ) i += 2;
 return tns_sbf_max[ sr_index ][ i ];
}  /* end max_tns_sfb */

static void tns_ar_filter ( float* spectrum, uint16_t size, int8_t inc, float* lpc, uint8_t order ) {
 uint8_t  j;
 uint16_t i;
 float    y;
 float    state[ 2 * TNS_MAX_ORDER ] = { 0 };
 int8_t   state_index = 0;
 for ( i = 0; i < size; ++i ) {
  y = *spectrum;
  for ( j = 0; j < order; ++j ) y -= state[ state_index + j ] * lpc[ j + 1 ];
  if ( --state_index < 0 ) state_index = order - 1;
  state[ state_index ] = state[ state_index + order ] = y;
  *spectrum = y;
  spectrum += inc;
 }  /* end for */
}  /* end tns_ar_filter */

static void tns_decode_frame ( ic_stream* ics, tns_info* tns, uint8_t sr_index, uint8_t object_type, float* spec ) {

 uint8_t  w, f, tns_order;
 int8_t   inc;
 int16_t  size;
 uint16_t bottom, top, start, end;
 uint16_t nshort = 128;
 float    lpc[ TNS_MAX_ORDER + 1 ];

 if ( !ics -> tns_data_present ) return;

 for ( w = 0; w < ics->num_windows; ++w ) {
  bottom = ics -> num_swb;
  for ( f = 0; f < tns -> n_filt[ w ]; ++f ) {
   top       = bottom;
   bottom    = max( top - tns -> length[ w ][ f ], 0 );
   tns_order = min( tns -> order[ w ][ f ], TNS_MAX_ORDER );
   if ( !tns_order ) continue;
   tns_decode_coef (
    tns_order, tns -> coef_res[ w ] + 3, tns -> coef_compress[ w ][ f ], tns -> coef[ w ][ f ], lpc
   );
   start = min(   bottom, max_tns_sfb (  sr_index, object_type, ( ics -> window_sequence == EIGHT_SHORT_SEQUENCE )  )   );
   start = min( start, ics -> max_sfb );
   start = min( ics -> swb_offset[ start ], ics -> swb_offset_max );
   end   = min(   top, max_tns_sfb (  sr_index, object_type, ( ics -> window_sequence == EIGHT_SHORT_SEQUENCE )  )   );
   end   = min( end, ics -> max_sfb );
   end   = min( ics -> swb_offset[ end ], ics -> swb_offset_max );
   size  = end - start;
   if ( size <= 0 ) continue;
   if ( tns -> direction[ w ][ f ] ) {
    inc   = -1;
    start = end - 1;
   } else inc = 1;
   tns_ar_filter ( &spec[ ( w * nshort ) + start ], size, inc, lpc, tns_order );
  }  /* end for */
 }  /* end for */

}  /* end tns_decode_frame */

static void drc_decode ( drc_info* drc, float* spec ) {

 uint16_t i, bd, top;
 float    factor, exp;
 uint16_t bottom = 0;

 if ( drc -> num_bands == 1 ) drc -> band_top[ 0 ] = 1024 / 4 - 1;

 for ( bd = 0; bd < drc -> num_bands; ++bd ) {
  top = 4 * ( drc -> band_top[ bd ] + 1 );
  if ( drc -> dyn_rng_sgn[ bd ] )
   exp = -drc -> ctrl1 * (  drc -> dyn_rng_ctl[ bd ] - ( DRC_REF_LEVEL - drc -> prog_ref_level )  ) / 24.0F;
  else exp = drc -> ctrl2 * (  drc -> dyn_rng_ctl[ bd ] - ( DRC_REF_LEVEL - drc -> prog_ref_level )  ) / 24.0F;
  factor = ( float )PowF ( 2.0F, exp );
  for ( i = bottom; i < top; ++i ) spec[ i ] *= factor;
  bottom = top;
 }  /* end for */

}  /* end drc_decode */

void _overlap_0 ( float*, const float*, const float*, const float*, const float* );
void _overlap_1 ( float*, const float*, const float*                             );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_overlap_0:\n\t"
 "addiu     $t1, $a3, 496\n\t"
 "addiu     $t2, $zero, 128\n\t"
 "mtsah     $zero, -2\n\t"
 "1:\n\t"
 "lqc2      $vf01,    0($a2)\n\t"
 "lqc2      $vf02,    0($t0)\n\t"
 "lqc2      $vf03, 1792($a1)\n\t"
 "vmula.xyzw    ACC, $vf01, $vf02\n\t"
 "vmaddw.xyzw   $vf01, $vf03, $vf00\n\t"
 "lqc2      $vf02,  512($a2)\n\t"
 "lq        $at, 0($t1)\n\t"
 "lqc2      $vf03, 1024($a2)\n\t"
 "pexew     $at, $at\n\t"
 "lqc2      $vf04, 0($a3)\n\t"
 "qfsrv     $at, $at, $at\n\t"
 "lqc2      $vf05, 2304($a1)\n\t"
 "qmtc2     $at, $vf06\n\t"
 "slti      $at, $t2, 68\n\t"
 "vmula.xyzw    ACC, $vf02, $vf06\n\t"
 "vmadda.xyzw   ACC, $vf03, $vf04\n\t"
 "vmaddw.xyzw   $vf02, $vf05, $vf00\n\t"
 "lqc2      $vf03, 1536($a2)\n\t"
 "lqc2      $vf05, 2048($a2)\n\t"
 "lqc2      $vf07, 2816($a1)\n\t"
 "vmula.xyzw    ACC, $vf03, $vf06\n\t"
 "vmadda.xyzw   ACC, $vf05, $vf04\n\t"
 "vmaddw.xyzw   $vf03, $vf07, $vf00\n\t"
 "lqc2      $vf05, 2560($a2)\n\t"
 "lqc2      $vf07, 3072($a2)\n\t"
 "lqc2      $vf08, 3328($a1)\n\t"
 "vmula.xyzw    ACC, $vf05, $vf06\n\t"
 "vmadda.xyzw   ACC, $vf07, $vf04\n\t"
 "vmaddw.xyzw   $vf05, $vf08, $vf00\n\t"
 "bne       $at, $zero, 2f\n\t"
 "addiu     $t2, $t2, -4\n\t"
 "lqc2      $vf07, 3584($a2)\n\t"
 "lqc2      $vf08, 4096($a2)\n\t"
 "lqc2      $vf09, 3840($a1)\n\t"
 "vmula.xyzw    ACC, $vf07, $vf06\n\t"
 "vmadda.xyzw   ACC, $vf08, $vf04\n\t"
 "vmaddw.xyzw   $vf06, $vf09, $vf00\n\t"
 "2:\n\t"
 "sqc2      $vf01, 1792($a0)\n\t"
 "sqc2      $vf02, 2304($a0)\n\t"
 "sqc2      $vf03, 2816($a0)\n\t"
 "sqc2      $vf05, 3328($a0)\n\t"
 "beql      $at, $zero, 2f\n\t"
 "sqc2      $vf06, 3840($a0)\n\t"
 "2:\n\t"
 "addiu     $a0, $a0,  16\n\t"
 "addiu     $a1, $a1,  16\n\t"
 "addiu     $a2, $a2,  16\n\t"
 "addiu     $a3, $a3,  16\n\t"
 "addiu     $t0, $t0,  16\n\t"
 "bgtz      $t2, 1b\n\t"
 "addiu     $t1, $t1, -16\n\t"
 "jr        $ra\n\t"
 "_overlap_1:\n\t"
 "addiu     $a3, $a2, 496\n\t"
 "addiu     $t0, $zero, 128\n\t"
 "1:\n\t"
 "slti      $at, $t0, 68\n\t"
 "lq        $t1, 0($a3)\n\t"
 "lqc2      $vf01, 0($a2)\n\t"
 "pexew     $t1, $t1\n\t"
 "qfsrv     $t1, $t1, $t1\n\t"
 "beq       $at, $zero, 2f\n\t"
 "qmtc2     $t1, $vf02\n\t"
 "lqc2      $vf03, 3584($a1)\n\t"
 "lqc2      $vf04, 4096($a1)\n\t"
 "vmula.xyzw    ACC, $vf03, $vf02\n\t"
 "vmadd.xyzw    $vf03, $vf04, $vf01\n\t"
 "2:\n\t"
 "lqc2      $vf04, 4608($a1)\n\t"
 "lqc2      $vf05, 5120($a1)\n\t"
 "vmula.xyzw    ACC, $vf04, $vf02\n\t"
 "vmadd.xyzw    $vf04, $vf05, $vf01\n\t"
 "lqc2      $vf05, 5632($a1)\n\t"
 "lqc2      $vf06, 6144($a1)\n\t"
 "vmula.xyzw    ACC, $vf05, $vf02\n\t"
 "vmadd.xyzw    $vf05, $vf06, $vf01\n\t"
 "lqc2      $vf06, 6656($a1)\n\t"
 "lqc2      $vf07, 7168($a1)\n\t"
 "lqc2      $vf08, 7680($a1)\n\t"
 "vmula.xyzw    ACC, $vf06, $vf02\n\t"
 "vmadd.xyzw    $vf06, $vf07, $vf01\n\t"
 "vmul.xyzw     $vf08, $vf08, $vf02\n\t"
 "bnel      $at, $zero, 2f\n\t"
 "sqc2      $vf03, -256($a0)\n\t"
 "2:\n\t"
 "addiu     $t0, $t0, -4\n\t"
 "sqc2      $vf04,  256($a0)\n\t"
 "sqc2      $vf05,  768($a0)\n\t"
 "sqc2      $vf06, 1280($a0)\n\t"
 "sqc2      $vf08, 1792($a0)\n\t"
 "addiu     $a0, $a0,  16\n\t"
 "addiu     $a1, $a1,  16\n\t"
 "addiu     $a2, $a2,  16\n\t"
 "bgtz      $t0, 1b\n\t"
 "addiu     $a3, $a3, -16\n\t"
 "jr        $ra\n\t"
 "nop\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static void ifilter_bank (
             _NeAACDecHandle hDecoder, uint8_t window_sequence, uint8_t window_shape,
             uint8_t window_shape_prev, float* freq_in,
             float* time_out, float* overlap
            ) {

 fb_info* fb         = &hDecoder -> fb;
 float*   transf_buf = hDecoder -> transf_buf;
 int16_t  i;

 const float* window_long       = fb -> long_window [ window_shape      ];
 const float* window_long_prev  = fb -> long_window [ window_shape_prev ];
 const float* window_short      = fb -> short_window[ window_shape      ];
 const float* window_short_prev = fb -> short_window[ window_shape_prev ];

 switch ( window_sequence ) {

  case ONLY_LONG_SEQUENCE:
   DSP_IMDCT ( &fb -> mdct2048, transf_buf, freq_in );
   DSP_VecMULA ( time_out, transf_buf, window_long_prev, overlap, 1024 );
   DSP_VecMULR ( overlap, &transf_buf[ 1024 ], window_long, 1024 );
  break;

  case LONG_START_SEQUENCE:
   DSP_IMDCT ( &fb -> mdct2048, transf_buf, freq_in );
   DSP_VecMULA ( time_out, transf_buf, window_long_prev, overlap, 1024 );
   memcpy (  overlap, &transf_buf[ 1024 ], 448 * sizeof ( float )  );
   DSP_VecMULR ( &overlap[ 448 ], &transf_buf[ 1024 + 448 ], window_short, 128 );
   memset ( &overlap[ 448 + 128 ], 0, 448 * sizeof ( float )  );
  break;

  case EIGHT_SHORT_SEQUENCE:
   DSP_IMDCT ( &fb -> mdct256, transf_buf + 2 * 128 * 0, freq_in + 0 * 128 );
   DSP_IMDCT ( &fb -> mdct256, transf_buf + 2 * 128 * 1, freq_in + 1 * 128 );
   DSP_IMDCT ( &fb -> mdct256, transf_buf + 2 * 128 * 2, freq_in + 2 * 128 );
   DSP_IMDCT ( &fb -> mdct256, transf_buf + 2 * 128 * 3, freq_in + 3 * 128 );
   DSP_IMDCT ( &fb -> mdct256, transf_buf + 2 * 128 * 4, freq_in + 4 * 128 );
   DSP_IMDCT ( &fb -> mdct256, transf_buf + 2 * 128 * 5, freq_in + 5 * 128 );
   DSP_IMDCT ( &fb -> mdct256, transf_buf + 2 * 128 * 6, freq_in + 6 * 128 );
   DSP_IMDCT ( &fb -> mdct256, transf_buf + 2 * 128 * 7, freq_in + 7 * 128 );
   memcpy (  time_out, overlap, 448 * sizeof ( float )  );
   _overlap_0 ( time_out, overlap, transf_buf, window_short, window_short_prev );
   _overlap_1 ( overlap, transf_buf, window_short );
   memset (  &overlap[ 448 + 128 ], 0, 448 * sizeof ( float )  );
  break;

  case LONG_STOP_SEQUENCE:
   DSP_IMDCT ( &fb -> mdct2048, transf_buf, freq_in );
   memcpy (  time_out, overlap, 448 * sizeof ( float )  );
   DSP_VecMULA ( &time_out[ 448 ], &transf_buf[ 448 ], window_short_prev, &overlap[ 448 ], 128 );
   for ( i = 0; i < 448; ++i ) time_out[ 448 + 128 + i ] = overlap[ 448 + 128 + i ] + transf_buf[ 448 + 128 + i ];
   DSP_VecMULR ( overlap, &transf_buf[ 1024 ], window_long, 1024 );
  break;

 }  /* end switch */

}  /* end ifilter_bank */

static uint8_t reconstruct_single_channel (
                _NeAACDecHandle hDecoder, ic_stream* ics,
                element* sce, int16_t* spec_data
               ) {

 float   spec_coef[ 1024 ];
 uint8_t retval;
 int     output_channels;

 output_channels = 1;

 if ( hDecoder -> element_output_channels[ hDecoder -> fr_ch_ele ] == 0 )
  hDecoder -> element_output_channels[ hDecoder -> fr_ch_ele ] = output_channels;
 else if ( hDecoder -> element_output_channels[ hDecoder -> fr_ch_ele ] != output_channels ) {
  hDecoder -> element_alloced        [ hDecoder -> fr_ch_ele ] = 0;
  hDecoder -> element_output_channels[ hDecoder -> fr_ch_ele ] = output_channels;
 }  /* end if */

 if ( hDecoder -> element_alloced[ hDecoder -> fr_ch_ele ] == 0 ) {
  retval = allocate_single_channel ( hDecoder, sce -> channel, output_channels );
  if ( retval > 0 ) return retval;
  hDecoder -> element_alloced[ hDecoder -> fr_ch_ele ] = 1;
 }  /* end if */

 quant_to_spec ( hDecoder, ics, spec_data, spec_coef );

 pns_decode (
  ics, NULL, spec_coef, NULL, 0, hDecoder -> object_type,
  &hDecoder -> __r1, &hDecoder -> __r2
 );

 if ( hDecoder -> object_type == FAAD_MAIN ) {
  ic_prediction ( ics, spec_coef, hDecoder -> pred_stat[ sce -> channel ], hDecoder -> sf_index );
  pns_reset_pred_state ( ics, hDecoder -> pred_stat[ sce -> channel ] );
 }  /* end if */

 tns_decode_frame ( ics, &ics -> tns, hDecoder -> sf_index, hDecoder -> object_type, spec_coef );

 if ( hDecoder -> drc.present ) {
  if ( !hDecoder -> drc.exclude_mask[ sce -> channel ] ||
       !hDecoder -> drc.excluded_chns_present
  ) drc_decode ( &hDecoder -> drc, spec_coef );
 }  /* end if */

 ifilter_bank (
  hDecoder, ics -> window_sequence, ics -> window_shape,
  hDecoder -> window_shape_prev[ sce -> channel ],
  spec_coef,
  hDecoder -> time_out   [ sce -> channel ],
  hDecoder -> fb_intermed[ sce -> channel ]
 );

 hDecoder -> window_shape_prev[ sce -> channel ] = ics -> window_shape;

 if (  ( hDecoder -> sbr_present_flag || hDecoder -> forceUpSampling == 1 ) && hDecoder -> sbr_alloced[ hDecoder -> fr_ch_ele ]  ) {
  int ele = hDecoder -> fr_ch_ele;
  int ch  = sce -> channel;
  if ( hDecoder -> sbr[ ele ] == NULL ) {
   hDecoder -> sbr[ ele ] = sbrDecodeInit (
    1024, hDecoder -> element_id[ ele ], 2 * get_sample_rate ( hDecoder -> sf_index ),
    hDecoder -> downSampledSBR
   );
  }  /* end if */
  if ( sce -> ics1.window_sequence == EIGHT_SHORT_SEQUENCE )
   hDecoder -> sbr[ ele ] -> maxAACLine = 8 * min (  sce -> ics1.swb_offset[ max ( sce -> ics1.max_sfb - 1, 0 ) ], sce -> ics1.swb_offset_max  );
  else hDecoder -> sbr[ ele ] -> maxAACLine = min (  sce -> ics1.swb_offset[ max ( sce -> ics1.max_sfb - 1, 0 ) ], sce -> ics1.swb_offset_max  );
  retval = sbrDecodeSingleFrame (
   hDecoder -> sbr[ ele ], hDecoder -> time_out[ ch ],
   hDecoder -> postSeekResetFlag, hDecoder -> downSampledSBR
  );
  if ( retval > 0 ) return retval;
 } else if (  ( hDecoder -> sbr_present_flag || hDecoder -> forceUpSampling ) && !hDecoder -> sbr_alloced [ hDecoder -> fr_ch_ele ] ) return 23;

 return 0;

}  /* end reconstruct_single_channel */

static uint8_t allocate_channel_pair (
                _NeAACDecHandle hDecoder, uint8_t channel, uint8_t paired_channel
               ) {
 int mul = 1;

 if ( hDecoder -> object_type == FAAD_MAIN ) {
  if ( hDecoder -> pred_stat[ channel ] == NULL ) {
   hDecoder -> pred_stat[ channel ] = ( pred_state* )malloc (  1024 * sizeof ( pred_state )  );
   reset_all_predictors ( hDecoder -> pred_stat[ channel ] );
  }  /* end if */
  if ( hDecoder -> pred_stat[ paired_channel ] == NULL ) {
   hDecoder -> pred_stat[ paired_channel ] = ( pred_state* )malloc (  1024 * sizeof ( pred_state )  );
   reset_all_predictors ( hDecoder -> pred_stat[ paired_channel ] );
  }  /* end if */
 }  /* end if */

 if ( hDecoder -> time_out[ channel ] == NULL ) {

  hDecoder -> sbr_alloced[ hDecoder -> fr_ch_ele ] = 0;
  if ( hDecoder -> sbr_present_flag == 1 || hDecoder -> forceUpSampling == 1 ) {
   mul                                              = 2;
   hDecoder -> sbr_alloced[ hDecoder -> fr_ch_ele ] = 1;
  }  /* end if */

  hDecoder -> time_out[ channel ] = ( float* )calloc (  mul * 1024, sizeof ( float )  );
 }  /* end if */
 if ( hDecoder -> time_out[ paired_channel ] == NULL )
  hDecoder -> time_out[ paired_channel ] = ( float* )calloc (  mul * 1024, sizeof ( float )  );
 if ( hDecoder -> fb_intermed[ channel ] == NULL )
  hDecoder -> fb_intermed[ channel ] = ( float* )calloc (  1024, sizeof ( float )  );
 if ( hDecoder -> fb_intermed[ paired_channel ] == NULL )
  hDecoder -> fb_intermed[ paired_channel ] = ( float* )calloc (  1024, sizeof ( float )  );

 return 0;

}  /* end allocate_channel_pair */

static int8_t SMS_INLINE is_intensity ( ic_stream* ics, uint8_t group, uint8_t sfb ) {
 switch ( ics -> sfb_cb[ group ][ sfb ] ) {
  case INTENSITY_HCB : return  1;
  case INTENSITY_HCB2: return -1;
  default            : return  0;
 }  /* end switch */
}  /* end is_intensity */

static void ms_decode ( ic_stream* ics, ic_stream* icsr, float* l_spec, float* r_spec ) {
 uint8_t  g, b, sfb;
 uint8_t  group  = 0;
 uint16_t i, k;
 float    tmp;
 if ( ics -> ms_mask_present >= 1 ) {
  for ( g = 0; g < ics -> num_window_groups; ++g )  {
   for ( b = 0; b < ics -> window_group_length[ g ]; ++b ) {
    for ( sfb = 0; sfb < ics -> max_sfb; ++sfb ) {
     if (  ( ics -> ms_used[ g ][ sfb ] || ics -> ms_mask_present == 2 ) &&
           !is_intensity ( icsr, g, sfb ) && !is_noise ( ics, g, sfb )
     ) {
      for (  i = ics -> swb_offset[ sfb ]; i < min( ics -> swb_offset[ sfb + 1 ], ics -> swb_offset_max ); ++i  ) {
       k = ( group * 128 ) + i;
       tmp = l_spec[ k ] - r_spec[ k ];
       l_spec[ k ] = l_spec[ k ] + r_spec[ k ];
       r_spec[ k ] = tmp;
      }  /* end for */
     }  /* end if */
    }  /* end for */
    ++group;
   }  /* end for */
  }  /* end for */
 }  /* end if */
}  /* end ms_decode */

static int8_t SMS_INLINE invert_intensity ( ic_stream* ics, uint8_t group, uint8_t sfb ) {
 if ( ics -> ms_mask_present == 1 ) return 1 - 2 * ics -> ms_used[ group ][ sfb ];
 return 1;
}  /* end invert_intensity */

static void is_decode ( ic_stream* ics, ic_stream* icsr, float* l_spec, float* r_spec ) {
 uint8_t  g, sfb, b;
 uint16_t i;
 float    scale;
 uint8_t  group  = 0;
 for ( g = 0; g < icsr -> num_window_groups; ++g ) {
  for ( b = 0; b < icsr -> window_group_length[ g ]; ++b ) {
   for ( sfb = 0; sfb < icsr -> max_sfb; ++sfb ) {
    if (  is_intensity ( icsr, g, sfb )  ) {
     ics  -> pred.prediction_used[ sfb ] = 0;
     icsr -> pred.prediction_used[ sfb ] = 0;
     scale = ( float )PowF (  0.5F, ( 0.25F * icsr -> scale_factors[ g ][ sfb ] )  );
     for (  i = icsr -> swb_offset[ sfb ]; i < min( icsr -> swb_offset[ sfb + 1 ], ics -> swb_offset_max ); ++i  ) {
      r_spec[ ( group * 128 ) + i ] = l_spec[ ( group * 128 ) + i ] * scale;
      if (  is_intensity ( icsr, g, sfb ) != invert_intensity ( ics, g, sfb )  ) r_spec[ ( group * 128 ) + i ] = -r_spec[ ( group * 128 ) + i ];
     }  /* end for */
    }  /* end if */
   }  /* end for */
   ++group;
  }  /* end for */
 }  /* end for */
}  /* end is_decode */

static uint16_t data_stream_element ( _NeAACDecHandle hDecoder, SMS_BitContext* ld ) {
 uint8_t  byte_aligned;
 uint16_t i, count;
 SMS_GetBits ( ld, LEN_TAG );
 byte_aligned = SMS_GetBit ( ld );
 count        = ( uint16_t )SMS_GetBits ( ld, 8 );
 if ( count == 255 ) count += ( uint16_t )SMS_GetBits ( ld, 8 );
 if ( byte_aligned ) SMS_AlignBits ( ld );
 for ( i = 0; i < count; ++i ) SMS_GetBits ( ld, 8 );
 return count;
}  /* end data_stream_element */

static uint8_t excluded_channels ( SMS_BitContext* ld, drc_info* drc ) {
 uint8_t i, n = 0;
 uint8_t num_excl_chan = 7;
 for ( i = 0; i < 7; ++i )
  drc -> exclude_mask[ i ] = SMS_GetBit ( ld );
 ++n;
 while (   (  drc -> additional_excluded_chns[ n - 1 ] = SMS_GetBit ( ld )  )   ) {
  for ( i = num_excl_chan; i < num_excl_chan + 7; ++i )
   drc -> exclude_mask[ i ] = SMS_GetBit ( ld );
  ++n;
  num_excl_chan += 7;
 }  /* end while */
 return n;
}  /* end excluded_channels */

static uint8_t dynamic_range_info ( SMS_BitContext* ld, drc_info* drc ) {
 uint8_t i, n = 1;
 uint8_t band_incr;
 drc -> num_bands = 1;
 if (  SMS_GetBit ( ld )  ) {
  drc -> pce_instance_tag = ( uint8_t )SMS_GetBits ( ld, 4 );
  SMS_GetBits ( ld, 4 );
  ++n;
 }  /* end if */
 drc -> excluded_chns_present = SMS_GetBit ( ld );
 if ( drc -> excluded_chns_present == 1 ) n += excluded_channels ( ld, drc );
 if (  SMS_GetBit ( ld )  ) {
  band_incr = ( uint8_t )SMS_GetBits ( ld, 4 );
  SMS_GetBits ( ld, 4 );
  ++n;
  drc -> num_bands += band_incr;
  for ( i = 0; i < drc -> num_bands; ++i ) {
   drc -> band_top[ i ] = ( uint8_t )SMS_GetBits ( ld, 8 );
   ++n;
  }  /* end for */
 }  /* end if */
 if (  SMS_GetBit ( ld )  ) {
  drc -> prog_ref_level = ( uint8_t )SMS_GetBits ( ld, 7 );
  SMS_GetBit ( ld );
  ++n;
 }  /* end if */
 for ( i = 0; i < drc -> num_bands; ++i ) {
  drc -> dyn_rng_sgn[ i ] = SMS_GetBit ( ld );
  drc -> dyn_rng_ctl[ i ] = ( uint8_t )SMS_GetBits ( ld, 7 );
  ++n;
 }  /* end for */
 return n;
}  /* end dynamic_range_info */

#define ANC_DATA           0
#define EXT_FIL            0
#define EXT_FILL_DATA      1
#define EXT_DATA_ELEMENT   2
#define EXT_DYNAMIC_RANGE 11

static uint16_t extension_payload ( SMS_BitContext* ld, drc_info* drc, uint16_t count ) {
 uint16_t i, n, dataElementLength;
 uint8_t  dataElementLengthPart;
 uint8_t  align = 4, data_element_version, loopCounter;
 uint8_t  extension_type = ( uint8_t )SMS_GetBits ( ld, 4 );
 switch ( extension_type ) {
  case EXT_DYNAMIC_RANGE:
   drc -> present = 1;
   n = dynamic_range_info ( ld, drc );
  return n;
  case EXT_FILL_DATA:
   SMS_GetBits ( ld, 4 );
   for ( i = 0; i < count - 1; ++i ) SMS_GetBits ( ld, 8 );
  return count;
  case EXT_DATA_ELEMENT:
   data_element_version = ( uint8_t )SMS_GetBits ( ld, 4 );
   switch ( data_element_version ) {
    case ANC_DATA:
     loopCounter       = 0;
     dataElementLength = 0;
     do {
      dataElementLengthPart = ( uint8_t )SMS_GetBits ( ld, 8 );
      dataElementLength    += dataElementLengthPart;
      ++loopCounter;
     } while ( dataElementLengthPart == 255 );
     for ( i = 0; i < dataElementLength; ++i ) SMS_GetBits ( ld, 8 );
    return dataElementLength+loopCounter + 1;
    default: align = 0;
   }  /* end switch */
  case EXT_FIL:
  default     :
   SMS_GetBits ( ld, align );
   for ( i = 0; i < count - 1; ++i ) SMS_GetBits ( ld, 8 );
  return count;
 }  /* end switch */
}  /* end extension_payload */

static uint8_t fill_element ( _NeAACDecHandle hDecoder, SMS_BitContext* ld, drc_info* drc, uint8_t sbr_ele ) {
 uint16_t count = ( uint16_t )SMS_GetBits ( ld, 4 );
 uint8_t  bs_extension_type;
 if ( count == 15 ) count += ( uint16_t )SMS_GetBits ( ld, 8 ) - 1;
 if ( count > 0 ) {
  bs_extension_type = ( uint8_t )SMS_ShowBits ( ld, 4 );
  if (  bs_extension_type == EXT_SBR_DATA ||
        bs_extension_type == EXT_SBR_DATA_CRC
  ) {
   if ( sbr_ele == INVALID_SBR_ELEMENT ) return 24;
   if ( !hDecoder -> sbr[ sbr_ele ] )
    hDecoder -> sbr[ sbr_ele ] = sbrDecodeInit (
     1024, hDecoder -> element_id[ sbr_ele ],
     2 * get_sample_rate ( hDecoder -> sf_index ), hDecoder -> downSampledSBR
    );
   hDecoder -> sbr_present_flag = 1;
   hDecoder -> sbr[ sbr_ele ] -> ret = sbr_extension_data (
    ld, hDecoder -> sbr[ sbr_ele ], count, hDecoder -> postSeekResetFlag
   );
  } else while ( count > 0 ) {
   count -= extension_payload ( ld, drc, count );
  }  /* end while */
 }  /* end if */
 return 0;
}  /* end fill_element */

static uint8_t reconstruct_channel_pair (
                _NeAACDecHandle hDecoder, ic_stream* ics1, ic_stream* ics2, element* cpe, int16_t* spec_data1, int16_t* spec_data2
               ) {

 float   spec_coef1[ 1024 ];
 float   spec_coef2[ 1024 ];
 uint8_t retval;

 if ( hDecoder -> element_alloced[ hDecoder -> fr_ch_ele ] == 0 ) {
  retval = allocate_channel_pair (  hDecoder, cpe -> channel, ( uint8_t )cpe -> paired_channel  );
  if ( retval > 0 ) return retval;
  hDecoder -> element_alloced[ hDecoder -> fr_ch_ele ] = 1;
 }  /* end if */
 quant_to_spec ( hDecoder, ics1, spec_data1, spec_coef1 );
 quant_to_spec ( hDecoder, ics2, spec_data2, spec_coef2 );
 if ( ics1 -> ms_mask_present )
  pns_decode (
   ics1, ics2, spec_coef1, spec_coef2, 1, hDecoder -> object_type, &hDecoder -> __r1, &hDecoder -> __r2
  );
 else {
  pns_decode (
   ics1, NULL, spec_coef1, NULL, 0, hDecoder -> object_type, &hDecoder -> __r1, &hDecoder -> __r2
  );
  pns_decode (
   ics2, NULL, spec_coef2, NULL, 0, hDecoder -> object_type, &hDecoder -> __r1, &hDecoder -> __r2
  );
 }  /* end else */
 ms_decode ( ics1, ics2, spec_coef1, spec_coef2 );
 is_decode ( ics1, ics2, spec_coef1, spec_coef2 );
 if ( hDecoder -> object_type == FAAD_MAIN ) {
  ic_prediction ( ics1, spec_coef1, hDecoder -> pred_stat[ cpe -> channel        ], hDecoder -> sf_index );
  ic_prediction ( ics2, spec_coef2, hDecoder -> pred_stat[ cpe -> paired_channel ], hDecoder -> sf_index );
  pns_reset_pred_state ( ics1, hDecoder -> pred_stat[ cpe -> channel        ] );
  pns_reset_pred_state ( ics2, hDecoder -> pred_stat[ cpe -> paired_channel ] );
 }  /* end if */
 tns_decode_frame ( ics1, &ics1 -> tns, hDecoder -> sf_index, hDecoder -> object_type, spec_coef1 );
 tns_decode_frame ( ics2, &ics2 -> tns, hDecoder -> sf_index, hDecoder -> object_type, spec_coef2 );
 if ( hDecoder -> drc.present ) {
  if ( !hDecoder -> drc.exclude_mask[ cpe -> channel ] ||
       !hDecoder -> drc.excluded_chns_present
  ) drc_decode ( &hDecoder -> drc, spec_coef1 );
  if ( !hDecoder -> drc.exclude_mask[ cpe -> paired_channel ] ||
       !hDecoder -> drc.excluded_chns_present
  ) drc_decode ( &hDecoder -> drc, spec_coef2 );
 }  /* end if */
 ifilter_bank (
  hDecoder, ics1 -> window_sequence, ics1 -> window_shape,
  hDecoder -> window_shape_prev[ cpe -> channel ], spec_coef1,
  hDecoder -> time_out[ cpe -> channel], hDecoder -> fb_intermed[ cpe -> channel ]
 );
 ifilter_bank (
  hDecoder, ics2 -> window_sequence, ics2 -> window_shape,
  hDecoder -> window_shape_prev[ cpe -> paired_channel ], spec_coef2,
  hDecoder -> time_out[ cpe -> paired_channel ], hDecoder -> fb_intermed[ cpe -> paired_channel ]
 );
 hDecoder -> window_shape_prev[ cpe -> channel        ] = ics1 -> window_shape;
 hDecoder -> window_shape_prev[ cpe -> paired_channel ] = ics2 -> window_shape;

 if (  ( hDecoder -> sbr_present_flag || hDecoder -> forceUpSampling ) && hDecoder -> sbr_alloced[ hDecoder -> fr_ch_ele ]  ) {
  int ele = hDecoder -> fr_ch_ele;
  int ch0 = cpe -> channel;
  int ch1 = cpe -> paired_channel;
  if ( hDecoder -> sbr[ ele ] == NULL ) {
   hDecoder -> sbr[ ele ] = sbrDecodeInit (
    1024, hDecoder -> element_id[ ele ], 2 * get_sample_rate ( hDecoder -> sf_index ),
    hDecoder -> downSampledSBR
   );
  }  /* end for */
  if ( cpe -> ics1.window_sequence == EIGHT_SHORT_SEQUENCE )
   hDecoder -> sbr[ ele ] -> maxAACLine = 8 * min (  cpe -> ics1.swb_offset[ max ( cpe -> ics1.max_sfb - 1, 0 ) ], cpe -> ics1.swb_offset_max  );
  else hDecoder -> sbr[ ele ] -> maxAACLine = min (  cpe -> ics1.swb_offset[ max ( cpe -> ics1.max_sfb - 1, 0 ) ], cpe -> ics1.swb_offset_max  );
  retval = sbrDecodeCoupleFrame (
   hDecoder -> sbr[ ele ],
   hDecoder -> time_out[ ch0 ], hDecoder -> time_out[ ch1 ],
   hDecoder -> postSeekResetFlag, hDecoder -> downSampledSBR
  );
  if ( retval > 0 ) return retval;
 } else if (  ( hDecoder -> sbr_present_flag || hDecoder -> forceUpSampling == 1 ) && !hDecoder -> sbr_alloced[ hDecoder -> fr_ch_ele ]  ) return 23;

 return 0;

}  /* end reconstruct_channel_pair */

static uint8_t channel_pair_element (
                _NeAACDecHandle hDecoder, SMS_BitContext* ld, uint8_t channels, uint8_t* tag
               ) {
 int16_t    spec_data1[ 1024 ] = { 0 };
 int16_t    spec_data2[ 1024 ] = { 0 };
 element*   cpe  = &hDecoder -> m_Element;
 ic_stream* ics1 = &cpe -> ics1;
 ic_stream* ics2 = &cpe -> ics2;
 uint8_t    result;
 cpe -> channel        = channels;
 cpe -> paired_channel = channels + 1;
 cpe -> element_instance_tag = ( uint8_t )SMS_GetBits ( ld, LEN_TAG );
 *tag = cpe -> element_instance_tag;
 if (   (  cpe -> common_window = SMS_GetBit ( ld )  )   ) {
  if (   (  result = ics_info ( hDecoder, ics1, ld, cpe -> common_window )  ) > 0   ) return result;
  ics1 -> ms_mask_present = ( uint8_t )SMS_GetBits ( ld, 2 );
  if ( ics1 -> ms_mask_present == 3 ) return 32;
  if ( ics1 -> ms_mask_present == 1 ) {
   uint8_t g, sfb;
   for ( g = 0; g < ics1 -> num_window_groups; ++g ) {
    for ( sfb = 0; sfb < ics1 -> max_sfb; ++sfb ) {
     ics1 -> ms_used[ g ][ sfb ] = SMS_GetBit ( ld );
    }  /* end for */
   }  /* end for */
  }  /* end if */
  memcpy (  ics2, ics1, sizeof ( ic_stream )  );
 } else ics1 -> ms_mask_present = 0;
 if (   (  result = individual_channel_stream ( hDecoder, cpe, ld, ics1, 0, spec_data1 )  ) > 0   ) return result;
 if (   (  result = individual_channel_stream ( hDecoder, cpe, ld, ics2, 0, spec_data2 )  ) > 0   ) return result;
 if (  SMS_ShowBits ( ld, LEN_SE_ID ) == ID_FIL  ) {
  SMS_SkipBits ( ld, LEN_SE_ID );
  if (   (  result = fill_element ( hDecoder, ld, &hDecoder -> drc, hDecoder -> fr_ch_ele )  ) > 0   ) return result;
 }  /* end if */
 if (   (  result = reconstruct_channel_pair ( hDecoder, ics1, ics2, cpe, spec_data1, spec_data2 )  ) > 0   ) return result;
 return 0;
}  /* end channel_pair_element */

static void decode_cpe (
             _NeAACDecHandle hDecoder, _NeAACDecFrameInfo* hInfo, SMS_BitContext* ld, uint8_t id_syn_ele
            ) {
 uint8_t channels = hDecoder -> fr_channels;
 uint8_t tag      = 0;
 if ( channels + 2 > MAX_CHANNELS ) {
  hInfo -> error = 12;
  return;
 }  /* end if */
 if ( hDecoder -> fr_ch_ele + 1 > MAX_SYNTAX_ELEMENTS ) {
  hInfo -> error = 13;
  return;
 }  /* end if */
 if ( hDecoder -> element_output_channels[ hDecoder -> fr_ch_ele ] == 0 )
  hDecoder -> element_output_channels[ hDecoder -> fr_ch_ele ] = 2;
 else if ( hDecoder -> element_output_channels[ hDecoder -> fr_ch_ele ] != 2 ) {
  hInfo -> error = 21;
  return;
 }  /* end if */
 hDecoder -> element_id[ hDecoder -> fr_ch_ele] = id_syn_ele;
 hInfo -> error = channel_pair_element ( hDecoder, ld, channels, &tag );
 if ( hDecoder -> pce_set ) {
  hDecoder -> internal_channel[ hDecoder -> pce.cpe_channel[ tag ]      ] = channels;
  hDecoder -> internal_channel[ hDecoder -> pce.cpe_channel[ tag ] + 1  ] = channels + 1;
 } else {
  hDecoder -> internal_channel[ channels     ] = channels;
  hDecoder -> internal_channel[ channels + 1 ] = channels + 1;
 }  /* end else */
 hDecoder -> fr_channels += 2;
 hDecoder -> fr_ch_ele   += 1;
}  /* end decode_cpe */

static uint8_t single_lfe_channel_element (
                _NeAACDecHandle hDecoder, SMS_BitContext* ld, uint8_t channel, uint8_t* tag
               ) {
 uint8_t    retval            = 0;
 element*   sce               = &hDecoder -> m_Element;
 ic_stream* ics               = &sce -> ics1;
 int16_t    spec_data[ 1024 ] = { 0 };
 sce -> element_instance_tag = ( uint8_t )SMS_GetBits ( ld, LEN_TAG );
 sce -> common_window        = 0;
 *tag = sce -> element_instance_tag;
 sce -> channel        = channel;
 sce -> paired_channel = -1;
 retval             = individual_channel_stream ( hDecoder, sce, ld, ics, 0, spec_data );
 if ( retval > 0 ) return retval;
 if ( ics -> is_used ) return 32;
 if (  SMS_ShowBits ( ld, LEN_SE_ID ) == ID_FIL  ) {
  SMS_SkipBits ( ld, LEN_SE_ID );
  if (   (  retval = fill_element ( hDecoder, ld, &hDecoder -> drc, hDecoder -> fr_ch_ele )  ) > 0   ) return retval;
 }  /* end if */
 retval = reconstruct_single_channel ( hDecoder, ics, sce, spec_data );
 if ( retval > 0 ) return retval;
 return 0;
}  /* end single_lfe_channel_element */

static void decode_sce_lfe (
             _NeAACDecHandle hDecoder, _NeAACDecFrameInfo* hInfo, SMS_BitContext* ld, uint8_t id_syn_ele
            ) {
 uint8_t channels = hDecoder -> fr_channels;
 uint8_t tag      = 0;
 if ( channels + 1 > MAX_CHANNELS ) {
  hInfo -> error = 12;
  return;
 }  /* end if */
 if ( hDecoder -> fr_ch_ele + 1 > MAX_SYNTAX_ELEMENTS ) {
  hInfo -> error = 13;
  return;
 }  /* end if */
 hDecoder -> element_id[ hDecoder -> fr_ch_ele ] = id_syn_ele;
 hInfo -> error = single_lfe_channel_element ( hDecoder, ld, channels, &tag );
 if ( hDecoder -> element_output_channels[ hDecoder -> fr_ch_ele ] == 2 ) {
  hDecoder -> internal_channel[ channels     ] = channels;
  hDecoder -> internal_channel[ channels + 1 ] = channels + 1;
 } else {
  if ( hDecoder -> pce_set )
   hDecoder -> internal_channel[  hDecoder -> pce.sce_channel[ tag ]  ] = channels;
  else hDecoder -> internal_channel[ channels ] = channels;
 }  /* end else */
 hDecoder -> fr_channels += hDecoder -> element_output_channels[ hDecoder -> fr_ch_ele ];
 hDecoder -> fr_ch_ele++;
}  /* end decode_sce_lfe */

static void raw_data_block (
             _NeAACDecHandle hDecoder, _NeAACDecFrameInfo* hInfo,
             SMS_BitContext* ld, program_config* pce, drc_info* drc
            ) {

 uint8_t id_syn_ele;
 uint8_t ele_this_frame = 0;

 hDecoder -> fr_channels   =  0;
 hDecoder -> fr_ch_ele     =  0;
 hDecoder -> first_syn_ele = 25;
 hDecoder -> has_lfe       =  0;

 while (   (  id_syn_ele = ( uint8_t )SMS_GetBits ( ld, LEN_SE_ID )  ) != ID_END && ld -> m_Idx < ld -> m_szInBits   ) {
  switch ( id_syn_ele ) {
   case ID_SCE:
    ++ele_this_frame;
    if ( hDecoder -> first_syn_ele == 25 ) hDecoder -> first_syn_ele = id_syn_ele;
    decode_sce_lfe ( hDecoder, hInfo, ld, id_syn_ele );
    if ( hInfo -> error > 0 ) return;
   break;
   case ID_CPE:
    ++ele_this_frame;
    if ( hDecoder -> first_syn_ele == 25 ) hDecoder -> first_syn_ele = id_syn_ele;
    decode_cpe ( hDecoder, hInfo, ld, id_syn_ele );
    if ( hInfo -> error > 0 ) return;
   break;
   case ID_LFE:
    ++ele_this_frame;
    ++hDecoder -> has_lfe;
    decode_sce_lfe ( hDecoder, hInfo, ld, id_syn_ele );
    if ( hInfo -> error > 0 ) return;
   break;
   case ID_CCE:
    ++ele_this_frame;
    hInfo -> error = 6;
    if ( hInfo -> error > 0 ) return;
   break;
   case ID_DSE:
    ++ele_this_frame;
    data_stream_element ( hDecoder, ld );
   break;
   case ID_PCE:
    if ( ele_this_frame != 0 ) {
     hInfo -> error = 31;
     return;
    }  /* end if */
    ++ele_this_frame;
    program_config_element ( pce, ld );
   break;
   case ID_FIL:
    ++ele_this_frame;
    if (   (  hInfo -> error = fill_element ( hDecoder, ld, drc, INVALID_SBR_ELEMENT )  ) > 0   ) return;
   break;
  }  /* end switch */
 }  /* end while */

 SMS_AlignBits ( ld );

}  /* end raw_data_block */

#define CONV( a, b ) (  ( a << 1 ) | ( b & 0x1 )  )

static void output_to_PCM ( _NeAACDecHandle hDecoder, float** input, short* sample_buffer, uint8_t channels, int buffer_size ) {

 switch (  CONV( channels, hDecoder -> downMatrix )  ) {
  float* lppChan[ 5 ];
  case CONV( 1, 0 ):
   lppChan[ 0 ] = input[ hDecoder -> internal_channel[ 0 ]  ];
   pcm_syn1 ( sample_buffer, lppChan, buffer_size );
  break;
  case CONV( 2, 0 ):
   if ( hDecoder -> upMatrix ) {
    lppChan[ 0 ] = input[ hDecoder -> internal_channel[ 0 ]  ];
    lppChan[ 1 ] = input[ hDecoder -> internal_channel[ 0 ]  ];
    pcm_syn2 ( sample_buffer, lppChan, buffer_size );
   } else {
    lppChan[ 0 ] = input[  hDecoder -> internal_channel[ 0 ]  ];
    lppChan[ 1 ] = input[  hDecoder -> internal_channel[ 1 ]  ];
    pcm_syn2 ( sample_buffer, lppChan, buffer_size );
   }  /* end else */
  break;
  default:
   lppChan[ 0 ] = input[  hDecoder -> internal_channel[ 0 ]  ];
   lppChan[ 1 ] = input[  hDecoder -> internal_channel[ 1 ]  ];
   lppChan[ 2 ] = input[  hDecoder -> internal_channel[ 2 ]  ];
   lppChan[ 3 ] = input[  hDecoder -> internal_channel[ 3 ]  ];
   lppChan[ 4 ] = input[  hDecoder -> internal_channel[ 4 ]  ];
   pcm_synN ( sample_buffer, lppChan, buffer_size );
  break;
 }  /* end switch */

}  /* end output_to_PCM */

static void create_channel_config ( _NeAACDecHandle hDecoder, _NeAACDecFrameInfo* hInfo ) {

 hInfo -> num_front_channels = 0;
 hInfo -> num_side_channels  = 0;
 hInfo -> num_back_channels  = 0;
 hInfo -> num_lfe_channels   = 0;

 memset (  hInfo -> channel_position, 0, MAX_CHANNELS * sizeof ( uint8_t )  );

 if ( hDecoder -> downMatrix ) {
  hInfo -> num_front_channels    = 2;
  hInfo -> channel_position[ 0 ] = FRONT_CHANNEL_LEFT;
  hInfo -> channel_position[ 1 ] = FRONT_CHANNEL_RIGHT;
  return;
 }  /* end if */

 if ( hDecoder -> pce_set ) {

  uint8_t i, chpos = 0;
  uint8_t chdir, back_center = 0;

  hInfo -> num_front_channels = hDecoder -> pce.num_front_channels;
  hInfo -> num_side_channels  = hDecoder -> pce.num_side_channels;
  hInfo -> num_back_channels  = hDecoder -> pce.num_back_channels;
  hInfo -> num_lfe_channels   = hDecoder -> pce.num_lfe_channels;

  chdir = hInfo -> num_front_channels;

  if ( chdir & 1 ) {
   hInfo -> channel_position[ chpos++ ] = FRONT_CHANNEL_CENTER;
   --chdir;
  }  /* end if */

  for ( i = 0; i < chdir; i += 2 ) {
   hInfo -> channel_position[ chpos++ ] = FRONT_CHANNEL_LEFT;
   hInfo -> channel_position[ chpos++ ] = FRONT_CHANNEL_RIGHT;
  }  /* end for */

  for ( i = 0; i < hInfo -> num_side_channels; i += 2 ) {
   hInfo -> channel_position[ chpos++ ] = SIDE_CHANNEL_LEFT;
   hInfo -> channel_position[ chpos++ ] = SIDE_CHANNEL_RIGHT;
  }  /* end for */

  chdir = hInfo -> num_back_channels;

  if ( chdir & 1 ) {
   back_center = 1;
   chdir--;
  }  /* end if */

  for ( i = 0; i < chdir; i += 2 ) {
   hInfo -> channel_position[ chpos++ ] = BACK_CHANNEL_LEFT;
   hInfo -> channel_position[ chpos++ ] = BACK_CHANNEL_RIGHT;
  }  /* end for */

  if ( back_center ) hInfo -> channel_position[ chpos++ ] = BACK_CHANNEL_CENTER;

  for ( i = 0; i < hInfo -> num_lfe_channels; ++i ) hInfo -> channel_position[ chpos++ ] = LFE_CHANNEL;

 } else switch ( hDecoder -> channelConfiguration ) {

  case 1:
   hInfo -> num_front_channels    = 1;
   hInfo -> channel_position[ 0 ] = FRONT_CHANNEL_CENTER;
  break;

  case 2:
   hInfo -> num_front_channels    = 2;
   hInfo -> channel_position[ 0 ] = FRONT_CHANNEL_LEFT;
   hInfo -> channel_position[ 1 ] = FRONT_CHANNEL_RIGHT;
  break;

  case 3:
   hInfo -> num_front_channels    = 3;
   hInfo -> channel_position[ 0 ] = FRONT_CHANNEL_CENTER;
   hInfo -> channel_position[ 1 ] = FRONT_CHANNEL_LEFT;
   hInfo -> channel_position[ 2 ] = FRONT_CHANNEL_RIGHT;
  break;

  case 4:
   hInfo -> num_front_channels    = 3;
   hInfo -> num_back_channels     = 1;
   hInfo -> channel_position[ 0 ] = FRONT_CHANNEL_CENTER;
   hInfo -> channel_position[ 1 ] = FRONT_CHANNEL_LEFT;
   hInfo -> channel_position[ 2 ] = FRONT_CHANNEL_RIGHT;
   hInfo -> channel_position[ 3 ] = BACK_CHANNEL_CENTER;
  break;

  case 5:
   hInfo->num_front_channels = 3;
   hInfo->num_back_channels = 2;
   hInfo->channel_position[0] = FRONT_CHANNEL_CENTER;
   hInfo->channel_position[1] = FRONT_CHANNEL_LEFT;
   hInfo->channel_position[2] = FRONT_CHANNEL_RIGHT;
   hInfo->channel_position[3] = BACK_CHANNEL_LEFT;
   hInfo->channel_position[4] = BACK_CHANNEL_RIGHT;
  break;

  case 6:
   hInfo -> num_front_channels    = 3;
   hInfo -> num_back_channels     = 2;
   hInfo -> num_lfe_channels      = 1;
   hInfo -> channel_position[ 0 ] = FRONT_CHANNEL_CENTER;
   hInfo -> channel_position[ 1 ] = FRONT_CHANNEL_LEFT;
   hInfo -> channel_position[ 2 ] = FRONT_CHANNEL_RIGHT;
   hInfo -> channel_position[ 3 ] = BACK_CHANNEL_LEFT;
   hInfo -> channel_position[ 4 ] = BACK_CHANNEL_RIGHT;
   hInfo -> channel_position[ 5 ] = LFE_CHANNEL;
  break;

  case 7:
   hInfo -> num_front_channels    = 3;
   hInfo -> num_side_channels     = 2;
   hInfo -> num_back_channels     = 2;
   hInfo -> num_lfe_channels      = 1;
   hInfo -> channel_position[ 0 ] = FRONT_CHANNEL_CENTER;
   hInfo -> channel_position[ 1 ] = FRONT_CHANNEL_LEFT;
   hInfo -> channel_position[ 2 ] = FRONT_CHANNEL_RIGHT;
   hInfo -> channel_position[ 3 ] = SIDE_CHANNEL_LEFT;
   hInfo -> channel_position[ 4 ] = SIDE_CHANNEL_RIGHT;
   hInfo -> channel_position[ 5 ] = BACK_CHANNEL_LEFT;
   hInfo -> channel_position[ 6 ] = BACK_CHANNEL_RIGHT;
   hInfo -> channel_position[ 7 ] = LFE_CHANNEL;
  break;

  default: {
   uint8_t i;
   uint8_t ch = hDecoder -> fr_channels - hDecoder -> has_lfe;
   if ( ch & 1 ) {
    uint8_t ch1 = ( ch - 1 ) / 2;
    if ( hDecoder -> first_syn_ele == ID_SCE ) {
     hInfo -> num_front_channels    = ch1 + 1;
     hInfo -> num_back_channels     = ch1;
     hInfo -> channel_position[ 0 ] = FRONT_CHANNEL_CENTER;
     for ( i = 1; i <= ch1; i += 2 ) {
      hInfo -> channel_position[ i     ] = FRONT_CHANNEL_LEFT;
      hInfo -> channel_position[ i + 1 ] = FRONT_CHANNEL_RIGHT;
     }  /* end for */
     for ( i = ch1 + 1; i < ch; i += 2 ) {
      hInfo -> channel_position[ i     ] = BACK_CHANNEL_LEFT;
      hInfo -> channel_position[ i + 1 ] = BACK_CHANNEL_RIGHT;
     }  /* end for */
    } else {
     hInfo -> num_front_channels = ch1;
     hInfo -> num_back_channels  = ch1 + 1;
     for (i = 0; i < ch1; i += 2 ) {
      hInfo -> channel_position[ i     ] = FRONT_CHANNEL_LEFT;
      hInfo -> channel_position[ i + 1 ] = FRONT_CHANNEL_RIGHT;
     }  /* end for */
     for ( i = ch1; i < ch - 1; i += 2 ) {
      hInfo -> channel_position[ i     ] = BACK_CHANNEL_LEFT;
      hInfo -> channel_position[ i + 1 ] = BACK_CHANNEL_RIGHT;
     }  /* end for */
     hInfo -> channel_position[ ch - 1 ] = BACK_CHANNEL_CENTER;
    }  /* end else */
   } else {
    uint8_t ch1 = ch / 2;
    hInfo -> num_front_channels = ch1;
    hInfo -> num_back_channels  = ch1;
    if ( ch1 & 1 ) {
     hInfo -> channel_position[ 0 ] = FRONT_CHANNEL_CENTER;
     for ( i = 1; i <= ch1; i += 2 ) {
      hInfo -> channel_position[ i     ] = FRONT_CHANNEL_LEFT;
      hInfo -> channel_position[ i + 1 ] = FRONT_CHANNEL_RIGHT;
     }  /* end for */
     for ( i = ch1 + 1; i < ch - 1; i += 2 ) {
      hInfo -> channel_position[ i     ] = BACK_CHANNEL_LEFT;
      hInfo -> channel_position[ i + 1 ] = BACK_CHANNEL_RIGHT;
     }  /* end for */
     hInfo -> channel_position[ ch - 1 ] = BACK_CHANNEL_CENTER;
    } else {
     for ( i = 0; i < ch1; i += 2 ) {
      hInfo -> channel_position[ i     ] = FRONT_CHANNEL_LEFT;
      hInfo -> channel_position[ i + 1 ] = FRONT_CHANNEL_RIGHT;
     } /* end for */
     for ( i = ch1; i < ch; i += 2 ) {
      hInfo -> channel_position[ i     ] = BACK_CHANNEL_LEFT;
      hInfo -> channel_position[ i + 1 ] = BACK_CHANNEL_RIGHT;
     }  /* end for */
    }  /* end else */
   }  /* end else */
   hInfo -> num_lfe_channels = hDecoder -> has_lfe;
   for ( i = ch; i < hDecoder -> fr_channels; ++i ) hInfo -> channel_position[ i ] = LFE_CHANNEL;
  } break;

 }  /* end switch */

}  /* end create_channel_config */

static int _aac_decode ( SMS_CodecContext* apCtx, SMS_RingBuffer* apOutput, SMS_RingBuffer* apInput ) {

 uint16_t           i;
 _NeAACDecHandle    hDecoder        = ( _NeAACDecHandle )apCtx -> m_pCodec -> m_pCtx;
 uint8_t            channels        = 0;
 uint8_t            output_channels = 0;
 SMS_AVPacket*      lpPkt           = ( SMS_AVPacket* )apInput -> m_pOut;
 uint8_t*           buffer          = lpPkt -> m_pData;
 int32_t            buffer_size     = lpPkt -> m_Size;
 _NeAACDecFrameInfo lInfo;
 SMS_BitContext     ld;
 uint32_t           bitsconsumed;
 short*             lpSamples;

 memset (  &lInfo, 0, sizeof ( lInfo )  );
 memset (  hDecoder -> internal_channel, 0, MAX_CHANNELS * sizeof ( hDecoder -> internal_channel[ 0 ] )  );

 if ( buffer_size >= 128 && buffer[ 0 ] == 'T' && buffer[ 1 ] == 'A' && buffer[ 2 ] == 'G' ) return 0;

 SMS_InitGetBits ( &ld, buffer, buffer_size << 3 );

 if ( hDecoder -> adts_header_present ) {
  adts_header adts;
  adts.old_format = hDecoder -> config.useOldADTSFormat;
  if (  adts_frame ( &adts, &ld ) > 0   ) goto error;
 }  /* end if */

 raw_data_block ( hDecoder, &lInfo, &ld, &hDecoder -> pce, &hDecoder -> drc );

 channels = hDecoder -> fr_channels;

 if ( lInfo.error > 0 ) goto error;

 bitsconsumed = ld.m_Idx;
 lInfo.bytesconsumed = bit2byte ( bitsconsumed );

 if ( !hDecoder -> adts_header_present && !hDecoder -> adif_header_present ) {
  if ( hDecoder -> channelConfiguration == 0 ) hDecoder -> channelConfiguration = channels;
  if ( channels == 8 ) hDecoder -> channelConfiguration = 7;
  if ( channels == 7 ) hDecoder -> channelConfiguration = 0;
 }  /* end if */

 hDecoder -> downMatrix = 0;
 hDecoder -> upMatrix   = 0;

 output_channels = 2;

 if ( channels > 2 )
  hDecoder -> downMatrix = 1;
 else if ( channels == 1 ) {
  if ( apCtx -> m_Channels == 2 )
   hDecoder -> upMatrix = 1;
  else output_channels = 1;
 }  /* end if */

 create_channel_config ( hDecoder, &lInfo );

 lInfo.samples     = 1024 * output_channels;
 lInfo.channels    = output_channels;
 lInfo.samplerate  = get_sample_rate ( hDecoder -> sf_index );
 lInfo.object_type = hDecoder -> object_type;
 lInfo.sbr         = FAAD_NO_SBR;
 lInfo.header_type = FAAD_RAW;

 buffer_size = 1024;

 if ( hDecoder -> sbr_present_flag || hDecoder -> forceUpSampling ) {
  if ( !hDecoder -> downSampledSBR ) {
   buffer_size      <<= 1;
   lInfo.samples    <<= 1;
   lInfo.samplerate <<= 1;
  }  /* end if */
  for ( i = 0; i < hDecoder -> fr_ch_ele; ++ i ) if ( !hDecoder -> sbr[ i ] ) {
   lInfo.error = 25;
   goto error;
  }  /* end if */
 }  /* end if */

 if ( hDecoder -> adif_header_present ) lInfo.header_type = FAAD_ADIF;
 if ( hDecoder -> adts_header_present ) lInfo.header_type = FAAD_ADTS;

 ++hDecoder -> frame;

 if ( channels == 0 ) goto end;

 i                  = lInfo.samples << 1;
 lpSamples          = ( short* )SMS_RingBufferAlloc ( apOutput, i + 80 );
 lpSamples         += 32;
 *( int* )lpSamples = i;
 lpSamples         += 8;

 output_to_PCM ( hDecoder, hDecoder -> time_out, lpSamples, output_channels, buffer_size );
 apOutput -> UserCB ( apOutput );

 goto end;
error:
 for ( i = 0; i < MAX_CHANNELS;        ++i ) if ( hDecoder -> fb_intermed[ i ] != NULL ) memset   (  hDecoder -> fb_intermed[ i ], 0, 1024 * sizeof ( float )  );
 for ( i = 0; i < MAX_SYNTAX_ELEMENTS; ++i ) if ( hDecoder -> sbr        [ i ] != NULL ) sbrReset (  hDecoder -> sbr[ i ]                                      );
end:
 return 0;

}  /* end _aac_decode */

static int _aac_init ( SMS_CodecContext* apCtx ) {

 _NeAACDecHandle lpAAC = ( _NeAACDecHandle )apCtx -> m_pCodec -> m_pCtx;
 uint32_t        lSR;
 uint8_t         lnChan;
 int             retVal;

 if ( apCtx -> m_pUserData && apCtx -> m_UserDataLen > 0 )
  retVal = _NeAACDecInit2 ( lpAAC, apCtx -> m_pUserData, apCtx -> m_UserDataLen, &lSR, &lnChan );
 else retVal = _NeAACDecInit  (  lpAAC, apCtx -> m_pUserData, abs ( apCtx -> m_UserDataLen ), &lSR, &lnChan  );

 return retVal;

}  /* end _aac_init */

static void _aac_destroy ( SMS_CodecContext* apCtx ) {

 _NeAACDecHandle lpAAC = ( _NeAACDecHandle )apCtx -> m_pCodec -> m_pCtx;
 _NeAACDecClose ( lpAAC );

}  /* end _aac_destroy */

void SMS_Codec_AAC_Open ( SMS_CodecContext* apCtx ) {

 _NeAACDecHandle lpAAC;

 apCtx -> m_pCodec = calloc (  1, sizeof ( SMS_Codec )  );

 apCtx -> m_pCodec -> m_pName = g_pAAC;
 apCtx -> m_pCodec -> m_pCtx  = _NeAACDecOpen ();
 apCtx -> m_pCodec -> Init    = _aac_init;
 apCtx -> m_pCodec -> Decode  = _aac_decode;
 apCtx -> m_pCodec -> Destroy = _aac_destroy;

 lpAAC = ( _NeAACDecHandle )apCtx -> m_pCodec -> m_pCtx;

 if ( apCtx -> m_SampleRate ) lpAAC -> config.defSampleRate = apCtx -> m_SampleRate;

}  /* end SMS_Codec_WMA_Open */
