/*!
 * @section LICENSE
 * (C) Copyright 2014 Bosch Sensortec GmbH All Rights Reserved
 *
 * This software program is licensed subject to the GNU General
 * Public License (GPL).Version 2,June 1991,
 * available at http://www.fsf.org/copyleft/gpl.html
 *
 * @filename bmm050.h
 * @date: 2013/11/25
 * @id       "7bf4b97"
 * @version: 1.2
 *
 * @brief    Header of BMM050API
 */

#ifndef __BMM050_H__
#define __BMM050_H__

#define BMM050_U16 unsigned short
#define BMM050_S16 signed short
#define BMM050_S32 signed int


#define BMM050_BUS_WR_RETURN_TYPE char
#define BMM050_BUS_WR_PARAM_TYPES\
	unsigned char, unsigned char, unsigned char *, unsigned char
#define BMM050_BUS_WR_PARAM_ORDER\
	(device_addr, register_addr, register_data, wr_len)
#define BMM050_BUS_WRITE_FUNC(\
		device_addr, register_addr, register_data, wr_len)\
	bus_write(device_addr, register_addr, register_data, wr_len)

#define BMM050_BUS_RD_RETURN_TYPE char

#define BMM050_BUS_RD_PARAM_TYPES\
	unsigned char, unsigned char, unsigned char *, unsigned char

#define BMM050_BUS_RD_PARAM_ORDER (device_addr, register_addr, register_data)

#define BMM050_BUS_READ_FUNC(device_addr, register_addr, register_data, rd_len)\
	bus_read(device_addr, register_addr, register_data, rd_len)


#define BMM050_DELAY_RETURN_TYPE void

#define BMM050_DELAY_PARAM_TYPES unsigned int

#define BMM050_DELAY_FUNC(delay_in_msec)\
	delay_func(delay_in_msec)

#define BMM050_DELAY_POWEROFF_SUSPEND      1
#define BMM050_DELAY_SUSPEND_SLEEP         5
#define BMM050_DELAY_SLEEP_ACTIVE          1
#define BMM050_DELAY_ACTIVE_SLEEP          1
#define BMM050_DELAY_SLEEP_SUSPEND         1
#define BMM050_DELAY_ACTIVE_SUSPEND        1
#define BMM050_DELAY_SLEEP_POWEROFF        1
#define BMM050_DELAY_ACTIVE_POWEROFF       1
#define BMM050_DELAY_SETTLING_TIME         5


#define BMM050_RETURN_FUNCTION_TYPE        int
#define BMM050_I2C_ADDRESS                 0x10

/*General Info datas*/
#define BMM050_SOFT_RESET7_ON              1
#define BMM050_SOFT_RESET1_ON              1
#define BMM050_SOFT_RESET7_OFF             0
#define BMM050_SOFT_RESET1_OFF             0
#define BMM050_DELAY_SOFTRESET             1

/* Fixed Data Registers */
#define BMM050_CHIP_ID                     0x40

/* Data Registers*/
#define BMM050_DATAX_LSB                   0x42
#define BMM050_DATAX_MSB                   0x43
#define BMM050_DATAY_LSB                   0x44
#define BMM050_DATAY_MSB                   0x45
#define BMM050_DATAZ_LSB                   0x46
#define BMM050_DATAZ_MSB                   0x47
#define BMM050_R_LSB                       0x48
#define BMM050_R_MSB                       0x49

/* Data Registers for remapped axis(XandY)
 * this only applicable for BMX055 */
#define BMM050_BMX055_REMAPPED_DATAY_LSB      0x42
#define BMM050_BMX055_REMAPPED_DATAY_MSB      0x43
#define BMM050_BMX055_REMAPPED_DATAX_LSB      0x44
#define BMM050_BMX055_REMAPPED_DATAX_MSB      0x45

/* Status Registers */
#define BMM050_INT_STAT                    0x4A

/* Control Registers */
#define BMM050_POWER_CNTL                  0x4B
#define BMM050_CONTROL                     0x4C
#define BMM050_INT_CNTL                    0x4D
#define BMM050_SENS_CNTL                   0x4E
#define BMM050_LOW_THRES                   0x4F
#define BMM050_HIGH_THRES                  0x50
#define BMM050_NO_REPETITIONS_XY           0x51
#define BMM050_NO_REPETITIONS_Z            0x52

