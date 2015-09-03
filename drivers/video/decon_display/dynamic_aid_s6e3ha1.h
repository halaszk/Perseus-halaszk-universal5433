#ifndef __DYNAMIC_AID_XXXX_H
#define __DYNAMIC_AID_XXXX_H __FILE__

#include "dynamic_aid.h"
#include "dynamic_aid_gamma_curve.h"

enum {
	IV_VT,
	IV_3,
	IV_11,
	IV_23,
	IV_35,
	IV_51,
	IV_87,
	IV_151,
	IV_203,
	IV_255,
	IV_MAX,
};

enum {
	IBRIGHTNESS_4NT,
	IBRIGHTNESS_5NT,
	IBRIGHTNESS_6NT,
	IBRIGHTNESS_7NT,
	IBRIGHTNESS_8NT,
	IBRIGHTNESS_9NT,
	IBRIGHTNESS_10NT,
	IBRIGHTNESS_11NT,
	IBRIGHTNESS_12NT,
	IBRIGHTNESS_13NT,
	IBRIGHTNESS_14NT,
	IBRIGHTNESS_15NT,
	IBRIGHTNESS_16NT,
	IBRIGHTNESS_17NT,
	IBRIGHTNESS_19NT,
	IBRIGHTNESS_20NT,
	IBRIGHTNESS_21NT,
	IBRIGHTNESS_22NT,
	IBRIGHTNESS_24NT,
	IBRIGHTNESS_25NT,
	IBRIGHTNESS_27NT,
	IBRIGHTNESS_29NT,
	IBRIGHTNESS_30NT,
	IBRIGHTNESS_32NT,
	IBRIGHTNESS_34NT,
	IBRIGHTNESS_37NT,
	IBRIGHTNESS_39NT,
	IBRIGHTNESS_41NT,
	IBRIGHTNESS_44NT,
	IBRIGHTNESS_47NT,
	IBRIGHTNESS_50NT,
	IBRIGHTNESS_53NT,
	IBRIGHTNESS_56NT,
	IBRIGHTNESS_60NT,
	IBRIGHTNESS_64NT,
	IBRIGHTNESS_68NT,
	IBRIGHTNESS_72NT,
	IBRIGHTNESS_77NT,
	IBRIGHTNESS_82NT,
	IBRIGHTNESS_87NT,
	IBRIGHTNESS_93NT,
	IBRIGHTNESS_98NT,
	IBRIGHTNESS_105NT,
	IBRIGHTNESS_111NT,
	IBRIGHTNESS_119NT,
	IBRIGHTNESS_126NT,
	IBRIGHTNESS_134NT,
	IBRIGHTNESS_143NT,
	IBRIGHTNESS_152NT,
	IBRIGHTNESS_162NT,
	IBRIGHTNESS_172NT,
	IBRIGHTNESS_183NT,
	IBRIGHTNESS_195NT,
	IBRIGHTNESS_207NT,
	IBRIGHTNESS_220NT,
	IBRIGHTNESS_234NT,
	IBRIGHTNESS_249NT,
	IBRIGHTNESS_265NT,
	IBRIGHTNESS_282NT,
	IBRIGHTNESS_300NT,
	IBRIGHTNESS_400NT,
	IBRIGHTNESS_MAX
};

#define VREG_OUT_X1000		6800	/* VREG_OUT x 1000 */

static const int index_voltage_table[IBRIGHTNESS_MAX] = {
	0,	/* IV_VT */
	3,	/* IV_3 */
	11,	/* IV_11 */
	23,	/* IV_23 */
	35,	/* IV_35 */
	51,	/* IV_51 */
	87,	/* IV_87 */
	151,	/* IV_151 */
	203,	/* IV_203 */
	255	/* IV_255 */
};

