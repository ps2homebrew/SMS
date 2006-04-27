/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
#
# Based on C source code of the VideoLAN project.
#
#  Authors of that code are:
#   Richard Boulton <richard@tartarus.org>
#   Ralph Loader    <suckfish@ihug.co.nz>
#
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
.set noat
.set volatile
.set noreorder
.set nomacro

.globl DSP_FFTInit
.globl DSP_FFTRun
.globl DSP_FFTGet

.data

s_SinCos:   .word 0x00000000, 0x3C490E90, 0x3CC90AB0, 0x3D16C32C
            .word 0x3D48FB30, 0x3D7B2B74, 0x3D96A905, 0x3DAFB680
            .word 0x3DC8BD36, 0x3DE1BC2E, 0x3DFAB272, 0x3E09CF86
            .word 0x3E164083, 0x3E22ABB6, 0x3E2F10A2, 0x3E3B6ECF
            .word 0x3E47C5C2, 0x3E541501, 0x3E605C13, 0x3E6C9A80
            .word 0x3E78CFCC, 0x3E827DC0, 0x3E888E93, 0x3E8E9A22
            .word 0x3E94A031, 0x3E9AA086, 0x3EA09AE5, 0x3EA68F12
            .word 0x3EAC7CD3, 0x3EB263EF, 0x3EB8442A, 0x3EBE1D4A
            .word 0x3EC3EF16, 0x3EC9B953, 0x3ECF7BCA, 0x3ED53641
            .word 0x3EDAE880, 0x3EE0924F, 0x3EE63375, 0x3EEBCBBA
            .word 0x3EF15AE9, 0x3EF6E0CA, 0x3EFC5D26, 0x3F00E7E4
            .word 0x3F039C3D, 0x3F064B83, 0x3F08F59B, 0x3F0B9A6B
            .word 0x3F0E39DA, 0x3F10D3CC, 0x3F13682B, 0x3F15F6D9
            .word 0x3F187FC0, 0x3F1B02C5, 0x3F1D7FD1, 0x3F1FF6CB
            .word 0x3F226799, 0x3F24D225, 0x3F273656, 0x3F299415
            .word 0x3F2BEB4A, 0x3F2E3BDE, 0x3F3085BB, 0x3F32C8C9
            .word 0x3F3504F3, 0x3F373A23, 0x3F396841, 0x3F3B8F3B
            .word 0x3F3DAEF9, 0x3F3FC767, 0x3F41D870, 0x3F43E201
            .word 0x3F45E403, 0x3F47DE65, 0x3F49D112, 0x3F4BBBF8
            .word 0x3F4D9F02, 0x3F4F7A1F, 0x3F514D3D, 0x3F531849
            .word 0x3F54DB31, 0x3F5695E5, 0x3F584853, 0x3F59F26A
            .word 0x3F5B941A, 0x3F5D2D53, 0x3F5EBE05, 0x3F604622
            .word 0x3F61C597, 0x3F633C5A, 0x3F64AA59, 0x3F660F87
            .word 0x3F676BD8, 0x3F68BF3C, 0x3F6A09A7, 0x3F6B4B0B
            .word 0x3F6C835E, 0x3F6DB293, 0x3F6ED89D, 0x3F6FF573
            .word 0x3F710908, 0x3F721353, 0x3F731447, 0x3F740BDD
            .word 0x3F74FA0B, 0x3F75DEC6, 0x3F76BA07, 0x3F778BC5
            .word 0x3F7853F8, 0x3F791298, 0x3F79C79D, 0x3F7A7302
            .word 0x3F7B14BE, 0x3F7BACCD, 0x3F7C3B28, 0x3F7CBFC9
            .word 0x3F7D3AAC, 0x3F7DABCC, 0x3F7E1324, 0x3F7E70B0
            .word 0x3F7EC46D, 0x3F7F0E58, 0x3F7F4E6D, 0x3F7F84AB
            .word 0x3F7FB10F, 0x3F7FD398, 0x3F7FEC43, 0x3F7FFB11
            .word 0x3F800000, 0x3F7FFB11, 0x3F7FEC43, 0x3F7FD397
            .word 0x3F7FB10F, 0x3F7F84AB, 0x3F7F4E6D, 0x3F7F0E58
            .word 0x3F7EC46D, 0x3F7E70B0, 0x3F7E1323, 0x3F7DABCC
            .word 0x3F7D3AAC, 0x3F7CBFC9, 0x3F7C3B28, 0x3F7BACCD
            .word 0x3F7B14BE, 0x3F7A7302, 0x3F79C79E, 0x3F791298
            .word 0x3F7853F8, 0x3F778BC5, 0x3F76BA07, 0x3F75DEC6
            .word 0x3F74FA0A, 0x3F740BDE, 0x3F731447, 0x3F721352
            .word 0x3F710908, 0x3F6FF573, 0x3F6ED89E, 0x3F6DB293
            .word 0x3F6C835F, 0x3F6B4B0C, 0x3F6A09A6, 0x3F68BF3C
            .word 0x3F676BD8, 0x3F660F87, 0x3F64AA59, 0x3F633C59
            .word 0x3F61C598, 0x3F604622, 0x3F5EBE06, 0x3F5D2D53
            .word 0x3F5B941A, 0x3F59F269, 0x3F584852, 0x3F5695E6
            .word 0x3F54DB32, 0x3F531849, 0x3F514D3D, 0x3F4F7A1F
            .word 0x3F4D9F02, 0x3F4BBBF7, 0x3F49D113, 0x3F47DE66
            .word 0x3F45E404, 0x3F43E200, 0x3F41D870, 0x3F3FC766
            .word 0x3F3DAEF8, 0x3F3B8F3C, 0x3F396842, 0x3F373A23
            .word 0x3F3504F3, 0x3F32C8C9, 0x3F3085BA, 0x3F2E3BDD
            .word 0x3F2BEB4B, 0x3F299415, 0x3F273656, 0x3F24D225
            .word 0x3F226799, 0x3F1FF6CA, 0x3F1D7FD0, 0x3F1B02C7
            .word 0x3F187FC1, 0x3F15F6D9, 0x3F13682A, 0x3F10D3CC
            .word 0x3F0E39D9, 0x3F0B9A6D, 0x3F08F59C, 0x3F064B83
            .word 0x3F039C3D, 0x3F00E7E4, 0x3EFC5D25, 0x3EF6E0C8
            .word 0x3EF15AED, 0x3EEBCBBD, 0x3EE63376, 0x3EE0924F
            .word 0x3EDAE87F, 0x3ED5363F, 0x3ECF7BC7, 0x3EC9B956
            .word 0x3EC3EF17, 0x3EBE1D4B, 0x3EB8442A, 0x3EB263EE
            .word 0x3EAC7CD2, 0x3EA68F0F, 0x3EA09AE8, 0x3E9AA088
            .word 0x3E94A033, 0x3E8E9A22, 0x3E888E92, 0x3E827DBE
            .word 0x3E78CFC5, 0x3E6C9A85, 0x3E605C17, 0x3E541503
            .word 0x3E47C5C1, 0x3E3B6ECC, 0x3E2F109D, 0x3E22ABAE
            .word 0x3E16408A, 0x3E09CF8A, 0x3DFAB276, 0x3DE1BC2D
            .word 0x3DC8BD30, 0x3DAFB675, 0x3D96A8F5, 0x3D7B2B8B
            .word 0x3D48FB3D, 0x3D16C330, 0x3CC90AA6, 0x3C490E57
            .word 0x3F800000, 0x3F7FFB11, 0x3F7FEC43, 0x3F7FD397
            .word 0x3F7FB10F, 0x3F7F84AB, 0x3F7F4E6D, 0x3F7F0E58
            .word 0x3F7EC46D, 0x3F7E70B0, 0x3F7E1324, 0x3F7DABCC
            .word 0x3F7D3AAC, 0x3F7CBFC9, 0x3F7C3B28, 0x3F7BACCD
            .word 0x3F7B14BE, 0x3F7A7302, 0x3F79C79D, 0x3F791298
            .word 0x3F7853F8, 0x3F778BC5, 0x3F76BA07, 0x3F75DEC6
            .word 0x3F74FA0B, 0x3F740BDD, 0x3F731447, 0x3F721352
            .word 0x3F710908, 0x3F6FF573, 0x3F6ED89E, 0x3F6DB293
            .word 0x3F6C835E, 0x3F6B4B0C, 0x3F6A09A7, 0x3F68BF3C
            .word 0x3F676BD8, 0x3F660F88, 0x3F64AA59, 0x3F633C5A
            .word 0x3F61C598, 0x3F604621, 0x3F5EBE06, 0x3F5D2D53
            .word 0x3F5B941A, 0x3F59F26A, 0x3F584853, 0x3F5695E5
            .word 0x3F54DB31, 0x3F531849, 0x3F514D3D, 0x3F4F7A20
            .word 0x3F4D9F02, 0x3F4BBBF8, 0x3F49D113, 0x3F47DE65
            .word 0x3F45E404, 0x3F43E200, 0x3F41D870, 0x3F3FC767
            .word 0x3F3DAEF9, 0x3F3B8F3B, 0x3F396842, 0x3F373A23
            .word 0x3F3504F3, 0x3F32C8C9, 0x3F3085BB, 0x3F2E3BDE
            .word 0x3F2BEB4A, 0x3F299414, 0x3F273656, 0x3F24D225
            .word 0x3F226799, 0x3F1FF6CB, 0x3F1D7FD1, 0x3F1B02C6
            .word 0x3F187FC0, 0x3F15F6D9, 0x3F13682B, 0x3F10D3CD
            .word 0x3F0E39DA, 0x3F0B9A6B, 0x3F08F59B, 0x3F064B82
            .word 0x3F039C3E, 0x3F00E7E5, 0x3EFC5D27, 0x3EF6E0C9
            .word 0x3EF15AEB, 0x3EEBCBBB, 0x3EE63374, 0x3EE09250
            .word 0x3EDAE881, 0x3ED53641, 0x3ECF7BC9, 0x3EC9B954
            .word 0x3EC3EF15, 0x3EBE1D48, 0x3EB8442B, 0x3EB263EF
            .word 0x3EAC7CD3, 0x3EA68F10, 0x3EA09AE5, 0x3E9AA086
            .word 0x3E94A030, 0x3E8E9A23, 0x3E888E93, 0x3E827DC0
            .word 0x3E78CFD0, 0x3E6C9A81, 0x3E605C12, 0x3E5414FE
            .word 0x3E47C5C4, 0x3E3B6ECF, 0x3E2F10A0, 0x3E22ABB9
            .word 0x3E164085, 0x3E09CF85, 0x3DFAB26C, 0x3DE1BC32
            .word 0x3DC8BD35, 0x3DAFB67B, 0x3D96A90B, 0x3D7B2B77
            .word 0x3D48FB29, 0x3D16C31C, 0x3CC90ABE, 0x3C490E86
            .word 0xB33BBD2E, 0xBC490E64, 0xBCC90AAD, 0xBD16C333
            .word 0xBD48FB21, 0xBD7B2B6E, 0xBD96A906, 0xBDAFB687
            .word 0xBDC8BD31, 0xBDE1BC2E, 0xBDFAB277, 0xBE09CF83
            .word 0xBE164082, 0xBE22ABB7, 0xBE2F10A6, 0xBE3B6ECD
            .word 0xBE47C5C2, 0xBE541504, 0xBE605C10, 0xBE6C9A7E
            .word 0xBE78CFCD, 0xBE827DC2, 0xBE888E92, 0xBE8E9A22
            .word 0xBE94A033, 0xBE9AA085, 0xBEA09AE4, 0xBEA68F13
            .word 0xBEAC7CD2, 0xBEB263EE, 0xBEB8442A, 0xBEBE1D4B
            .word 0xBEC3EF14, 0xBEC9B953, 0xBECF7BCB, 0xBED53640
            .word 0xBEDAE880, 0xBEE0924F, 0xBEE63376, 0xBEEBCBBE
            .word 0xBEF15AE6, 0xBEF6E0C8, 0xBEFC5D26, 0xBF00E7E4
            .word 0xBF039C3D, 0xBF064B83, 0xBF08F59C, 0xBF0B9A6A
            .word 0xBF0E39D9, 0xBF10D3CC, 0xBF13682A, 0xBF15F6DA
            .word 0xBF187FC1, 0xBF1B02C7, 0xBF1D7FD0, 0xBF1FF6CA
            .word 0xBF226799, 0xBF24D225, 0xBF273656, 0xBF299415
            .word 0xBF2BEB4B, 0xBF2E3BDD, 0xBF3085BA, 0xBF32C8C9
            .word 0xBF3504F3, 0xBF373A23, 0xBF396843, 0xBF3B8F3C
            .word 0xBF3DAEF8, 0xBF3FC766, 0xBF41D870, 0xBF43E201
            .word 0xBF45E404, 0xBF47DE66, 0xBF49D113, 0xBF4BBBF7
            .word 0xBF4D9F02, 0xBF4F7A1F, 0xBF514D3D, 0xBF531849
            .word 0xBF54DB32, 0xBF5695E4, 0xBF584852, 0xBF59F26A
            .word 0xBF5B941A, 0xBF5D2D53, 0xBF5EBE06, 0xBF604622
            .word 0xBF61C597, 0xBF633C59, 0xBF64AA59, 0xBF660F88
            .word 0xBF676BD8, 0xBF68BF3C, 0xBF6A09A7, 0xBF6B4B0B
            .word 0xBF6C835E, 0xBF6DB293, 0xBF6ED89E, 0xBF6FF573
            .word 0xBF710909, 0xBF721353, 0xBF731447, 0xBF740BDD
            .word 0xBF74FA0B, 0xBF75DEC6, 0xBF76BA07, 0xBF778BC5
            .word 0xBF7853F8, 0xBF791297, 0xBF79C79D, 0xBF7A7302
            .word 0xBF7B14BF, 0xBF7BACCD, 0xBF7C3B28, 0xBF7CBFC9
            .word 0xBF7D3AAC, 0xBF7DABCB, 0xBF7E1323, 0xBF7E70B0
            .word 0xBF7EC46D, 0xBF7F0E58, 0xBF7F4E6E, 0xBF7F84AB
            .word 0xBF7FB10F, 0xBF7FD397, 0xBF7FEC43, 0xBF7FFB11