/* Trim Extended Registers */
#define BMM050_DIG_X1                      0x5D
#define BMM050_DIG_Y1                      0x5E
#define BMM050_DIG_Z4_LSB                  0x62
#define BMM050_DIG_Z4_MSB                  0x63
#define BMM050_DIG_X2                      0x64
#define BMM050_DIG_Y2                      0x65
#define BMM050_DIG_Z2_LSB                  0x68
#define BMM050_DIG_Z2_MSB                  0x69
#define BMM050_DIG_Z1_LSB                  0x6A
#define BMM050_DIG_Z1_MSB                  0x6B
#define BMM050_DIG_XYZ1_LSB                0x6C
#define BMM050_DIG_XYZ1_MSB                0x6D
#define BMM050_DIG_Z3_LSB                  0x6E
#define BMM050_DIG_Z3_MSB                  0x6F
#define BMM050_DIG_XY2                     0x70
#define BMM050_DIG_XY1                     0x71


/* Data X LSB Register */
#define BMM050_DATAX_LSB_VALUEX__POS        3
#define BMM050_DATAX_LSB_VALUEX__LEN        5
#define BMM050_DATAX_LSB_VALUEX__MSK        0xF8
#define BMM050_DATAX_LSB_VALUEX__REG        BMM050_DATAX_LSB

/* Data X SelfTest Register */
#define BMM050_DATAX_LSB_TESTX__POS         0
#define BMM050_DATAX_LSB_TESTX__LEN         1
#define BMM050_DATAX_LSB_TESTX__MSK         0x01
#define BMM050_DATAX_LSB_TESTX__REG         BMM050_DATAX_LSB

/* Data Y LSB Register */
#define BMM050_DATAY_LSB_VALUEY__POS        3
#define BMM050_DATAY_LSB_VALUEY__LEN        5
#define BMM050_DATAY_LSB_VALUEY__MSK        0xF8
#define BMM050_DATAY_LSB_VALUEY__REG        BMM050_DATAY_LSB

/* Data Y SelfTest Register */
#define BMM050_DATAY_LSB_TESTY__POS         0
#define BMM050_DATAY_LSB_TESTY__LEN         1
#define BMM050_DATAY_LSB_TESTY__MSK         0x01
#define BMM050_DATAY_LSB_TESTY__REG         BMM050_DATAY_LSB

/* Data Z LSB Register */
#define BMM050_DATAZ_LSB_VALUEZ__POS        1
#define BMM050_DATAZ_LSB_VALUEZ__LEN        7
#define BMM050_DATAZ_LSB_VALUEZ__MSK        0xFE
#define BMM050_DATAZ_LSB_VALUEZ__REG        BMM050_DATAZ_LSB

/* Data Z SelfTest Register */
#define BMM050_DATAZ_LSB_TESTZ__POS         0
#define BMM050_DATAZ_LSB_TESTZ__LEN         1
#define BMM050_DATAZ_LSB_TESTZ__MSK         0x01
#define BMM050_DATAZ_LSB_TESTZ__REG         BMM050_DATAZ_LSB

/* Hall Resistance LSB Register */
#define BMM050_R_LSB_VALUE__POS             2
#define BMM050_R_LSB_VALUE__LEN             6
#define BMM050_R_LSB_VALUE__MSK             0xFC
#define BMM050_R_LSB_VALUE__REG             BMM050_R_LSB

#define BMM050_DATA_RDYSTAT__POS            0
#define BMM050_DATA_RDYSTAT__LEN            1
#define BMM050_DATA_RDYSTAT__MSK            0x01
#define BMM050_DATA_RDYSTAT__REG            BMM050_R_LSB