static const int index_brightness_table[IBRIGHTNESS_MAX] = {
	4,	/* IBRIGHTNESS_4NT */
	5,	/* IBRIGHTNESS_5NT */
	6,	/* IBRIGHTNESS_6NT */
	7,	/* IBRIGHTNESS_7NT */
	8,	/* IBRIGHTNESS_8NT */
	9,	/* IBRIGHTNESS_9NT */
	10,	/* IBRIGHTNESS_10NT */
	11,	/* IBRIGHTNESS_11NT */
	12,	/* IBRIGHTNESS_12NT */
	13,	/* IBRIGHTNESS_13NT */
	14,	/* IBRIGHTNESS_14NT */
	15,	/* IBRIGHTNESS_15NT */
	16,	/* IBRIGHTNESS_16NT */
	17,	/* IBRIGHTNESS_17NT */
	19,	/* IBRIGHTNESS_19NT */
	20,	/* IBRIGHTNESS_20NT */
	21,	/* IBRIGHTNESS_21NT */
	22,	/* IBRIGHTNESS_22NT */
	24,	/* IBRIGHTNESS_24NT */
	25,	/* IBRIGHTNESS_25NT */
	27,	/* IBRIGHTNESS_27NT */
	29,	/* IBRIGHTNESS_29NT */
	30,	/* IBRIGHTNESS_30NT */
	32,	/* IBRIGHTNESS_32NT */
	34,	/* IBRIGHTNESS_34NT */
	37,	/* IBRIGHTNESS_37NT */
	39,	/* IBRIGHTNESS_39NT */
	41,	/* IBRIGHTNESS_41NT */
	44,	/* IBRIGHTNESS_44NT */
	47,	/* IBRIGHTNESS_47NT */
	50,	/* IBRIGHTNESS_50NT */
	53,	/* IBRIGHTNESS_53NT */
	56,	/* IBRIGHTNESS_56NT */
	60,	/* IBRIGHTNESS_60NT */
	64,	/* IBRIGHTNESS_64NT */
	68,	/* IBRIGHTNESS_68NT */
	72,	/* IBRIGHTNESS_72NT */
	77,	/* IBRIGHTNESS_77NT */
	82,	/* IBRIGHTNESS_82NT */
	87,	/* IBRIGHTNESS_87NT */
	93,	/* IBRIGHTNESS_93NT */
	98,	/* IBRIGHTNESS_98NT */
	105,	/* IBRIGHTNESS_105NT */
	111,	/* IBRIGHTNESS_111NT */
	119,	/* IBRIGHTNESS_119NT */
	126,	/* IBRIGHTNESS_126NT */
	134,	/* IBRIGHTNESS_134NT */
	143,	/* IBRIGHTNESS_143NT */
	152,	/* IBRIGHTNESS_152NT */
	162,	/* IBRIGHTNESS_162NT */
	172,	/* IBRIGHTNESS_172NT */
	183,	/* IBRIGHTNESS_183NT */
	195,	/* IBRIGHTNESS_195NT */
	207,	/* IBRIGHTNESS_207NT */
	220,	/* IBRIGHTNESS_220NT */
	234,	/* IBRIGHTNESS_234NT */
	249,	/* IBRIGHTNESS_249NT */
	265,	/* IBRIGHTNESS_265NT */
	282,	/* IBRIGHTNESS_282NT */
	300,	/* IBRIGHTNESS_300NT */
	300	/* IBRIGHTNESS_400NT */
};

static const int gamma_default_0[IV_MAX*CI_MAX] = {
	0x00, 0x00, 0x00,	/* IV_VT */
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x80, 0x80, 0x80,
	0x100, 0x100, 0x100,	/* IV_255 */
};

static const int *gamma_default = gamma_default_0;

static const struct formular_t gamma_formula[IV_MAX] = {
	{0, 860},	/* IV_VT */
	{64, 320},
	{64, 320},
	{64, 320},
	{64, 320},
	{64, 320},
	{64, 320},
	{64, 320},
	{64, 320},
	{72, 860},	/* IV_255 */
};

static const int vt_voltage_value[] = {
	0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 138, 148, 158, 168, 178, 186
};