s_BitRev:   .half 0x0000, 0x0100, 0x0080, 0x0180, 0x0040, 0x0140, 0x00C0, 0x01C0
            .half 0x0020, 0x0120, 0x00A0, 0x01A0, 0x0060, 0x0160, 0x00E0, 0x01E0
            .half 0x0010, 0x0110, 0x0090, 0x0190, 0x0050, 0x0150, 0x00D0, 0x01D0
            .half 0x0030, 0x0130, 0x00B0, 0x01B0, 0x0070, 0x0170, 0x00F0, 0x01F0
            .half 0x0008, 0x0108, 0x0088, 0x0188, 0x0048, 0x0148, 0x00C8, 0x01C8
            .half 0x0028, 0x0128, 0x00A8, 0x01A8, 0x0068, 0x0168, 0x00E8, 0x01E8
            .half 0x0018, 0x0118, 0x0098, 0x0198, 0x0058, 0x0158, 0x00D8, 0x01D8
            .half 0x0038, 0x0138, 0x00B8, 0x01B8, 0x0078, 0x0178, 0x00F8, 0x01F8
            .half 0x0004, 0x0104, 0x0084, 0x0184, 0x0044, 0x0144, 0x00C4, 0x01C4
            .half 0x0024, 0x0124, 0x00A4, 0x01A4, 0x0064, 0x0164, 0x00E4, 0x01E4
            .half 0x0014, 0x0114, 0x0094, 0x0194, 0x0054, 0x0154, 0x00D4, 0x01D4
            .half 0x0034, 0x0134, 0x00B4, 0x01B4, 0x0074, 0x0174, 0x00F4, 0x01F4
            .half 0x000C, 0x010C, 0x008C, 0x018C, 0x004C, 0x014C, 0x00CC, 0x01CC
            .half 0x002C, 0x012C, 0x00AC, 0x01AC, 0x006C, 0x016C, 0x00EC, 0x01EC
            .half 0x001C, 0x011C, 0x009C, 0x019C, 0x005C, 0x015C, 0x00DC, 0x01DC
            .half 0x003C, 0x013C, 0x00BC, 0x01BC, 0x007C, 0x017C, 0x00FC, 0x01FC
            .half 0x0002, 0x0102, 0x0082, 0x0182, 0x0042, 0x0142, 0x00C2, 0x01C2
            .half 0x0022, 0x0122, 0x00A2, 0x01A2, 0x0062, 0x0162, 0x00E2, 0x01E2
            .half 0x0012, 0x0112, 0x0092, 0x0192, 0x0052, 0x0152, 0x00D2, 0x01D2
            .half 0x0032, 0x0132, 0x00B2, 0x01B2, 0x0072, 0x0172, 0x00F2, 0x01F2
            .half 0x000A, 0x010A, 0x008A, 0x018A, 0x004A, 0x014A, 0x00CA, 0x01CA
            .half 0x002A, 0x012A, 0x00AA, 0x01AA, 0x006A, 0x016A, 0x00EA, 0x01EA
            .half 0x001A, 0x011A, 0x009A, 0x019A, 0x005A, 0x015A, 0x00DA, 0x01DA
            .half 0x003A, 0x013A, 0x00BA, 0x01BA, 0x007A, 0x017A, 0x00FA, 0x01FA
            .half 0x0006, 0x0106, 0x0086, 0x0186, 0x0046, 0x0146, 0x00C6, 0x01C6
            .half 0x0026, 0x0126, 0x00A6, 0x01A6, 0x0066, 0x0166, 0x00E6, 0x01E6
            .half 0x0016, 0x0116, 0x0096, 0x0196, 0x0056, 0x0156, 0x00D6, 0x01D6
            .half 0x0036, 0x0136, 0x00B6, 0x01B6, 0x0076, 0x0176, 0x00F6, 0x01F6
            .half 0x000E, 0x010E, 0x008E, 0x018E, 0x004E, 0x014E, 0x00CE, 0x01CE
            .half 0x002E, 0x012E, 0x00AE, 0x01AE, 0x006E, 0x016E, 0x00EE, 0x01EE
            .half 0x001E, 0x011E, 0x009E, 0x019E, 0x005E, 0x015E, 0x00DE, 0x01DE
            .half 0x003E, 0x013E, 0x00BE, 0x01BE, 0x007E, 0x017E, 0x00FE, 0x01FE
            .half 0x0001, 0x0101, 0x0081, 0x0181, 0x0041, 0x0141, 0x00C1, 0x01C1
            .half 0x0021, 0x0121, 0x00A1, 0x01A1, 0x0061, 0x0161, 0x00E1, 0x01E1
            .half 0x0011, 0x0111, 0x0091, 0x0191, 0x0051, 0x0151, 0x00D1, 0x01D1
            .half 0x0031, 0x0131, 0x00B1, 0x01B1, 0x0071, 0x0171, 0x00F1, 0x01F1
            .half 0x0009, 0x0109, 0x0089, 0x0189, 0x0049, 0x0149, 0x00C9, 0x01C9
            .half 0x0029, 0x0129, 0x00A9, 0x01A9, 0x0069, 0x0169, 0x00E9, 0x01E9
            .half 0x0019, 0x0119, 0x0099, 0x0199, 0x0059, 0x0159, 0x00D9, 0x01D9
            .half 0x0039, 0x0139, 0x00B9, 0x01B9, 0x0079, 0x0179, 0x00F9, 0x01F9
            .half 0x0005, 0x0105, 0x0085, 0x0185, 0x0045, 0x0145, 0x00C5, 0x01C5
            .half 0x0025, 0x0125, 0x00A5, 0x01A5, 0x0065, 0x0165, 0x00E5, 0x01E5
            .half 0x0015, 0x0115, 0x0095, 0x0195, 0x0055, 0x0155, 0x00D5, 0x01D5
            .half 0x0035, 0x0135, 0x00B5, 0x01B5, 0x0075, 0x0175, 0x00F5, 0x01F5
            .half 0x000D, 0x010D, 0x008D, 0x018D, 0x004D, 0x014D, 0x00CD, 0x01CD
            .half 0x002D, 0x012D, 0x00AD, 0x01AD, 0x006D, 0x016D, 0x00ED, 0x01ED
            .half 0x001D, 0x011D, 0x009D, 0x019D, 0x005D, 0x015D, 0x00DD, 0x01DD
            .half 0x003D, 0x013D, 0x00BD, 0x01BD, 0x007D, 0x017D, 0x00FD, 0x01FD
            .half 0x0003, 0x0103, 0x0083, 0x0183, 0x0043, 0x0143, 0x00C3, 0x01C3
            .half 0x0023, 0x0123, 0x00A3, 0x01A3, 0x0063, 0x0163, 0x00E3, 0x01E3
            .half 0x0013, 0x0113, 0x0093, 0x0193, 0x0053, 0x0153, 0x00D3, 0x01D3
            .half 0x0033, 0x0133, 0x00B3, 0x01B3, 0x0073, 0x0173, 0x00F3, 0x01F3
            .half 0x000B, 0x010B, 0x008B, 0x018B, 0x004B, 0x014B, 0x00CB, 0x01CB
            .half 0x002B, 0x012B, 0x00AB, 0x01AB, 0x006B, 0x016B, 0x00EB, 0x01EB
            .half 0x001B, 0x011B, 0x009B, 0x019B, 0x005B, 0x015B, 0x00DB, 0x01DB
            .half 0x003B, 0x013B, 0x00BB, 0x01BB, 0x007B, 0x017B, 0x00FB, 0x01FB
            .half 0x0007, 0x0107, 0x0087, 0x0187, 0x0047, 0x0147, 0x00C7, 0x01C7
            .half 0x0027, 0x0127, 0x00A7, 0x01A7, 0x0067, 0x0167, 0x00E7, 0x01E7
            .half 0x0017, 0x0117, 0x0097, 0x0197, 0x0057, 0x0157, 0x00D7, 0x01D7
            .half 0x0037, 0x0137, 0x00B7, 0x01B7, 0x0077, 0x0177, 0x00F7, 0x01F7
            .half 0x000F, 0x010F, 0x008F, 0x018F, 0x004F, 0x014F, 0x00CF, 0x01CF
            .half 0x002F, 0x012F, 0x00AF, 0x01AF, 0x006F, 0x016F, 0x00EF, 0x01EF
            .half 0x001F, 0x011F, 0x009F, 0x019F, 0x005F, 0x015F, 0x00DF, 0x01DF
            .half 0x003F, 0x013F, 0x00BF, 0x01BF, 0x007F, 0x017F, 0x00FF, 0x01FF
