/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
# Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
#
# The ifft algorithms in this file have been largely inspired by Dan
# Bernstein's work, djbfft, available at http://cr.yp.to/djbfft.html
#
# This file WAS part of a52dec, a free ATSC A-52 stream decoder.
# See http://liba52.sourceforge.net/ for updates.
#
# Adopted for SMS and optimized for R5900 by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_AC3.h"

static sample_t s_IMDCTWindow[ 256 ] = {
 0x00023A64, 0x0003FEFE, 0x00060139, 0x000851AD,
 0x000AFA57, 0x000E03CE, 0x00117660, 0x00155A62,
 0x0019B855, 0x001E98F2, 0x0024052F, 0x002A0643,
 0x0030A5A6, 0x0037ED12, 0x003FE67D, 0x00489C1C,
 0x00521861, 0x005C65F3, 0x00678FB3, 0x0073A0B3,
 0x0080A435, 0x008EA5A9, 0x009DB0A7, 0x00ADD0EC,
 0x00BF1257, 0x00D180E4, 0x00E528A6, 0x00FA15C8,
 0x01105480, 0x0127F111, 0x0140F7C3, 0x015B74E0,
 0x017774A9, 0x0195035A, 0x01B42D1A, 0x01D4FE00,
 0x01F78203, 0x021BC4FC, 0x0241D29C, 0x0269B668,
 0x02937BB1, 0x02BF2D91, 0x02ECD6E1, 0x031C8239,
 0x034E39E1, 0x038207D3, 0x03B7F5B4, 0x03F00CC7,
 0x042A55F1, 0x0466D9AB, 0x04A5A002, 0x04E6B08D,
 0x052A126B, 0x056FCC3B, 0x05B7E418, 0x06025F93,
 0x064F43B0, 0x069E94DC, 0x06F056F2, 0x07448D2A,
 0x079B3A22, 0x07F45FCE, 0x084FFF7D, 0x08AE19D1,
 0x090EAEBE, 0x0971BD85, 0x09D744B4, 0x0A3F421F,
 0x0AA9B2E4, 0x0B169364, 0x0B85DF46, 0x0BF79170,
 0x0C6BA40C, 0x0CE21086, 0x0D5ACF88, 0x0DD5D8FF,
 0x0E532419, 0x0ED2A747, 0x0F54583E, 0x0FD82BF6,
 0x105E16B1, 0x10E60BFA, 0x116FFEA5, 0x11FBE0D8,
 0x1289A40A, 0x13193909, 0x13AA8FFC, 0x143D9867,
 0x14D24134, 0x156878B2, 0x16002C9E, 0x16994A27,
 0x1733BDF6, 0x17CF7431, 0x186C5884, 0x190A5627,
 0x19A957E3, 0x1A49481D, 0x1AEA10DB, 0x1B8B9BCB,
 0x1C2DD24B, 0x1CD09D75, 0x1D73E622, 0x1E1794F3,
 0x1EBB925D, 0x1F5FC6B0, 0x20041A1C, 0x20A874C1,
 0x214CBEB0, 0x21F0DFFC, 0x2294C0BB, 0x23384914,
 0x23DB6149, 0x247DF1BA, 0x251FE2F5, 0x25C11DBA,
 0x26618B04, 0x27011417, 0x279FA281, 0x283D2027,
 0x28D9774E, 0x297492A1, 0x2A0E5D39, 0x2AA6C2A5,
 0x2B3DAEF4, 0x2BD30EB8, 0x2C66CF10, 0x2CF8DDB0,
 0x2D8928E4, 0x2E179F97, 0x2EA4315D, 0x2F2ECE73,
 0x2FB767C7, 0x303DEEFD, 0x30C25674, 0x3144914A,
 0x31C4935C, 0x32425150, 0x32BDC094, 0x3336D760,
 0x33AD8CBB, 0x3421D87C, 0x3493B34A, 0x3503169C,
 0x356FFCBF, 0x35DA60CF, 0x36423EBD, 0x36A79349,
 0x370A5C06, 0x376A9754, 0x37C84460, 0x38236321,
 0x387BF457, 0x38D1F985, 0x392574F0, 0x39766999,
 0x39C4DB3A, 0x3A10CE42, 0x3A5A47CD, 0x3AA14DA2,
 0x3AE5E62B, 0x3B281872, 0x3B67EC16, 0x3BA56948,
 0x3BE098C7, 0x3C1983D0, 0x3C503421, 0x3C84B3EC,
 0x3CB70DCF, 0x3CE74CCF, 0x3D157C51, 0x3D41A80E,
 0x3D6BDC0E, 0x3D9424A1, 0x3DBA8E54, 0x3DDF25EB,
 0x3E01F858, 0x3E2312B6, 0x3E42823D, 0x3E60543C,
 0x3E7C9613, 0x3E975529, 0x3EB09EE5, 0x3EC880AB,
 0x3EDF07CD, 0x3EF4418E, 0x3F083B14, 0x3F1B0163,
 0x3F2CA15D, 0x3F3D27B4, 0x3F4CA0EB, 0x3F5B194D,
 0x3F689CEB, 0x3F753797, 0x3F80F4DF, 0x3F8BE00B,
 0x3F960417, 0x3F9F6BB3, 0x3FA82140, 0x3FB02ECC,
 0x3FB79E10, 0x3FBE7872, 0x3FC4C6FF, 0x3FCA926E,
 0x3FCFE31E, 0x3FD4C114, 0x3FD933FE, 0x3FDD4332,
 0x3FE0F5AF, 0x3FE4521C, 0x3FE75ECC, 0x3FEA21BE,
 0x3FECA09D, 0x3FEEE0C4, 0x3FF0E741, 0x3FF2B8D3,
 0x3FF459EF, 0x3FF5CEC2, 0x3FF71B33, 0x3FF842E7,
 0x3FF94942, 0x3FFA3169, 0x3FFAFE46, 0x3FFBB28C,
 0x3FFC50B8, 0x3FFCDB13, 0x3FFD53B6, 0x3FFDBC8E,
 0x3FFE175B, 0x3FFE65B7, 0x3FFEA914, 0x3FFEE2C4,
 0x3FFF13F6, 0x3FFF3DBA, 0x3FFF6106, 0x3FFF7EB6,
 0x3FFF978C, 0x3FFFAC35, 0x3FFFBD4C, 0x3FFFCB58,
 0x3FFFD6CF, 0x3FFFE019, 0x3FFFE790, 0x3FFFED82,
 0x3FFFF233, 0x3FFFF5DD, 0x3FFFF8AF, 0x3FFFFAD4,
 0x3FFFFC70, 0x3FFFFD9E, 0x3FFFFE77, 0x3FFFFF0E,
 0x3FFFFF75, 0x3FFFFFB7, 0x3FFFFFE0, 0x3FFFFFF6
};