/* Data X LSB Remapped Register only applicable for BMX055 */
#define BMM050_BMX055_REMAPPED_DATAX_LSB_VALUEX__POS        3
#define BMM050_BMX055_REMAPPED_DATAX_LSB_VALUEX__LEN        5
#define BMM050_BMX055_REMAPPED_DATAX_LSB_VALUEX__MSK        0xF8
#define BMM050_BMX055_REMAPPED_DATAX_LSB_VALUEX__REG\
					BMM050_BMX055_REMAPPED_DATAX_LSB

/* Data Y LSB Remapped Register only applicable for BMX055  */
#define BMM050_BMX055_REMAPPED_DATAY_LSB_VALUEY__POS        3
#define BMM050_BMX055_REMAPPED_DATAY_LSB_VALUEY__LEN        5
#define BMM050_BMX055_REMAPPED_DATAY_LSB_VALUEY__MSK        0xF8
#define BMM050_BMX055_REMAPPED_DATAY_LSB_VALUEY__REG\
					BMM050_BMX055_REMAPPED_DATAY_LSB

/* Interrupt Status Register */
#define BMM050_INT_STAT_DOR__POS            7
#define BMM050_INT_STAT_DOR__LEN            1
#define BMM050_INT_STAT_DOR__MSK            0x80
#define BMM050_INT_STAT_DOR__REG            BMM050_INT_STAT

#define BMM050_INT_STAT_OVRFLOW__POS        6
#define BMM050_INT_STAT_OVRFLOW__LEN        1
#define BMM050_INT_STAT_OVRFLOW__MSK        0x40
#define BMM050_INT_STAT_OVRFLOW__REG        BMM050_INT_STAT

#define BMM050_INT_STAT_HIGH_THZ__POS       5
#define BMM050_INT_STAT_HIGH_THZ__LEN       1
#define BMM050_INT_STAT_HIGH_THZ__MSK       0x20
#define BMM050_INT_STAT_HIGH_THZ__REG       BMM050_INT_STAT

#define BMM050_INT_STAT_HIGH_THY__POS       4
#define BMM050_INT_STAT_HIGH_THY__LEN       1
#define BMM050_INT_STAT_HIGH_THY__MSK       0x10
#define BMM050_INT_STAT_HIGH_THY__REG       BMM050_INT_STAT

#define BMM050_INT_STAT_HIGH_THX__POS       3
#define BMM050_INT_STAT_HIGH_THX__LEN       1
#define BMM050_INT_STAT_HIGH_THX__MSK       0x08
#define BMM050_INT_STAT_HIGH_THX__REG       BMM050_INT_STAT

#define BMM050_INT_STAT_LOW_THZ__POS        2
#define BMM050_INT_STAT_LOW_THZ__LEN        1
#define BMM050_INT_STAT_LOW_THZ__MSK        0x04
#define BMM050_INT_STAT_LOW_THZ__REG        BMM050_INT_STAT

#define BMM050_INT_STAT_LOW_THY__POS        1
#define BMM050_INT_STAT_LOW_THY__LEN        1
#define BMM050_INT_STAT_LOW_THY__MSK        0x02
#define BMM050_INT_STAT_LOW_THY__REG        BMM050_INT_STAT

#define BMM050_INT_STAT_LOW_THX__POS        0
#define BMM050_INT_STAT_LOW_THX__LEN        1
#define BMM050_INT_STAT_LOW_THX__MSK        0x01
#define BMM050_INT_STAT_LOW_THX__REG        BMM050_INT_STAT

/* Power Control Register */
#define BMM050_POWER_CNTL_SRST7__POS       7
#define BMM050_POWER_CNTL_SRST7__LEN       1
#define BMM050_POWER_CNTL_SRST7__MSK       0x80
#define BMM050_POWER_CNTL_SRST7__REG       BMM050_POWER_CNTL

#define BMM050_POWER_CNTL_SPI3_EN__POS     2
#define BMM050_POWER_CNTL_SPI3_EN__LEN     1
#define BMM050_POWER_CNTL_SPI3_EN__MSK     0x04
#define BMM050_POWER_CNTL_SPI3_EN__REG     BMM050_POWER_CNTL

#define BMM050_POWER_CNTL_SRST1__POS       1
#define BMM050_POWER_CNTL_SRST1__LEN       1
#define BMM050_POWER_CNTL_SRST1__MSK       0x02
#define BMM050_POWER_CNTL_SRST1__REG       BMM050_POWER_CNTL