static const int brightness_base_table[IBRIGHTNESS_MAX] = {
	113,	/* IBRIGHTNESS_4NT */
	113,	/* IBRIGHTNESS_5NT */
	113,	/* IBRIGHTNESS_6NT */
	113,	/* IBRIGHTNESS_7NT */
	113,	/* IBRIGHTNESS_8NT */
	113,	/* IBRIGHTNESS_9NT */
	113,	/* IBRIGHTNESS_10NT */
	113,	/* IBRIGHTNESS_11NT */
	113,	/* IBRIGHTNESS_12NT */
	113,	/* IBRIGHTNESS_13NT */
	113,	/* IBRIGHTNESS_14NT */
	113,	/* IBRIGHTNESS_15NT */
	113,	/* IBRIGHTNESS_16NT */
	113,	/* IBRIGHTNESS_17NT */
	113,	/* IBRIGHTNESS_19NT */
	113,	/* IBRIGHTNESS_20NT */
	113,	/* IBRIGHTNESS_21NT */
	113,	/* IBRIGHTNESS_22NT */
	113,	/* IBRIGHTNESS_24NT */
	113,	/* IBRIGHTNESS_25NT */
	113,	/* IBRIGHTNESS_27NT */
	113,	/* IBRIGHTNESS_29NT */
	113,	/* IBRIGHTNESS_30NT */
	113,	/* IBRIGHTNESS_32NT */
	113,	/* IBRIGHTNESS_34NT */
	113,	/* IBRIGHTNESS_37NT */
	113,	/* IBRIGHTNESS_39NT */
	113,	/* IBRIGHTNESS_41NT */
	113,	/* IBRIGHTNESS_44NT */
	113,	/* IBRIGHTNESS_47NT */
	113,	/* IBRIGHTNESS_50NT */
	113,	/* IBRIGHTNESS_53NT */
	113,	/* IBRIGHTNESS_56NT */
	113,	/* IBRIGHTNESS_60NT */
	113,	/* IBRIGHTNESS_64NT */
	113,	/* IBRIGHTNESS_68NT */
	113,	/* IBRIGHTNESS_72NT */
	120,	/* IBRIGHTNESS_77NT */
	128,	/* IBRIGHTNESS_82NT */
	136,	/* IBRIGHTNESS_87NT */
	142,	/* IBRIGHTNESS_93NT */
	152,	/* IBRIGHTNESS_98NT */
	161,	/* IBRIGHTNESS_105NT */
	170,	/* IBRIGHTNESS_111NT */
	181,	/* IBRIGHTNESS_119NT */
	189,	/* IBRIGHTNESS_126NT */
	201,	/* IBRIGHTNESS_134NT */
	212,	/* IBRIGHTNESS_143NT */
	225,	/* IBRIGHTNESS_152NT */
	240,	/* IBRIGHTNESS_162NT */
	253,	/* IBRIGHTNESS_172NT */
	253,	/* IBRIGHTNESS_183NT */
	253,	/* IBRIGHTNESS_195NT */
	253,	/* IBRIGHTNESS_207NT */
	253,	/* IBRIGHTNESS_220NT */
	253,	/* IBRIGHTNESS_234NT */
	253,	/* IBRIGHTNESS_249NT */
	265,	/* IBRIGHTNESS_265NT */
	283,	/* IBRIGHTNESS_282NT */
	300,	/* IBRIGHTNESS_300NT */
	300	/* IBRIGHTNESS_400NT */
};

static const int *gamma_curve_tables[IBRIGHTNESS_MAX] = {
	gamma_curve_2p15_table,	/* IBRIGHTNESS_4NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_5NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_6NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_7NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_8NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_9NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_10NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_11NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_12NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_13NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_14NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_15NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_16NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_17NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_19NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_20NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_21NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_22NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_24NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_25NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_27NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_29NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_30NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_32NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_34NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_37NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_39NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_41NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_44NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_47NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_50NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_53NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_56NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_60NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_64NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_68NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_72NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_77NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_82NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_87NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_93NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_98NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_105NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_111NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_119NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_126NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_134NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_143NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_152NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_162NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_172NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_183NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_195NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_207NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_220NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_234NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_249NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_265NT */
	gamma_curve_2p15_table,	/* IBRIGHTNESS_282NT */
	gamma_curve_2p20_table,	/* IBRIGHTNESS_300NT */
	gamma_curve_2p20_table	/* IBRIGHTNESS_400NT */
};