static sample_t s_Roots16[ 3 ] = {
 0x3B20D79E, 0x2D413CCC, 0x187DE2A6
};

static sample_t s_Roots32[ 7 ] = {
 0x3EC52F9F, 0x3B20D79E, 0x3536CC52, 0x2D413CCC,
 0x238E7673, 0x187DE2A6, 0x0C7C5C1E
};

static sample_t s_Roots64[ 15 ] = {
 0x3FB11B47, 0x3EC52F9F, 0x3D3E82AD, 0x3B20D79E,
 0x387165E3, 0x3536CC52, 0x317900D6, 0x2D413CCC,
 0x2899E64A, 0x238E7673, 0x1E2B5D38, 0x187DE2A6,
 0x1294062E, 0x0C7C5C1E, 0x0645E9AF
};

static sample_t s_Roots128[ 31 ] = {
 0x3FEC43C6, 0x3FB11B47, 0x3F4EAAFE, 0x3EC52F9F,
 0x3E14FDF7, 0x3D3E82AD, 0x3C424209, 0x3B20D79E,
 0x39DAF5E8, 0x387165E3, 0x36E5068A, 0x3536CC52,
 0x3367C08F, 0x317900D6, 0x2F6BBE44, 0x2D413CCC,
 0x2AFAD269, 0x2899E64A, 0x261FEFF9, 0x238E7673,
 0x20E70F32, 0x1E2B5D38, 0x1B5D1009, 0x187DE2A6,
 0x158F9A75, 0x1294062E, 0x0F8CFCBD, 0x0C7C5C1E,
 0x09640837, 0x0645E9AF, 0x0323ECBE
};