.align 4
s_Scale:    .float 0.000015, 0.000061, 0.000122, 0.000244
            .float 0.000488, 0.000488, 0.000977, 0.000977
            .float 0.000031, 0.000122, 0.000132, 0.000244
            .float 0.000488, 0.000488, 0.000977, 0.000977
            .float    128.0,    128.0,    128.0,    128.0
            .float      2.0,      2.0,      2.0,      2.0

.text

DSP_FFTInit:
    lui     $a2, %hi( s_BitRev )
    addiu   $a2, $a2, %lo( s_BitRev )
    addiu   $v1, $zero, 128
1:
    lh      $t0, 0($a2)
    lh      $t1, 2($a2)
    lh      $t2, 4($a2)
    lh      $t3, 6($a2)
    addiu   $a2, $a2, 8
    addiu   $v1, $v1, -1
    sll     $t0, $t0, 1
    sll     $t1, $t1, 1
    sll     $t2, $t2, 1
    sll     $t3, $t3, 1
    addu    $t0, $t0, $a0
    addu    $t1, $t1, $a0
    addu    $t2, $t2, $a0
    addu    $t3, $t3, $a0
    lh      $t0, 0($t0)
    lh      $t1, 0($t1)
    lh      $t2, 0($t2)
    lh      $t3, 0($t3)
    mtc1    $t0, $f00
    mtc1    $t1, $f01
    mtc1    $t2, $f02
    mtc1    $t3, $f03
    cvt.s.w $f00, $f00
    cvt.s.w $f01, $f01
    cvt.s.w $f02, $f02
    cvt.s.w $f03, $f03
    swc1    $f00,   0($a1)
    sw      $zero,  4($a1)
    swc1    $f01,   8($a1)
    sw      $zero, 12($a1)
    swc1    $f02,  16($a1)
    sw      $zero, 20($a1)
    swc1    $f03,  24($a1)
    sw      $zero, 28($a1)
    bgtz    $v1, 1b
    addiu   $a1, $a1, 32
    jr      $ra