static const int *gamma_curve_lut = gamma_curve_2p20_table;

static const unsigned char aor_cmd[IBRIGHTNESS_MAX][2] = {
	{0x09, 0xBE},	/* IBRIGHTNESS_4NT */
	{0x09, 0xAA},	/* IBRIGHTNESS_5NT */
	{0x09, 0x98},	/* IBRIGHTNESS_6NT */
	{0x09, 0x85},	/* IBRIGHTNESS_7NT */
	{0x09, 0x73},	/* IBRIGHTNESS_8NT */
	{0x09, 0x5F},	/* IBRIGHTNESS_9NT */
	{0x09, 0x4D},	/* IBRIGHTNESS_10NT */
	{0x09, 0x38},	/* IBRIGHTNESS_11NT */
	{0x09, 0x25},	/* IBRIGHTNESS_12NT */
	{0x09, 0x11},	/* IBRIGHTNESS_13NT */
	{0x08, 0xFE},	/* IBRIGHTNESS_14NT */
	{0x08, 0xEA},	/* IBRIGHTNESS_15NT */
	{0x08, 0xD7},	/* IBRIGHTNESS_16NT */
	{0x08, 0xC3},	/* IBRIGHTNESS_17NT */
	{0x08, 0x9B},	/* IBRIGHTNESS_19NT */
	{0x08, 0x87},	/* IBRIGHTNESS_20NT */
	{0x08, 0x73},	/* IBRIGHTNESS_21NT */
	{0x08, 0x5F},	/* IBRIGHTNESS_22NT */
	{0x08, 0x36},	/* IBRIGHTNESS_24NT */
	{0x08, 0x22},	/* IBRIGHTNESS_25NT */
	{0x07, 0xF9},	/* IBRIGHTNESS_27NT */
	{0x07, 0xD1},	/* IBRIGHTNESS_29NT */
	{0x07, 0xBB},	/* IBRIGHTNESS_30NT */
	{0x07, 0x90},	/* IBRIGHTNESS_32NT */
	{0x07, 0x66},	/* IBRIGHTNESS_34NT */
	{0x07, 0x26},	/* IBRIGHTNESS_37NT */
	{0x06, 0xFC},	/* IBRIGHTNESS_39NT */
	{0x06, 0xD1},	/* IBRIGHTNESS_41NT */
	{0x06, 0xA4},	/* IBRIGHTNESS_44NT */
	{0x06, 0x4A},	/* IBRIGHTNESS_47NT */
	{0x06, 0x07},	/* IBRIGHTNESS_50NT */
	{0x05, 0xC6},	/* IBRIGHTNESS_53NT */
	{0x05, 0x7F},	/* IBRIGHTNESS_56NT */
	{0x05, 0x22},	/* IBRIGHTNESS_60NT */
	{0x04, 0xC5},	/* IBRIGHTNESS_64NT */
	{0x04, 0x65},	/* IBRIGHTNESS_68NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_72NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_77NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_82NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_87NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_93NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_98NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_105NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_111NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_119NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_126NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_134NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_143NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_152NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_162NT */
	{0x03, 0xFE},	/* IBRIGHTNESS_172NT */
	{0x03, 0x7B},	/* IBRIGHTNESS_183NT */
	{0x02, 0xE5},	/* IBRIGHTNESS_195NT */
	{0x02, 0x46},	/* IBRIGHTNESS_207NT */
	{0x01, 0x99},	/* IBRIGHTNESS_220NT */
	{0x00, 0xD5},	/* IBRIGHTNESS_234NT */
	{0x00, 0x0A},	/* IBRIGHTNESS_249NT */
	{0x00, 0x0A},	/* IBRIGHTNESS_265NT */
	{0x00, 0x0A},	/* IBRIGHTNESS_282NT */
	{0x00, 0x0A},	/* IBRIGHTNESS_300NT */
	{0x00, 0x0A}	/* IBRIGHTNESS_400NT */
};