static sample_t s_Pre1[ 256 ] = {
 0x2D64B9DA, 0x2D1DA3D5, 0x003243F1, 0x3FFFEC42,
 0x18AC4B86, 0x3B0D8908, 0xE7B09556, 0x3B3401BB,
 0x23B836C9, 0x351ACEDC, 0xF3B4F46D, 0x3ECEEAAD,
 0xDC9B5FD2, 0x3552A8F4, 0x0CADA4F4, 0x3EBB4DDA,
 0x28C0B4D2, 0x31590E3D, 0xF9EC1E3C, 0x3FB5F4EA,
 0x12C41A4E, 0x3D2FD86C, 0xE201009A, 0x38890662,
 0xD78D014A, 0x3198D4EA, 0x0677EDBA, 0x3FAC1A5B,
 0x1E57A86D, 0x3859A292, 0xED9C1967, 0x3D4D0727,
 0x2B2003AB, 0x2F49EE0F, 0xFD0E48AC, 0x3FEEA776,
 0x15BEE78B, 0x3C31405F, 0xE4D068E3, 0x39F061D1,
 0x21122240, 0x36CB1E29, 0xF0A3CA5D, 0x3E212179,
 0xDA087B6A, 0x3385A221, 0x0995BDFC, 0x3F473758,
 0xD52A795D, 0x2F8D7139, 0x03562037, 0x3FE9B8A9,
 0x1B8A7814, 0x39C5664F, 0xEA9FBFEE, 0x3C531E88,
 0x264843D8, 0x3349BF48, 0xF6CDB35A, 0x3F55F796,
 0xDF441828, 0x36FECD0D, 0x0FBDBA40, 0x3E08B429,
 0x2C45C89F, 0x2E37592C, 0xFEA02B2E, 0x3FFC38D0,
 0x173763C9, 0x3BA3FDE7, 0xE63E82BC, 0x3A96B636,
 0x2267D39F, 0x35F71FB1, 0xF22B4E66, 0x3E7CD778,
 0xDB4F1967, 0x34703094, 0x0B228D41, 0x3F061E94,
 0x27878893, 0x3255483F, 0xF85C5201, 0x3F8ADC76,
 0x11423EEF, 0x3DA106BD, 0xE0A0211A, 0x37C836C2,
 0xD6588726, 0x3096E223, 0x04E767C4, 0x3FCFD50A,
 0x1CF34BAE, 0x3913EB0E, 0xEC1C6417, 0x3CD4C38A,
 0xD4030685, 0x2E7CAB1C, 0x01C454F4, 0x3FF9C139,
 0x1A1D6543, 0x3A6DF8F7, 0xE926679D, 0x3BC82C1E,
 0x250317DE, 0x34364DA5, 0xF5407FBD, 0x3F174E6F,
 0xDDED1B6F, 0x362CE854, 0x0E36C829, 0x3E66D0B4,
 0x29F3984B, 0x30553827, 0xFB7CDA63, 0x3FD73A4A,
 0x144310DC, 0x3CB53AAA, 0xE366803D, 0x39411E33,
 0xD8C7B839, 0x329321C7, 0x08077456, 0x3F7E8E1E,
 0x1FB7575C, 0x3796A996, 0xEF1EA4B2, 0x3DBBD6D4,
 0xD329E182, 0xD254A022, 0x0096CB58, 0xC000B1A7,
 0xE80DB22D, 0xC4A617A7, 0x1908EF81, 0xC519815F,
 0xDCEF4DC2, 0xCA760086, 0x0D101F0D, 0xC158E9C1,
 0x240B7542, 0xCB1D8E43, 0xF417AC23, 0xC11E137A,
 0xD7DB1B34, 0xCE27DECA, 0x06DBE9BB, 0xC05E5D4F,
 0xEDFC7A7D, 0xC296615E, 0x1EB00695, 0xC7D64C48,
 0x290E0660, 0xCEE73232, 0xFA503931, 0xC040CDBB,
 0xE259F3A4, 0xC748214D, 0x13241FB6, 0xC2EDED49,
 0xD5756016, 0xD02F80F1, 0x03BA80DF, 0xC01BD3D7,
 0xEAFE9C24, 0xC38B9828, 0x1BE51517, 0xC66623BF,
 0xDF9AA355, 0xC8CE0BC1, 0x101F1806, 0xC2105236,
 0x2698A4A5, 0xCCF2A21E, 0xF7313B60, 0xC09BE473,
 0x2B6A164C, 0xD0FA09C9, 0xFD72B8D3, 0xC00D077C,
 0xE52B8CEE, 0xC5E531A2, 0x161D595C, 0xC3F1324E,
 0xDA59985A, 0xCC3EFA25, 0x09F917AB, 0xC0C82507,
 0x21680B0F, 0xC96917ED, 0xF10574E1, 0xC1C70A84,
 0x2C8E2A86, 0xD20E6ACD, 0xFF04AEB5, 0xC001ED79,
 0xE69AAA49, 0xC5411D1B, 0x1794F5E6, 0xC480C379,
 0xDBA1A534, 0xCB566DDF, 0x0B857EC6, 0xC10BACC8,
 0x22BC6DC9, 0xCA3F2E1A, 0xF28D8716, 0xC16DBBF3,
 0xD6A50D5E, 0xCF27EBC5, 0x054B9DD2, 0xC0382DA9,
 0xEC7C0A1D, 0xC30C49AD, 0x1D4CD02B, 0xC719D4ED,
 0x27D667D5, 0xCDE90D7A, 0xF8C02B32, 0xC06971F9,
 0xE0F7E6FA, 0xC806C5B5, 0x11A2F7FB, 0xC27A616A,
 0xD44C4232, 0xD13E75A8, 0x0228D0BB, 0xC0095438,
 0xE9846B64, 0xC414392B, 0x1A790CD3, 0xC5BB5473,
 0xDE425E90, 0xC99DD4B5, 0x0E98BBA6, 0xC1AFD008,
 0x2554EDD0, 0xCC04161E, 0xF5A3A741, 0xC0D81D61,
 0x2A3F5039, 0xCFECE916, 0xFBE127AD, 0xC021FDFB,
 0xE3C092B9, 0xC6923BEC, 0x14A253D1, 0xC36AE401,
 0xD91759C9, 0xCD2F817B, 0x086B26DE, 0xC08E5CE6,
 0x200E8190, 0xC89B6CBF, 0xEF7FB1FB, 0xC229F168
};