DSP_FFTRun:
    lui     $t4, %hi( s_SinCos )
    addiu   $t4, $t4, %lo( s_SinCos )
    addiu   $a3, $zero,   1
    addiu   $t2, $zero, 256
    addu    $t5, $zero, $zero
    addiu   $t3, $t4, 1024
1:
    addu    $t1, $zero, $zero
    sll     $a2, $a3, 1
    mult    $v0, $t1, $t2
2:
    addu    $a1, $zero, $t1
    sll     $v0, $v0, 2
    addu    $v1, $v0, $t3
    addu    $v0, $v0, $t4
    lwc1    $f14, 0($v0)
    lwc1    $f12, 0($v1)
3:
    addu    $v0, $a1, $a3
    sll     $v1, $a1, 3
    sll     $v0, $v0, 3
    addu    $v1, $v1, $a0
    addu    $v0, $v0, $a0
    lwc1    $f00, 0($v0)
    lwc1    $f04, 4($v0)
    mul.s   $f10, $f14, $f00
    mul.s	$f02, $f12, $f04
    mul.s   $f00, $f12, $f00
    mul.s   $f04, $f14, $f04
    lwc1    $f08, 0($v1)
    add.s   $f02, $f02, $f10
    lwc1    $f06, 4($v1)
    sub.s	$f00, $f00, $f04
    add.s	$f10, $f06, $f02
    sub.s   $f06, $f06, $f02
    add.s   $f04, $f08, $f00
    addu    $a1, $a1, $a2
    sub.s   $f08, $f08, $f00
    sltiu   $t0, $a1, 512
    swc1    $f06, 4($v0)
    swc1    $f08, 0($v0)
    swc1    $f10, 4($v1)
    bne     $t0, $zero, 3b
    swc1    $f04, 0($v1)
    addiu   $t1, $t1, 1
    sltu    $v0, $t1, $a3
    bne     $v0, $zero, 2b
    mult    $v0, $t1, $t2
    addiu   $t5, $t5, 1
    addu    $a3, $zero, $a2
    sltiu   $v0, $t5, 9
    bne     $v0, $zero, 1b
    srl     $t2, $t2, 1
    jr      $ra
    nop