static const int offset_gradation[IBRIGHTNESS_MAX][IV_MAX] = {
	/* V0 ~ V255 */
	{0, 46, 46, 44, 42, 37, 28, 19, 10, 0},
	{0, 42, 41, 38, 36, 32, 24, 16, 8, 0},
	{0, 36, 38, 35, 33, 29, 22, 15, 8, 0},
	{0, 36, 36, 32, 29, 26, 20, 13, 6, 0},
	{0, 35, 33, 30, 28, 24, 19, 13, 6, 0},
	{0, 32, 31, 28, 26, 22, 17, 12, 6, 0},
	{0, 33, 29, 26, 24, 20, 16, 11, 6, 0},
	{0, 32, 28, 25, 22, 19, 15, 11, 6, 0},
	{0, 28, 26, 23, 21, 17, 14, 10, 5, 0},
	{0, 27, 25, 22, 20, 16, 13, 10, 5, 0},
	{0, 26, 24, 21, 19, 15, 12, 9, 5, 0},
	{0, 25, 23, 19, 18, 14, 12, 9, 5, 0},
	{0, 25, 22, 18, 17, 14, 12, 8, 5, 0},
	{0, 23, 21, 18, 16, 13, 11, 8, 5, 0},
	{0, 22, 19, 16, 14, 12, 10, 8, 4, 0},
	{0, 21, 19, 16, 14, 11, 10, 8, 4, 0},
	{0, 20, 18, 15, 13, 11, 9, 7, 4, 0},
	{0, 19, 17, 15, 13, 11, 9, 7, 4, 0},
	{0, 19, 17, 14, 12, 10, 9, 7, 4, 0},
	{0, 18, 16, 13, 11, 9, 8, 7, 4, 0},
	{0, 17, 15, 12, 10, 9, 8, 7, 4, 0},
	{0, 16, 14, 11, 10, 8, 7, 6, 4, 0},
	{0, 15, 14, 11, 10, 8, 7, 6, 4, 0},
	{0, 14, 13, 10, 9, 7, 7, 6, 4, 0},
	{0, 13, 12, 9, 8, 7, 7, 6, 4, 0},
	{0, 12, 11, 8, 8, 6, 7, 6, 4, 0},
	{0, 12, 11, 8, 7, 6, 6, 6, 4, 0},
	{0, 11, 10, 7, 7, 5, 6, 5, 3, 0},
	{0, 10, 8, 6, 6, 4, 5, 4, 3, 0},
	{0, 10, 9, 6, 6, 5, 5, 5, 3, 0},
	{0, 9, 8, 6, 6, 5, 5, 4, 3, 0},
	{0, 8, 8, 5, 5, 4, 5, 4, 3, 0},
	{0, 7, 7, 5, 5, 4, 4, 4, 3, 0},
	{0, 6, 6, 4, 5, 3, 4, 4, 3, 0},
	{0, 6, 6, 4, 4, 3, 4, 3, 3, 0},
	{0, 6, 5, 3, 4, 3, 4, 4, 2, 0},
	{0, 5, 5, 3, 4, 2, 3, 4, 2, 0},
	{0, 5, 5, 3, 3, 2, 3, 3, 2, 0},
	{0, 6, 5, 4, 3, 2, 3, 3, 1, 0},
	{0, 5, 5, 3, 2, 3, 4, 3, 2, 0},
	{0, 6, 5, 4, 3, 3, 4, 4, 2, 0},
	{0, 6, 4, 3, 3, 3, 4, 4, 1, 0},
	{0, 6, 5, 3, 2, 3, 4, 4, 2, 0},
	{0, 6, 5, 3, 3, 3, 3, 4, 1, 0},
	{0, 6, 5, 3, 3, 3, 4, 5, 2, 0},
	{0, 7, 4, 3, 3, 3, 4, 5, 3, 0},
	{0, 6, 4, 4, 2, 3, 5, 6, 3, 0},
	{0, 5, 5, 3, 3, 4, 4, 6, 4, 0},
	{0, 5, 5, 4, 3, 4, 4, 6, 3, 0},
	{0, 5, 4, 3, 3, 3, 5, 5, 4, 0},
	{0, 5, 4, 4, 3, 3, 3, 6, 3, 0},
	{0, 4, 4, 3, 2, 3, 3, 5, 3, 0},
	{0, 3, 3, 3, 2, 2, 2, 4, 2, 0},
	{0, 3, 2, 2, 1, 2, 2, 4, 2, 0},
	{0, 2, 2, 2, 1, 1, 1, 3, 1, 0},
	{0, 1, 1, 1, 0, 1, 0, 2, 1, 0},
	{0, 1, 1, 1, 0, 0, 0, 1, 0, 0},
	{0, 1, 0, 0, -1, 0, 0, 1, 0, 0},
	{0, 1, 0, -1, -1, -1, -1, 0, -1, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static const int offset_color[IBRIGHTNESS_MAX][CI_MAX * IV_MAX] = {
	/* V0 ~ V255 */
	{0, 0, 0, 0, 0, 0, -10, 7, -16, -6, 6, -13, -11, 6, -14, -16, 8, -16, -11, 5, -12, -5, 1, -7, -5, 0, -4, -4, 1, -4},
	{0, 0, 0, 0, 0, 0, -10, 7, -16, -10, 6, -16, -11, 6, -13, -16, 8, -16, -12, 4, -10, -5, 1, -5, -3, 0, -3, -3, 0, -4},
	{0, 0, 0, 0, 0, 0, -10, 7, -16, -10, 6, -15, -12, 6, -14, -16, 8, -16, -10, 4, -8, -4, 1, -4, -3, 0, -3, -2, 0, -3},
	{0, 0, 0, 0, 0, 0, -5, 7, -16, -11, 8, -14, -13, 8, -16, -17, 7, -15, -8, 4, -8, -4, 0, -3, -3, 1, -4, -1, 0, -2},
	{0, 0, 0, 0, 0, 0, -7, 9, -18, -11, 7, -16, -13, 7, -14, -15, 6, -14, -8, 4, -8, -4, 0, -3, -3, 0, -4, -1, 0, -2},
	{0, 0, 0, 0, 0, 0, -4, 10, -20, -13, 7, -14, -13, 7, -14, -16, 6, -14, -9, 4, -8, -2, 0, -3, -3, 0, -3, -1, 0, -1},
	{0, 0, 0, 0, 0, 0, -8, 9, -19, -12, 7, -16, -16, 7, -16, -13, 6, -13, -8, 3, -7, -2, 0, -2, -2, 0, -3, -1, 0, -1},
	{0, 0, 0, 0, 0, 0, -9, 9, -20, -13, 7, -14, -14, 7, -16, -13, 5, -12, -9, 3, -8, 0, 0, -1, -3, 0, -3, -1, 0, -1},
	{0, 0, 0, 0, 0, 0, -8, 11, -23, -14, 7, -15, -14, 7, -14, -11, 5, -12, -7, 3, -6, -3, 0, -3, -1, 0, -2, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -13, 11, -24, -11, 6, -14, -13, 6, -12, -12, 6, -12, -7, 2, -6, -3, 1, -4, -2, 0, -2, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, 12, -24, -14, 7, -13, -13, 7, -14, -13, 5, -12, -6, 1, -4, -3, 1, -4, -1, 0, -2, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 11, -22, -15, 6, -16, -11, 6, -12, -13, 5, -12, -5, 2, -4, -3, 0, -4, -1, 0, -1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 11, -24, -17, 6, -17, -12, 6, -12, -11, 5, -11, -7, 2, -4, -3, 0, -4, 0, 0, -1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -12, 13, -28, -14, 6, -15, -14, 6, -14, -11, 4, -10, -6, 2, -5, -2, 0, -3, -1, 0, -1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -12, 13, -28, -16, 6, -17, -13, 6, -12, -8, 4, -9, -4, 2, -4, -2, 0, -3, 0, 0, -1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -13, 13, -27, -14, 5, -16, -13, 5, -11, -9, 4, -10, -4, 2, -4, -2, 0, -2, 0, 0, -1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -14, 14, -29, -14, 5, -18, -13, 5, -10, -9, 4, -10, -4, 1, -3, -2, 0, -3, 0, 0, -1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -16, 16, -33, -13, 5, -16, -11, 5, -10, -8, 4, -8, -4, 1, -3, -2, 0, -3, 0, 0, -1, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -17, 13, -28, -11, 4, -17, -13, 4, -10, -8, 3, -8, -4, 1, -4, -1, 0, -2, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -13, 14, -28, -14, 5, -18, -13, 5, -12, -7, 2, -6, -4, 2, -4, -1, 0, -2, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -16, 12, -26, -14, 5, -18, -10, 5, -10, -7, 2, -6, -3, 2, -4, -1, 0, -1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -16, 14, -28, -15, 4, -20, -8, 4, -8, -8, 3, -7, -3, 1, -3, 0, 0, -1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -15, 13, -28, -14, 4, -18, -11, 4, -10, -7, 2, -6, -2, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -15, 13, -27, -14, 4, -18, -10, 4, -10, -6, 2, -5, -2, 1, -3, -1, 0, -1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -13, 14, -30, -14, 4, -19, -10, 4, -9, -5, 2, -4, -3, 1, -3, 0, 0, -1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -14, 15, -31, -14, 4, -18, -8, 4, -9, -5, 2, -4, -1, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -14, 13, -28, -11, 4, -16, -8, 4, -8, -4, 2, -4, -3, 1, -2, 0, 0, -1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -13, 14, -29, -12, 4, -17, -6, 4, -9, -5, 1, -3, -1, 1, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -14, 15, -30, -13, 3, -13, -2, 3, -6, -5, 1, -3, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, -12, 14, -28, -10, 4, -14, -5, 4, -8, -4, 1, -3, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -14, 16, -33, -10, 3, -14, -4, 3, -6, -4, 1, -3, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -11, 12, -26, -8, 4, -14, -5, 4, -9, -3, 0, -2, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -13, 15, -31, -8, 3, -14, -2, 3, -6, -2, 0, -1, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -13, 16, -32, -5, 2, -13, -4, 2, -6, -3, 0, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, 14, -28, -5, 3, -12, -3, 3, -7, -1, 0, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -11, 14, -30, -5, 3, -12, -2, 3, -7, -2, 0, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -11, 13, -28, -3, 2, -12, -2, 2, -5, -3, 0, -2, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, 14, -28, -3, 2, -12, -3, 2, -6, -1, 0, 1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 14, -28, -2, 2, -10, -3, 2, -6, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, 14, -28, -2, 2, -11, -2, 2, -4, -2, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -9, 14, -30, -3, 1, -9, -2, 1, -4, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 14, -30, -5, 1, -10, -2, 1, -4, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -5, 12, -25, -4, 1, -10, -2, 1, -4, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 12, -26, -2, 1, -8, -2, 1, -3, -2, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, -8, 13, -27, -2, 1, -8, -3, 1, -3, 0, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, -5, 12, -26, -2, 1, -7, -1, 1, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, -7, 12, -26, 0, 0, -6, -2, 0, -1, -2, 1, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, -4, 11, -22, -2, 0, -6, -3, 0, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, -5, 11, -22, 0, 0, -6, -2, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, -4, 10, -22, -1, 0, -6, -2, 0, -2, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, -5, 12, -24, -2, 0, -6, 0, 0, 0, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2},
	{0, 0, 0, 0, 0, 0, -1, 9, -20, -2, 0, -6, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, 0, 10, -20, 0, 0, -4, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, 0, 10, -21, 0, 0, -4, -2, 0, -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, 3, 9, -19, 0, 0, -4, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 3, 8, -18, 3, 0, -5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

#endif /* __DYNAMIC_AID_XXXX_H */