#define BMM050_POWER_CNTL_PCB__POS         0
#define BMM050_POWER_CNTL_PCB__LEN         1
#define BMM050_POWER_CNTL_PCB__MSK         0x01
#define BMM050_POWER_CNTL_PCB__REG         BMM050_POWER_CNTL

/* Control Register */
#define BMM050_CNTL_ADV_ST__POS            6
#define BMM050_CNTL_ADV_ST__LEN            2
#define BMM050_CNTL_ADV_ST__MSK            0xC0
#define BMM050_CNTL_ADV_ST__REG            BMM050_CONTROL

#define BMM050_CNTL_DR__POS                3
#define BMM050_CNTL_DR__LEN                3
#define BMM050_CNTL_DR__MSK                0x38
#define BMM050_CNTL_DR__REG                BMM050_CONTROL

#define BMM050_CNTL_OPMODE__POS            1
#define BMM050_CNTL_OPMODE__LEN            2
#define BMM050_CNTL_OPMODE__MSK            0x06
#define BMM050_CNTL_OPMODE__REG            BMM050_CONTROL

#define BMM050_CNTL_S_TEST__POS            0
#define BMM050_CNTL_S_TEST__LEN            1
#define BMM050_CNTL_S_TEST__MSK            0x01
#define BMM050_CNTL_S_TEST__REG            BMM050_CONTROL

/* Interrupt Control Register */
#define BMM050_INT_CNTL_DOR_EN__POS            7
#define BMM050_INT_CNTL_DOR_EN__LEN            1
#define BMM050_INT_CNTL_DOR_EN__MSK            0x80
#define BMM050_INT_CNTL_DOR_EN__REG            BMM050_INT_CNTL

#define BMM050_INT_CNTL_OVRFLOW_EN__POS        6
#define BMM050_INT_CNTL_OVRFLOW_EN__LEN        1
#define BMM050_INT_CNTL_OVRFLOW_EN__MSK        0x40
#define BMM050_INT_CNTL_OVRFLOW_EN__REG        BMM050_INT_CNTL

#define BMM050_INT_CNTL_HIGH_THZ_EN__POS       5
#define BMM050_INT_CNTL_HIGH_THZ_EN__LEN       1
#define BMM050_INT_CNTL_HIGH_THZ_EN__MSK       0x20
#define BMM050_INT_CNTL_HIGH_THZ_EN__REG       BMM050_INT_CNTL

#define BMM050_INT_CNTL_HIGH_THY_EN__POS       4
#define BMM050_INT_CNTL_HIGH_THY_EN__LEN       1
#define BMM050_INT_CNTL_HIGH_THY_EN__MSK       0x10
#define BMM050_INT_CNTL_HIGH_THY_EN__REG       BMM050_INT_CNTL

#define BMM050_INT_CNTL_HIGH_THX_EN__POS       3
#define BMM050_INT_CNTL_HIGH_THX_EN__LEN       1
#define BMM050_INT_CNTL_HIGH_THX_EN__MSK       0x08
#define BMM050_INT_CNTL_HIGH_THX_EN__REG       BMM050_INT_CNTL

#define BMM050_INT_CNTL_LOW_THZ_EN__POS        2
#define BMM050_INT_CNTL_LOW_THZ_EN__LEN        1
#define BMM050_INT_CNTL_LOW_THZ_EN__MSK        0x04
#define BMM050_INT_CNTL_LOW_THZ_EN__REG        BMM050_INT_CNTL

#define BMM050_INT_CNTL_LOW_THY_EN__POS        1
#define BMM050_INT_CNTL_LOW_THY_EN__LEN        1
#define BMM050_INT_CNTL_LOW_THY_EN__MSK        0x02
#define BMM050_INT_CNTL_LOW_THY_EN__REG        BMM050_INT_CNTL

#define BMM050_INT_CNTL_LOW_THX_EN__POS        0
#define BMM050_INT_CNTL_LOW_THX_EN__LEN        1
#define BMM050_INT_CNTL_LOW_THX_EN__MSK        0x01
#define BMM050_INT_CNTL_LOW_THX_EN__REG        BMM050_INT_CNTL