static sample_t s_Post1[ 128 ] = {
 0x3FFFB10B, 0x006487C3, 0x3FFD3968, 0x012D936B,
 0x3FF84A3B, 0x01F69373, 0x3FF0E3B5, 0x02BF801A,
 0x3FE7061F, 0x038851A2, 0x3FDAB1D9, 0x0451004D,
 0x3FCBE75E, 0x0519845E, 0x3FBAA73F, 0x05E1D61A,
 0x3FA6F228, 0x06A9EDC9, 0x3F90C8D9, 0x0771C3B2,
 0x3F782C2F, 0x08395023, 0x3F5D1D1C, 0x09008B6A,
 0x3F3F9CAB, 0x09C76DD8, 0x3F1FABFF, 0x0A8DEFC2,
 0x3EFD4C53, 0x0B540982, 0x3ED87EFB, 0x0C19B374,
 0x3EB14562, 0x0CDEE5F9, 0x3E87A10B, 0x0DA39977,
 0x3E5B9392, 0x0E67C659, 0x3E2D1EA7, 0x0F2B650F,
 0x3DFC4418, 0x0FEE6E0D, 0x3DC905C4, 0x10B0D9CF,
 0x3D9365A7, 0x1172A0D7, 0x3D5B65D1, 0x1233BBAB,
 0x3D21086C, 0x12F422DA, 0x3CE44FB6, 0x13B3CEFA,
 0x3CA53E08, 0x1472B8A5, 0x3C63D5D0, 0x1530D880,
 0x3C201994, 0x15EE2737, 0x3BDA0BEF, 0x16AA9D7D,
 0x3B91AF96, 0x1766340F, 0x3B470752, 0x1820E3B0,
 0x3AFA1605, 0x18DAA52E, 0x3AAADEA5, 0x19937161,
 0x3A596441, 0x1A4B4127, 0x3A05A9FD, 0x1B020D6C,
 0x39AFB313, 0x1BB7CF23, 0x395782D3, 0x1C6C7F49,
 0x38FD1CA4, 0x1D2016E8, 0x38A08402, 0x1DD28F14,
 0x3841BC7F, 0x1E83E0EA, 0x37E0C9C2, 0x1F340596,
 0x377DAF89, 0x1FE2F64B, 0x371871A4, 0x2090AC4D,
 0x36B113FD, 0x213D20E8, 0x36479A8E, 0x21E84D76,
 0x35DC0968, 0x22922B5E, 0x356E64B2, 0x233AB413,
 0x34FEB0A5, 0x23E1E117, 0x348CF190, 0x2487ABF7,
 0x34192BD5, 0x252C0E4E, 0x33A363EB, 0x25CF01C7,
 0x332B9E5D, 0x2670801A, 0x32B1DFC9, 0x2710830B,
 0x32362CDF, 0x27AF0471, 0x31B88A66, 0x284BFE2F,
 0x3138FD34, 0x28E76A37, 0x30B78A35, 0x2981428B,
 0x30343667, 0x2A19813E, 0x2FAF06D9, 0x2AB02071,
 0x2F2800AE, 0x2B451A54, 0x2E9F291B, 0x2BD8692B,
 0x2E148566, 0x2C6A0746, 0x2D881AE7, 0x2CF9EF09
};

static sample_t s_Pre2[ 128 ] = {
 0x3FFFB10B, 0xFF9B783D, 0x2D881AE7, 0x2CF9EF09,
 0x3B470752, 0x1820E3B0, 0x18DAA52E, 0x3AFA1605,
 0x3ED87EFB, 0x0C19B374, 0x23E1E117, 0x34FEB0A5,
 0x0CDEE5F9, 0x3EB14562, 0x356E64B2, 0x233AB413,
 0x3FBAA73F, 0x05E1D61A, 0x28E76A37, 0x3138FD34,
 0x38A08402, 0x1DD28F14, 0x12F422DA, 0x3D21086C,
 0x06A9EDC9, 0x3FA6F228, 0x31B88A66, 0x284BFE2F,
 0x3D5B65D1, 0x1233BBAB, 0x1E83E0EA, 0x3841BC7F,
 0x3FF0E3B5, 0x02BF801A, 0x2B451A54, 0x2F2800AE,
 0x3A05A9FD, 0x1B020D6C, 0x15EE2737, 0x3C201994,
 0x3E2D1EA7, 0x0F2B650F, 0x213D20E8, 0x36B113FD,
 0x09C76DD8, 0x3F3F9CAB, 0x33A363EB, 0x25CF01C7,
 0x038851A2, 0x3FE7061F, 0x2FAF06D9, 0x2AB02071,
 0x3C63D5D0, 0x1530D880, 0x1BB7CF23, 0x39AFB313,
 0x3F5D1D1C, 0x09008B6A, 0x2670801A, 0x332B9E5D,
 0x0FEE6E0D, 0x3DFC4418, 0x371871A4, 0x2090AC4D,
 0x3FFD3968, 0x012D936B, 0x2C6A0746, 0x2E148566,
 0x3AAADEA5, 0x19937161, 0x1766340F, 0x3B91AF96,
 0x3E87A10B, 0x0DA39977, 0x22922B5E, 0x35DC0968,
 0x0B540982, 0x3EFD4C53, 0x348CF190, 0x2487ABF7,
 0x3F90C8D9, 0x0771C3B2, 0x27AF0471, 0x32362CDF,
 0x37E0C9C2, 0x1F340596, 0x1172A0D7, 0x3D9365A7,
 0x0519845E, 0x3FCBE75E, 0x30B78A35, 0x2981428B,
 0x3CE44FB6, 0x13B3CEFA, 0x1D2016E8, 0x38FD1CA4,
 0x01F69373, 0x3FF84A3B, 0x2E9F291B, 0x2BD8692B,
 0x3BDA0BEF, 0x16AA9D7D, 0x1A4B4127, 0x3A596441,
 0x3F1FABFF, 0x0A8DEFC2, 0x252C0E4E, 0x34192BD5,
 0x0E67C659, 0x3E5B9392, 0x36479A8E, 0x21E84D76,
 0x3FDAB1D9, 0x0451004D, 0x2A19813E, 0x30343667,
 0x395782D3, 0x1C6C7F49, 0x1472B8A5, 0x3CA53E08,
 0x08395023, 0x3F782C2F, 0x32B1DFC9, 0x2710830B,
 0x3DC905C4, 0x10B0D9CF, 0x1FE2F64B, 0x377DAF89
};