DSP_FFTGet:
    lui         $a1, 0x7000
    addiu       $v0, $v0, 32
1:
    lqc2        $vf01,   0($a0)
    lqc2        $vf02,  16($a0)
    lqc2        $vf03,  32($a0)
    lqc2        $vf04,  48($a0)
    lqc2        $vf05,  64($a0)
    lqc2        $vf06,  80($a0)
    lqc2        $vf07,  96($a0)
    lqc2        $vf08, 112($a0)
    vmul.xyzw   $vf01, $vf01, $vf01
    vmul.xyzw   $vf02, $vf02, $vf02
    vmul.xyzw   $vf03, $vf03, $vf03
    vmul.xyzw   $vf04, $vf04, $vf04
    vmr32.xz    $vf09, $vf01
    vmr32.xz    $vf10, $vf02
    vmr32.xz    $vf11, $vf03
    vmr32.xz    $vf12, $vf04
    vadd.xz     $vf01, $vf01, $vf09
    vadd.xz     $vf02, $vf02, $vf10
    vadd.xz     $vf03, $vf03, $vf11
    vadd.xz     $vf04, $vf04, $vf12
    vmul.xyzw   $vf05, $vf05, $vf05
    vmul.xyzw   $vf06, $vf06, $vf06
    vmul.xyzw   $vf07, $vf07, $vf07
    vmul.xyzw   $vf08, $vf08, $vf08
    vmr32.xz    $vf13, $vf05
    vmr32.xz    $vf14, $vf06
    vmr32.xz    $vf15, $vf07
    vmr32.xz    $vf16, $vf08
    vadd.xz     $vf05, $vf05, $vf13
    vadd.xz     $vf06, $vf06, $vf14
    vadd.xz     $vf07, $vf07, $vf15
    vadd.xz     $vf08, $vf08, $vf16
    vsqrt       Q, $vf01x
    vwaitq
    vaddq.x     $vf01, $vf00, Q
    vsqrt       Q, $vf01z
    vwaitq
    vaddq.z     $vf01, $vf00, Q
    vsqrt       Q, $vf02x
    vwaitq
    vaddq.x     $vf02, $vf00, Q
    vsqrt       Q, $vf02z
    vwaitq
    vaddq.z     $vf02, $vf00, Q
    vsqrt       Q, $vf03x
    vwaitq
    vaddq.x     $vf03, $vf00, Q
    vsqrt       Q, $vf03z
    vwaitq
    vaddq.z     $vf03, $vf00, Q
    vsqrt       Q, $vf04x
    vwaitq
    vaddq.x     $vf04, $vf00, Q
    vsqrt       Q, $vf04z
    vwaitq
    vaddq.z     $vf04, $vf00, Q
    vsqrt       Q, $vf05x
    vwaitq
    vaddq.x     $vf05, $vf00, Q
    vsqrt       Q, $vf05z
    vwaitq
    vaddq.z     $vf05, $vf00, Q
    vsqrt       Q, $vf06x
    vwaitq
    vaddq.x     $vf06, $vf00, Q
    vsqrt       Q, $vf06z
    vwaitq
    vaddq.z     $vf06, $vf00, Q
    vsqrt       Q, $vf07x
    vwaitq
    vaddq.x     $vf07, $vf00, Q
    vsqrt       Q, $vf07z
    vwaitq
    vaddq.z     $vf07, $vf00, Q
    vsqrt       Q, $vf08x
    vwaitq
    vaddq.x     $vf08, $vf00, Q
    vsqrt       Q, $vf08z
    vwaitq
    vaddq.z     $vf08, $vf00, Q
    vaddz.y     $vf01, $vf00, $vf01
    vaddx.z     $vf01, $vf00, $vf02
    vaddz.w     $vf01, $vf00, $vf02
    vaddz.y     $vf03, $vf00, $vf03
    vaddx.z     $vf03, $vf00, $vf04
    vaddz.w     $vf03, $vf00, $vf04
    vaddz.y     $vf05, $vf00, $vf05
    vaddx.z     $vf05, $vf00, $vf06
    vaddz.w     $vf05, $vf00, $vf06
    vaddz.y     $vf07, $vf00, $vf07
    vaddx.z     $vf07, $vf00, $vf08
    vaddz.w     $vf07, $vf00, $vf08
    sqc2        $vf01, 0x3800($a1)
    sqc2        $vf03, 0x3810($a1)
    sqc2        $vf05, 0x3820($a1)
    sqc2        $vf07, 0x3830($a1)
    addiu       $v0, $v0, -1
    addiu       $a0, $a0, 128
    bgtz        $v0, 1b
    addiu       $a1, $a1, 64
    lui         $a1, 0x7000
    lui         $v1, 0x3F80
    pextlw      $v1, $v1, $v1
    pcpyld      $v1, $v1, $v1
    qmtc2       $v1, $vf10
    lui         $a2, 0x7000
    addiu       $a3, $zero, 2
    lui         $t0, %hi( s_Scale )
    addiu       $t0, $t0, %lo( s_Scale )