/* Sensor Control Register */
#define BMM050_SENS_CNTL_DRDY_EN__POS          7
#define BMM050_SENS_CNTL_DRDY_EN__LEN          1
#define BMM050_SENS_CNTL_DRDY_EN__MSK          0x80
#define BMM050_SENS_CNTL_DRDY_EN__REG          BMM050_SENS_CNTL

#define BMM050_SENS_CNTL_IE__POS               6
#define BMM050_SENS_CNTL_IE__LEN               1
#define BMM050_SENS_CNTL_IE__MSK               0x40
#define BMM050_SENS_CNTL_IE__REG               BMM050_SENS_CNTL

#define BMM050_SENS_CNTL_CHANNELZ__POS         5
#define BMM050_SENS_CNTL_CHANNELZ__LEN         1
#define BMM050_SENS_CNTL_CHANNELZ__MSK         0x20
#define BMM050_SENS_CNTL_CHANNELZ__REG         BMM050_SENS_CNTL

#define BMM050_SENS_CNTL_CHANNELY__POS         4
#define BMM050_SENS_CNTL_CHANNELY__LEN         1
#define BMM050_SENS_CNTL_CHANNELY__MSK         0x10
#define BMM050_SENS_CNTL_CHANNELY__REG         BMM050_SENS_CNTL

#define BMM050_SENS_CNTL_CHANNELX__POS         3
#define BMM050_SENS_CNTL_CHANNELX__LEN         1
#define BMM050_SENS_CNTL_CHANNELX__MSK         0x08
#define BMM050_SENS_CNTL_CHANNELX__REG         BMM050_SENS_CNTL

#define BMM050_SENS_CNTL_DR_POLARITY__POS      2
#define BMM050_SENS_CNTL_DR_POLARITY__LEN      1
#define BMM050_SENS_CNTL_DR_POLARITY__MSK      0x04
#define BMM050_SENS_CNTL_DR_POLARITY__REG      BMM050_SENS_CNTL

#define BMM050_SENS_CNTL_INTERRUPT_LATCH__POS            1
#define BMM050_SENS_CNTL_INTERRUPT_LATCH__LEN            1
#define BMM050_SENS_CNTL_INTERRUPT_LATCH__MSK            0x02
#define BMM050_SENS_CNTL_INTERRUPT_LATCH__REG            BMM050_SENS_CNTL

#define BMM050_SENS_CNTL_INTERRUPT_POLARITY__POS         0
#define BMM050_SENS_CNTL_INTERRUPT_POLARITY__LEN         1
#define BMM050_SENS_CNTL_INTERRUPT_POLARITY__MSK         0x01
#define BMM050_SENS_CNTL_INTERRUPT_POLARITY__REG         BMM050_SENS_CNTL

/* Register 6D */
#define BMM050_DIG_XYZ1_MSB__POS         0
#define BMM050_DIG_XYZ1_MSB__LEN         7
#define BMM050_DIG_XYZ1_MSB__MSK         0x7F
#define BMM050_DIG_XYZ1_MSB__REG         BMM050_DIG_XYZ1_MSB


#define BMM050_X_AXIS               0
#define BMM050_Y_AXIS               1
#define BMM050_Z_AXIS               2
#define BMM050_RESISTANCE           3
#define BMM050_X                    1
#define BMM050_Y                    2
#define BMM050_Z                    4
#define BMM050_XYZ                  7

/* Constants */
#define BMM050_Zero_U8X                         0
#define BMM050_NULL                             0
#define BMM050_DISABLE                          0
#define BMM050_ENABLE                           1
#define BMM050_CHANNEL_DISABLE                  1
#define BMM050_CHANNEL_ENABLE                   0
#define BMM050_INTPIN_LATCH_ENABLE              1
#define BMM050_INTPIN_LATCH_DISABLE             0
#define BMM050_OFF                              0
#define BMM050_ON                               1

#define BMM050_NORMAL_MODE                      0x00
#define BMM050_FORCED_MODE                      0x01
#define BMM050_SUSPEND_MODE                     0x02
#define BMM050_SLEEP_MODE                       0x03