static sample_t s_Post2[ 64 ] = {
 0x3FFEC42D, 0x00C90E8F, 0x3FF4E5DF, 0x025B0CAE,
 0x3FE12ACB, 0x03ECADCF, 0x3FC395F9, 0x057DB402,
 0x3F9C2BFA, 0x070DE171, 0x3F6AF2E3, 0x089CF867,
 0x3F2FF249, 0x0A2ABB58, 0x3EEB3347, 0x0BB6ECEF,
 0x3E9CC076, 0x0D415012, 0x3E44A5EE, 0x0EC9A7F2,
 0x3DE2F147, 0x104FB80E, 0x3D77B191, 0x11D3443F,
 0x3D02F756, 0x135410C2, 0x3C84D496, 0x14D1E242,
 0x3BFD5CC4, 0x164C7DDD, 0x3B6CA4C4, 0x17C3A931,
 0x3AD2C2E7, 0x19372A63, 0x3A2FCEE8, 0x1AA6C82B,
 0x3983E1E7, 0x1C1249D8, 0x38CF1669, 0x1D79775B,
 0x3811884C, 0x1EDC1952, 0x374B54CE, 0x2039F90E,
 0x367C9A7D, 0x2192E09A, 0x35A5793C, 0x22E69AC7,
 0x34C61236, 0x2434F332, 0x33DE87DE, 0x257DB64B,
 0x32EEFDE9, 0x26C0B162, 0x31F79947, 0x27FDB2A6,
 0x30F8801F, 0x29348937, 0x2FF1D9C6, 0x2A650525,
 0x2EE3CEBE, 0x2B8EF77C, 0x2DCE88A9, 0x2CB2324B
};

static uint32_t s_FFTOrder[ 128 ] = {
   0, 128, 64, 192, 32, 160, 224,  96,  16, 144, 80, 208, 240, 112,  48, 176,
   8, 136, 72, 200, 40, 168, 232, 104, 248, 120, 56, 184,  24, 152, 216,  88,
   4, 132, 68, 196, 36, 164, 228, 100,  20, 148, 84, 212, 244, 116,  52, 180,
 252, 124, 60, 188, 28, 156, 220,  92,  12, 140, 76, 204, 236, 108,  44, 172,
   2, 130, 66, 194, 34, 162, 226,  98,  18, 146, 82, 210, 242, 114,  50, 178,
  10, 138, 74, 202, 42, 170, 234, 106, 250, 122, 58, 186,  26, 154, 218,  90,
 254, 126, 62, 190, 30, 158, 222,  94,  14, 142, 78, 206, 238, 110,  46, 174,
   6, 134, 70, 198, 38, 166, 230, 102, 246, 118, 54, 182,  22, 150, 214,  86
};

static void SMS_INLINE _ifft2 ( sample_t* apBuf ) {

 __asm__ __volatile__ (
  ".set noat\n\t"
  "ld        $t0,  0(%0)\n\t"
  "ld        $t1,  8(%0)\n\t"
  "pcpyld    $t0, $t0, $t0\n\t"
  "ld        $t0, 16(%0)\n\t"
  "pcpyld    $t1, $t1, $t1\n\t"
  "ld        $t1, 24(%0)\n\t"
  "paddw     $at, $t0, $t1\n\t"
  "psubw     $t0, $t0, $t1\n\t"
  "sd        $at, 16(%0)\n\t"
  "pcpyud    $at, $at, $at\n\t"
  "sd        $t0, 24(%0)\n\t"
  "pcpyud    $t0, $t0, $t0\n\t"
  "sd        $at,  0(%0)\n\t"
  "sd        $t0,  8(%0)\n\t"
  ".set at\n\t"
  :: "r"( apBuf ) : "t0", "t1", "at", "memory"
 );

}  /* end _ifft2 */

