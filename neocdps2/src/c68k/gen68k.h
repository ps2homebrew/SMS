/*********************************************************************************
 * GEN68K.H :
 *
 * C68K generator include file
 *
 ********************************************************************************/

#ifndef _GEN68K_H_
#define _GEN68K_H_

#ifdef __cplusplus
extern "C" {
#endif

// setting
///////////

// structure definition
////////////////////////

typedef struct {
	u32 name;
	u32 mask;
	u32 match;
} c68k_ea_info_struc;

typedef struct __c68k_op_info_struc {
    s8  op_name[8 + 1];
	u16 op_base;
	u16 op_mask;
    s8  size_type;
    s8  size_sft;
    s8  eam_sft;
    s8  reg_sft;
    s8  eam2_sft;
    s8  reg2_sft;
    s8  ea_supported[12 + 1];
    s8  ea2_supported[12 + 1];
    void (*genfunc)();
} c68k_op_info_struc;


#ifdef __cplusplus
}
#endif

#endif  // _GEN68K_H_