#define BMM050_ADVANCED_SELFTEST_OFF            0
#define BMM050_ADVANCED_SELFTEST_NEGATIVE       2
#define BMM050_ADVANCED_SELFTEST_POSITIVE       3

#define BMM050_NEGATIVE_SATURATION_Z            -32767
#define BMM050_POSITIVE_SATURATION_Z            32767

#define BMM050_SPI_RD_MASK                      0x80
#define BMM050_READ_SET                         0x01

#define E_BMM050_NULL_PTR                       ((signed char)-127)
#define E_BMM050_COMM_RES                       ((signed char)-1)
#define E_BMM050_OUT_OF_RANGE                   ((signed char)-2)
#define E_BMM050_UNDEFINED_MODE                 0

#define BMM050_WR_FUNC_PTR\
	int (*bus_write)(unsigned char, unsigned char,\
			unsigned char *, unsigned char)

#define BMM050_RD_FUNC_PTR\
	int (*bus_read)(unsigned char, unsigned char,\
			unsigned char *, unsigned char)
#define BMM050_MDELAY_DATA_TYPE unsigned int

/*Shifting Constants*/
#define SHIFT_RIGHT_1_POSITION                  1
#define SHIFT_RIGHT_2_POSITION                  2
#define SHIFT_RIGHT_3_POSITION                  3
#define SHIFT_RIGHT_4_POSITION                  4
#define SHIFT_RIGHT_5_POSITION                  5
#define SHIFT_RIGHT_6_POSITION                  6
#define SHIFT_RIGHT_7_POSITION                  7
#define SHIFT_RIGHT_8_POSITION                  8

#define SHIFT_LEFT_1_POSITION                   1
#define SHIFT_LEFT_2_POSITION                   2
#define SHIFT_LEFT_3_POSITION                   3
#define SHIFT_LEFT_4_POSITION                   4
#define SHIFT_LEFT_5_POSITION                   5
#define SHIFT_LEFT_6_POSITION                   6
#define SHIFT_LEFT_7_POSITION                   7
#define SHIFT_LEFT_8_POSITION                   8

/* Conversion factors*/
#define BMM050_CONVFACTOR_LSB_UT                6