static void SMS_INLINE _ifft4 ( int* apBuf ) {

 __asm__ __volatile__(
  ".set noat\n\t"
  "lq        $t0,  0(%0)\n\t"
  "lq        $t1, 16(%0)\n\t"
  "mtsah     $zero, -2\n\t"
  "pexcw     $t1, $t1\n\t"
  "prot3w    $t1, $t1\n\t"
  "pcpyld    $at, $t1, $t0\n\t"
  "pcpyud    $t0, $t0, $t1\n\t"
  "paddw     $t1, $at, $t0\n\t"
  "psubw     $t0, $at, $t0\n\t"
  "prot3w    $t0, $t0\n\t"
  "pexcw     $t0, $t0\n\t"
  "pcpyld    $at, $t0, $t1\n\t"
  "pcpyud    $t1, $t1, $t0\n\t"
  "paddw     $t0, $at, $t1\n\t"
  "psubw     $t1, $at, $t1\n\t"
  "qfsrv     $t0, $t0, $t0\n\t"
  "qfsrv     $t1, $t1, $t1\n\t"
  "prot3w    $t0, $t0\n\t"
  "prot3w    $t1, $t1\n\t"
  "sq        $t0,  0(%0)\n\t"
  "sq        $t1, 16(%0)\n\t"
  ".set at\n\t"
  :: "r"( apBuf ) : "t0", "t1", "at", "memory"
 );

}  /* end _ifft4 */

extern void BUTTERFLY_0 ( sample_t, sample_t, sample_t, sample_t, sample_t*, sample_t* );
__asm__(
 ".set noreorder\n\t"
 "BUTTERFLY_0:\n\t"
 "pextlw    $a0, $a1, $a0\n\t"
 "pextlw    $a2, $a3, $a2\n\t"
 "pcpyld    $a0, $a0, $a0\n\t"
 "pcpyld    $a2, $a2, $a2\n\t"
 "pexcw     $a2, $a2\n\t"
 "pexew     $a2, $a2\n\t"
 "prevh     $a2, $a2\n\t"
 "phmaddh   $a1, $a0, $a2\n\t"
 "psrlw     $a0, $a0, 16\n\t"
 "pmulth    $a0, $a0, $a2\n\t"
 "mtsah     $zero, 2\n\t"
 "psraw     $a1, $a1, 14\n\t"
 "psllw     $a0, $a0, 2\n\t"
 "paddw     $v0, $a0, $a1\n\t"
 "qfsrv     $v1, $v0, $v0\n\t"
 "paddw     $a0, $v0, $v1\n\t"
 "psubw     $a1, $v0, $v1\n\t"
 "psravw    $v0, $a0, $zero\n\t"
 "psravw    $v1, $a1, $zero\n\t"
 "sw        $v0, 0($t0)\n\t"
 "pcpyud    $v1, $v1, $zero\n\t"
 "jr        $ra\n\t"
 "sw        $v1, 0($t1)\n\t"
 ".set reorder\n\t"
);

extern void BUTTERFLY_ZERO ( sample_t*, sample_t*, sample_t*, sample_t* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 "BUTTERFLY_ZERO:\n\t"
 "ld        $t0, 0($a2)\n\t"
 "ld        $t1, 0($a3)\n\t"
 "pxor      $at, $at, $at\n\t"
 "nor       $at, $zero, $zero\n\t"
 "pexew     $at, $at\n\t"
 "psrlw     $t2, $at, 31\n\t"
 "pextlw    $v0, $t1, $t0\n\t"
 "pextlw    $v1, $t0, $t1\n\t"
 "mtsah     $zero, 4\n\t"
 "pxor      $at, $v1, $at\n\t"
 "paddw     $at, $at, $t2\n\t"
 "paddw     $at, $v0, $at\n\t"
 "ld        $t0, 0($a0)\n\t"
 "pexew     $at, $at\n\t"
 "ld        $t1, 0($a1)\n\t"
 "qfsrv     $at, $at, $at\n\t"
 "pcpyld    $t0, $t1, $t0\n\t"
 "psubw     $t1, $t0, $at\n\t"
 "paddw     $t0, $t0, $at\n\t"
 "sd        $t1, 0($a2)\n\t"
 "pcpyud    $t1, $t1, $t1\n\t"
 "sd        $t0, 0($a0)\n\t"
 "pcpyud    $t0, $t0, $t0\n\t"
 "sd        $t1, 0($a3)\n\t"
 "jr        $ra\n\t"
 "sd        $t0, 0($a1)\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

extern void BUTTERFLY_HALF ( sample_t*, sample_t*, sample_t*, sample_t*, sample_t );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 "BUTTERFLY_HALF:\n\t"
 "lw      $t1, 0($a2)\n\t"
 "lw      $t2, 4($a2)\n\t"
 "lw      $t3, 0($a3)\n\t"
 "lw      $t4, 4($a3)\n\t"
 "addu    $v0, $t1, $t2\n\t"
 "subu    $v1, $t2, $t1\n\t"
 "pextlw  $v0, $v1, $v0\n\t"
 "subu    $v1, $t3, $t4\n\t"
 "addu    $at, $t3, $t4\n\t"
 "pextlw  $v1, $at, $v1\n\t"
 "pcpyld  $v0, $v1, $v0\n\t"
 "pextlw  $t0, $t0, $t0\n\t"
 "pcpyld  $t0, $t0, $t0\n\t"
 "prevh   $t0, $t0\n\t"
 "phmadh  $at, $v0, $t0\n\t"
 "psraw   $v0, $v0, 16\n\t"
 "pmulth  $v0, $v0, $t0\n\t"
 "mtsah   $zero, 2\n\t"
 "psraw   $at, $at, 14\n\t"
 "psllw   $v0, $v0, 2\n\t"
 "paddw   $v0, $v0, $at\n\t"
 "qfsrv   $v0, $v0, $v0\n\t"
 "pcpyud  $at, $v0, $v0\n\t"
 "paddw   $v1, $v0, $at\n\t"
 "psubw   $v0, $v0, $at\n\t"
 "pcpyld  $v0, $v0, $v1\n\t"
 "prot3w  $v0, $v0\n\t"
 "pexcw   $v0, $v0\n\t"
 "ld      $t0, 0($a0)\n\t"
 "ld      $t1, 0($a1)\n\t"
 "pcpyld  $t0, $t1, $t0\n\t"
 "psubw   $t1, $t0, $v0\n\t"
 "paddw   $t0, $t0, $v0\n\t"
 "pcpyud  $v0, $t1, $t1\n\t"
 "pcpyud  $v1, $t0, $t0\n\t"
 "sd      $t0, 0($a0)\n\t"
 "sd      $v1, 0($a1)\n\t"
 "sd      $t1, 0($a2)\n\t"
 "jr      $ra\n\t"
 "sd      $v0, 0($a3)\n\t"
 ".set reorder\n\t"
 ".set macro\n\t"
 ".set at\n\t"
);