3:
    addiu       $v0, $zero, 8
2:
    vadda.xyzw  ACC, $vf00, $vf00
    lqc2        $vf01, 0x3800($a1)
    lqc2        $vf02, 0x3810($a1)
    lqc2        $vf03, 0x3820($a1)
    lqc2        $vf04, 0x3830($a1)
    vmadda.xyzw ACC, $vf01, $vf10
    vmadda.xyzw ACC, $vf02, $vf10
    vmadda.xyzw ACC, $vf03, $vf10
    vmadda.xyzw ACC, $vf04, $vf10
    vmadd.xyzw  $vf01, $vf00, $vf00
    vaddy.x     $vf01, $vf01, $vf01
    vaddz.x     $vf01, $vf01, $vf01
    vaddw.x     $vf01, $vf01, $vf01
    addiu       $a1, $a1, 64
    addiu       $v0, $v0, -1
    qmfc2       $v1, $vf01
    addiu       $a2, $a2, 4
    bgtz        $v0, 2b
    sw          $v1, 0x37FC($a2)
    addiu       $a3, $a3, -1
    bgtzl       $a3, 3b
    addiu       $a1, $a1, 512
    lui         $a1, 0x7000
    lqc2        $vf01,  0($t0)
    lqc2        $vf02, 16($t0)
    lqc2        $vf03, 32($t0)
    lqc2        $vf04, 48($t0)
    lqc2        $vf05, 64($t0)
    lqc2        $vf06, 80($t0)
    lqc2        $vf07, 0x3800($a1)
    lqc2        $vf08, 0x3810($a1)
    lqc2        $vf09, 0x3820($a1)
    lqc2        $vf10, 0x3830($a1)
    vmul.xyzw   $vf07, $vf07, $vf01
    vmul.xyzw   $vf08, $vf08, $vf02
    vmul.xyzw   $vf09, $vf09, $vf03
    vmul.xyzw   $vf10, $vf10, $vf04
    vmini.xyzw  $vf07, $vf07, $vf05
    vmini.xyzw  $vf08, $vf08, $vf05
    vmini.xyzw  $vf09, $vf09, $vf05
    vmini.xyzw  $vf10, $vf10, $vf05
    vmax.xyzw   $vf07, $vf07, $vf06
    vmax.xyzw   $vf08, $vf08, $vf06
    vmax.xyzw   $vf09, $vf09, $vf06
    vmax.xyzw   $vf10, $vf10, $vf06
    vftoi0.xyzw $vf07, $vf07
    vftoi0.xyzw $vf08, $vf08
    vftoi0.xyzw $vf09, $vf09
    vftoi0.xyzw $vf10, $vf10
    sqc2        $vf07, 0x3800($a1)
    sqc2        $vf08, 0x3810($a1)
    sqc2        $vf09, 0x3820($a1)
    jr          $ra
    sqc2        $vf10, 0x3830($a1)
    