/* get bit slice  */
#define BMM050_GET_BITSLICE(regvar, bitname)\
	((regvar & bitname##__MSK) >> bitname##__POS)

/* Set bit slice */
#define BMM050_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

/* compensated output value returned if sensor had overflow */
#define BMM050_OVERFLOW_OUTPUT       -32768
#define BMM050_OVERFLOW_OUTPUT_S32   ((BMM050_S32)(-2147483647-1))
#define BMM050_OVERFLOW_OUTPUT_FLOAT 0.0f
#define BMM050_FLIP_OVERFLOW_ADCVAL  -4096
#define BMM050_HALL_OVERFLOW_ADCVAL  -16384
#define BMM050_OVERFLOW_OUTPUT_S32_XY   ((BMM050_S32)(-20800))
#define BMM050_OVERFLOW_OUTPUT_S32_Z   ((BMM050_S32)(-40000))
#define BMM050_ADC_SELFTEST_RANGE_XY	10400
#define BMM050_ADC_SELFTEST_RANGE_Z		20000


#define BMM050_PRESETMODE_LOWPOWER                  1
#define BMM050_PRESETMODE_REGULAR                   2
#define BMM050_PRESETMODE_HIGHACCURACY              3
#define BMM050_PRESETMODE_ENHANCED                  4

/* PRESET MODES - DATA RATES */
#define BMM050_LOWPOWER_DR                       BMM050_DR_10HZ
#define BMM050_REGULAR_DR                        BMM050_DR_10HZ
#define BMM050_HIGHACCURACY_DR                   BMM050_DR_20HZ
#define BMM050_ENHANCED_DR                       BMM050_DR_10HZ

/* PRESET MODES - REPETITIONS-XY RATES */
#define BMM050_LOWPOWER_REPXY                     1
#define BMM050_REGULAR_REPXY                      4
#define BMM050_HIGHACCURACY_REPXY                23
#define BMM050_ENHANCED_REPXY                     7

/* PRESET MODES - REPETITIONS-Z RATES */
#define BMM050_LOWPOWER_REPZ                      2
#define BMM050_REGULAR_REPZ                      15
#define BMM050_HIGHACCURACY_REPZ                 82
#define BMM050_ENHANCED_REPZ                     26

/* Data Rates */

#define BMM050_DR_10HZ                     0
#define BMM050_DR_02HZ                     1
#define BMM050_DR_06HZ                     2
#define BMM050_DR_08HZ                     3
#define BMM050_DR_15HZ                     4
#define BMM050_DR_20HZ                     5
#define BMM050_DR_25HZ                     6
#define BMM050_DR_30HZ                     7

/*user defined Structures*/
struct bmm050_mdata {
	BMM050_S16 datax;
	BMM050_S16 datay;
	BMM050_S16 dataz;
	BMM050_U16 resistance;
};
struct bmm050_mdata_s32 {
	BMM050_S32 datax;
	BMM050_S32 datay;
	BMM050_S32 dataz;
	BMM050_U16 resistance;
	BMM050_U16 drdy;
};
struct bmm050_mdata_float {
	float datax;
	float datay;
	float  dataz;
	BMM050_U16 resistance;
};

/*user defined Structures for remapped functions
 * this only applicable for BMX055*/
struct bmm050_remapped_mdata {
	BMM050_S16 datax;
	BMM050_S16 datay;
	BMM050_S16 dataz;
	BMM050_U16 resistance;
};
struct bmm050_remapped_mdata_s32 {
	BMM050_S32 datax;
	BMM050_S32 datay;
	BMM050_S32 dataz;
	BMM050_U16 resistance;
};
struct bmm050_remapped_mdata_float {
	float datax;
	float datay;
	float  dataz;
	BMM050_U16 resistance;
};
struct bmm050 {
	unsigned char company_id;
	unsigned char dev_addr;

	BMM050_WR_FUNC_PTR;
	BMM050_RD_FUNC_PTR;
	void(*delay_msec)(BMM050_MDELAY_DATA_TYPE);

	signed char dig_x1;
	signed char dig_y1;

	signed char dig_x2;
	signed char dig_y2;

	BMM050_U16 dig_z1;
	BMM050_S16 dig_z2;
	BMM050_S16 dig_z3;
	BMM050_S16 dig_z4;

	unsigned char dig_xy1;
	signed char dig_xy2;

	BMM050_U16 dig_xyz1;
};


BMM050_RETURN_FUNCTION_TYPE bmm050_init(struct bmm050 *p_bmm050);
BMM050_RETURN_FUNCTION_TYPE bmm050_read_mdataXYZ(
		struct bmm050_mdata *mdata);
BMM050_RETURN_FUNCTION_TYPE bmm050_read_mdataXYZ_s32(
		struct bmm050_mdata_s32 *mdata);
#ifdef ENABLE_FLOAT
BMM050_RETURN_FUNCTION_TYPE bmm050_read_mdataXYZ_float(
		struct bmm050_mdata_float *mdata);
#endif
BMM050_RETURN_FUNCTION_TYPE bmm050_read_register(
		unsigned char addr, unsigned char *data, unsigned char len);
BMM050_RETURN_FUNCTION_TYPE bmm050_write_register(
		unsigned char addr, unsigned char *data, unsigned char len);
BMM050_RETURN_FUNCTION_TYPE bmm050_get_self_test_XYZ(
		unsigned char *self_testxyz);
BMM050_S16 bmm050_compensate_X(
		BMM050_S16 mdata_x, BMM050_U16 data_r);
BMM050_S32 bmm050_compensate_X_s32(
		BMM050_S16 mdata_x,  BMM050_U16 data_r);
#ifdef ENABLE_FLOAT
float bmm050_compensate_X_float(
		BMM050_S16 mdata_x,  BMM050_U16 data_r);
#endif
BMM050_S16 bmm050_compensate_Y(
		BMM050_S16 mdata_y, BMM050_U16 data_r);
BMM050_S32 bmm050_compensate_Y_s32(
		BMM050_S16 mdata_y,  BMM050_U16 data_r);
#ifdef ENABLE_FLOAT
float bmm050_compensate_Y_float(
		BMM050_S16 mdata_y,  BMM050_U16 data_r);
#endif
BMM050_S16 bmm050_compensate_Z(
		BMM050_S16 mdata_z,  BMM050_U16 data_r);
BMM050_S32 bmm050_compensate_Z_s32(
		BMM050_S16 mdata_z,  BMM050_U16 data_r);
#ifdef ENABLE_FLOAT
float bmm050_compensate_Z_float(
		BMM050_S16 mdata_z,  BMM050_U16 data_r);
#endif
BMM050_RETURN_FUNCTION_TYPE bmm050_get_raw_xyz(
struct bmm050_mdata *mdata);

BMM050_RETURN_FUNCTION_TYPE bmm050_init_trim_registers(void);

BMM050_RETURN_FUNCTION_TYPE bmm050_set_spi3(
unsigned char value);

BMM050_RETURN_FUNCTION_TYPE bmm050_get_powermode(
unsigned char *mode);
BMM050_RETURN_FUNCTION_TYPE bmm050_set_powermode(
unsigned char mode);
BMM050_RETURN_FUNCTION_TYPE bmm050_set_adv_selftest(
unsigned char adv_selftest);
BMM050_RETURN_FUNCTION_TYPE bmm050_get_adv_selftest(
unsigned char *adv_selftest);
BMM050_RETURN_FUNCTION_TYPE bmm050_set_datarate(
unsigned char data_rate);
BMM050_RETURN_FUNCTION_TYPE bmm050_get_datarate(
unsigned char *data_rate);
BMM050_RETURN_FUNCTION_TYPE bmm050_set_functional_state(
unsigned char functional_state);
BMM050_RETURN_FUNCTION_TYPE bmm050_get_functional_state(
unsigned char *functional_state);
BMM050_RETURN_FUNCTION_TYPE bmm050_set_selftest(
unsigned char selftest);
BMM050_RETURN_FUNCTION_TYPE bmm050_get_selftest(
		unsigned char *selftest);
BMM050_RETURN_FUNCTION_TYPE bmm050_perform_advanced_selftest(
		BMM050_S16 *diff_z);
BMM050_RETURN_FUNCTION_TYPE bmm050_get_repetitions_XY(
		unsigned char *no_repetitions_xy);
BMM050_RETURN_FUNCTION_TYPE bmm050_set_repetitions_XY(
		unsigned char no_repetitions_xy);
BMM050_RETURN_FUNCTION_TYPE bmm050_get_repetitions_Z(
		unsigned char *no_repetitions_z);
BMM050_RETURN_FUNCTION_TYPE bmm050_set_repetitions_Z(
		unsigned char no_repetitions_z);
BMM050_RETURN_FUNCTION_TYPE bmm050_get_presetmode(unsigned char *mode);
BMM050_RETURN_FUNCTION_TYPE bmm050_set_presetmode(unsigned char mode);
BMM050_RETURN_FUNCTION_TYPE bmm050_set_control_measurement_x(
		unsigned char enable_disable);
BMM050_RETURN_FUNCTION_TYPE bmm050_set_control_measurement_y(
		unsigned char enable_disable);
BMM050_RETURN_FUNCTION_TYPE bmm050_soft_reset(void);
BMM050_RETURN_FUNCTION_TYPE bmm050_get_bmx055_remapped_raw_xyz
(struct bmm050_remapped_mdata *mdata);
BMM050_RETURN_FUNCTION_TYPE bmm050_read_bmx055_remapped_mdataXYZ_float
(struct bmm050_remapped_mdata_float *mdata);
BMM050_RETURN_FUNCTION_TYPE bmm050_read_bmx055_remapped_mdataXYZ_s32
(struct bmm050_remapped_mdata_s32 *mdata);
BMM050_RETURN_FUNCTION_TYPE bmm050_read_bmx055_remapped_mdataXYZ
(struct bmm050_remapped_mdata *mdata);
#endif