static void BUTTERFLY ( sample_t* a0, sample_t* a1, sample_t* a2, sample_t* a3, sample_t wr, sample_t wi ) {

 BUTTERFLY_0 (  wr, wi, a2[ 0 ], a2[ 1 ], a2, a2 + 1  );
 BUTTERFLY_0 (  wr, wi, a3[ 1 ], a3[ 0 ], a3 + 1, a3  );
 BUTTERFLY_ZERO ( a0, a1, a2, a3 );

}  /* end BUTTERFLY */

static SMS_INLINE void _ifft8 ( sample_t* apBuf ) {

 _ifft4 ( apBuf      );
 _ifft2 ( apBuf +  8 );

 BUTTERFLY_ZERO ( apBuf + 0, apBuf + 4, apBuf +  8, apBuf + 12 );
 BUTTERFLY_HALF ( apBuf + 2, apBuf + 6, apBuf + 10, apBuf + 14, s_Roots16[ 1 ] );

}  /* end _ifft8 */

static void _ifft_pass ( sample_t* apBuf, sample_t* apWeight, int aN ) {

 sample_t* lpBuf1;
 sample_t* lpBuf2;
 sample_t* lpBuf3;
 int       i;

 apBuf += 2;
 lpBuf1 = apBuf + (      ( aN << 1 )  );
 lpBuf2 = apBuf + (  2 * ( aN << 1 )  );
 lpBuf3 = apBuf + (  3 * ( aN << 1 )  );

 BUTTERFLY_ZERO( apBuf - 2, lpBuf1 - 2, lpBuf2 - 2, lpBuf3 - 2 );

 i = aN - 1;

 do {

  BUTTERFLY( apBuf, lpBuf1, lpBuf2, lpBuf3, apWeight[ 0 ], apWeight[ 2 * i - aN ] );
  apBuf  += 2;
  lpBuf1 += 2;
  lpBuf2 += 2;
  lpBuf3 += 2;
  ++apWeight;

 } while ( --i );

}  /* end _ifft_pass */

static void _ifft16 ( sample_t* apBuf ) {

 _ifft8 ( apBuf      );
 _ifft4 ( apBuf + 16 );
 _ifft4 ( apBuf + 24 );
 _ifft_pass ( apBuf, s_Roots16, 4 );

}  /* end _ifft16 */

static void _ifft32 ( sample_t* apBuf ) {

 _ifft16 ( apBuf      );
 _ifft8  ( apBuf + 32 );
 _ifft8  ( apBuf + 48 );
 _ifft_pass ( apBuf, s_Roots32, 8 );

}  /* end _ifft32 */

static void _ifft64 ( sample_t* apBuf ) {

 _ifft32 ( apBuf      );
 _ifft16 ( apBuf + 64 );
 _ifft16 ( apBuf + 96 );
 _ifft_pass ( apBuf, s_Roots64, 16 );

}  /* end _ifft64 */

static void _ifft128 ( sample_t* apBuf ) {

 _ifft32 ( apBuf );
 _ifft16 ( apBuf + 64 );
 _ifft16 ( apBuf + 96 );
 _ifft_pass ( apBuf, s_Roots64, 16 );

 _ifft32 ( apBuf + 128 );
 _ifft32 ( apBuf + 192 );
 _ifft_pass ( apBuf, s_Roots128, 32 );

}  /* end _ifft128 */

void _ac3_imdct_512 ( sample_t* apData, sample_t* apDelay, sample_t aBias ) {

 const sample_t* lpWindow = s_IMDCTWindow;

 int       i, j, k;
 sample_t  t_r, t_i, a_r, a_i, b_r, b_i, w_1, w_2;
 sample_t* lBuf = ( sample_t* )0x70000000;
	
 for ( i = 0, j = 0; i < 128; ++i, j += 2 ) {

  k = s_FFTOrder[ i ];
  t_r = s_Pre1[ j + 0 ];
  t_i = s_Pre1[ j + 1 ];

  BUTTERFLY_0 ( t_r, t_i, apData[ k ], apData[ 255 - k ], &lBuf[ j ], &lBuf[ j + 1 ] );

 }  /* end for */

 _ifft128 ( lBuf );

 for ( i = 0, j = 0; i < 64; ++i, j += 2 ) {

  t_r = s_Post1[ j + 0 ];
  t_i = s_Post1[ j + 1 ];

  BUTTERFLY_0 ( t_i, t_r, lBuf[ j + 1 ], lBuf[ j ], &a_r, &a_i );
  BUTTERFLY_0 ( t_r, t_i, lBuf[ 254 - j + 1 ], lBuf[ 254 - j ], &b_r, &b_i );

  w_1 = lpWindow[       j ];
  w_2 = lpWindow[ 255 - j ];

  BUTTERFLY_0 ( w_2, w_1, a_r, apDelay[ j ], &apData[ 255 - j ], &apData[ j ] );

  apDelay[ j ] = a_i;

  w_1 = lpWindow[ j + 1   ];
  w_2 = lpWindow[ 254 - j ];

  BUTTERFLY_0 ( w_1, w_2, b_r, apDelay[ j + 1 ], &apData[ j + 1 ], &apData[ 254 - j ] );

  apDelay[ j + 1 ] = b_i;

 }  /* end for */

}  /* end _ac3_imdct_512 */

void _ac3_imdct_256 ( sample_t* apData, sample_t* apDelay, sample_t aBias ) {

 const sample_t* lpWindow = s_IMDCTWindow;

 int       i, j, k;
 sample_t  t_r, t_i, a_r, a_i, b_r, b_i, c_r, c_i, d_r, d_i, w_1, w_2;
 sample_t* lBuf1 = ( sample_t* )0x70000000;
 sample_t* lBuf2 = ( sample_t* )0x70000200;

 for ( i = 0, j = 0; i < 64; ++i, j += 2 ) {

  k   = s_FFTOrder[ i ];
  t_r = s_Pre2[ j + 0 ];
  t_i = s_Pre2[ j + 1 ];

  BUTTERFLY_0 ( t_r, t_i, apData[ k + 0 ], apData[ 254 - k ], &lBuf1[ j ], &lBuf1[ j + 1 ] );
  BUTTERFLY_0 ( t_r, t_i, apData[ k + 1 ], apData[ 255 - k ], &lBuf2[ j ], &lBuf2[ j + 1 ] );

 }  /* end for */

 _ifft64 ( lBuf1 );
 _ifft64 ( lBuf2 );

 for ( i = 0, j = 0; i < 32; ++i, j += 2 ) {

  t_r = s_Post2[ j + 0 ];
  t_i = s_Post2[ j + 1 ];

  BUTTERFLY_0 ( t_i, t_r, lBuf1[       j + 1 ], lBuf1[       j ], &a_r, &a_i );
  BUTTERFLY_0 ( t_r, t_i, lBuf1[ 126 - j + 1 ], lBuf1[ 126 - j ], &b_r, &b_i );
  BUTTERFLY_0 ( t_i, t_r, lBuf2[       j + 1 ], lBuf2[       j ], &c_r, &c_i );
  BUTTERFLY_0 ( t_r, t_i, lBuf2[ 126 - j + 1 ], lBuf2[ 126 - j ], &d_r, &d_i );

  w_1 = lpWindow[       j ];
  w_2 = lpWindow[ 255 - j ];

  BUTTERFLY_0 ( w_2, w_1, a_r, apDelay[ j ], &apData[ 255 - j ], &apData[ j ] );

  apDelay[ j ] = c_i;

  w_1 = lpWindow[ 128 + j ];
  w_2 = lpWindow[ 127 - j ];

  BUTTERFLY_0 ( w_1, w_2, a_i, apDelay[ 127 - j ], &apData[ 128 + j ], &apData[ 127 - j ] );

  apDelay[ 127 - j ] = c_r;

  w_1 = lpWindow[ j + 1   ];
  w_2 = lpWindow[ 254 - j ];

  BUTTERFLY_0 ( w_2, w_1, b_i, apDelay[ j + 1 ], &apData[ 254 - j ], &apData[ j + 1 ] );

  apDelay[ j + 1 ] = d_r;

  w_1 = lpWindow[ 129 + j ];
  w_2 = lpWindow[ 126 - j ];

  BUTTERFLY_0 ( w_1, w_2, b_r, apDelay[ 126 - j ], &apData[ 129 + j ], &apData[ 126 - j ] );

  apDelay[ 126 - j ] = d_i;

 }  /* end for */

}  /* end _ac3_imdct_256 */
