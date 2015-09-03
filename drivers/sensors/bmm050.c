/*!
 * @section LICENSE
 * (C) Copyright 2014 Bosch Sensortec GmbH All Rights Reserved
 *
 * This software program is licensed subject to the GNU General
 * Public License (GPL).Version 2,June 1991,
 * available at http://www.fsf.org/copyleft/gpl.html
 *
 * @filename bmm050.c
 * @date: 2013/11/25
 * @id       "fcff9b1"
 * @version: 1.2
 *
 * @brief    BMM050API
 */

#include "bmm050.h"
static struct bmm050 *p_bmm050;

BMM050_RETURN_FUNCTION_TYPE bmm050_init(struct bmm050 *bmm050)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char a_data_u8r[2] = {0, 0};
	p_bmm050 = bmm050;

	p_bmm050->dev_addr = BMM050_I2C_ADDRESS;

	/* set device from suspend into sleep mode */
	bmm050_set_powermode(BMM050_ON);

	/* wait two millisecond for bmc to settle */
	p_bmm050->delay_msec(BMM050_DELAY_SETTLING_TIME);

	/*Read CHIP_ID and REv. info */
	comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_CHIP_ID, a_data_u8r, 1);
	p_bmm050->company_id = a_data_u8r[0];

	/* Function to initialise trim values */
	bmm050_init_trim_registers();
	bmm050_set_presetmode(BMM050_PRESETMODE_REGULAR);
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_set_presetmode(unsigned char mode)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	switch (mode) {
	case BMM050_PRESETMODE_LOWPOWER:
		/* Set the data rate for Low Power mode */
		comres = bmm050_set_datarate(BMM050_LOWPOWER_DR);
		/* Set the XY-repetitions number for Low Power mode */
		comres += bmm050_set_repetitions_XY(BMM050_LOWPOWER_REPXY);
		/* Set the Z-repetitions number  for Low Power mode */
		comres += bmm050_set_repetitions_Z(BMM050_LOWPOWER_REPZ);
		break;
	case BMM050_PRESETMODE_REGULAR:
		/* Set the data rate for Regular mode */
		comres = bmm050_set_datarate(BMM050_REGULAR_DR);
		/* Set the XY-repetitions number for Regular mode */
		comres += bmm050_set_repetitions_XY(BMM050_REGULAR_REPXY);
		/* Set the Z-repetitions number  for Regular mode */
		comres += bmm050_set_repetitions_Z(BMM050_REGULAR_REPZ);
		break;
	case BMM050_PRESETMODE_HIGHACCURACY:
		/* Set the data rate for High Accuracy mode */
		comres = bmm050_set_datarate(BMM050_HIGHACCURACY_DR);
		/* Set the XY-repetitions number for High Accuracy mode */
		comres += bmm050_set_repetitions_XY(BMM050_HIGHACCURACY_REPXY);
		/* Set the Z-repetitions number  for High Accuracy mode */
		comres += bmm050_set_repetitions_Z(BMM050_HIGHACCURACY_REPZ);
		break;
	case BMM050_PRESETMODE_ENHANCED:
		/* Set the data rate for Enhanced Accuracy mode */
		comres = bmm050_set_datarate(BMM050_ENHANCED_DR);
		/* Set the XY-repetitions number for High Enhanced mode */
		comres += bmm050_set_repetitions_XY(BMM050_ENHANCED_REPXY);
		/* Set the Z-repetitions number  for High Enhanced mode */
		comres += bmm050_set_repetitions_Z(BMM050_ENHANCED_REPZ);
		break;
	default:
		comres = E_BMM050_OUT_OF_RANGE;
		break;
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_set_functional_state(
unsigned char functional_state)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data1_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return E_BMM050_NULL_PTR;
	} else {
		switch (functional_state) {
		case BMM050_NORMAL_MODE:
			comres = bmm050_get_powermode(&v_data1_u8r);
			if (v_data1_u8r == BMM050_OFF) {
				comres += bmm050_set_powermode(BMM050_ON);
				p_bmm050->delay_msec(
						BMM050_DELAY_SUSPEND_SLEEP);
			}
			{
				comres += p_bmm050->BMM050_BUS_READ_FUNC(
						p_bmm050->dev_addr,
						BMM050_CNTL_OPMODE__REG,
						&v_data1_u8r, 1);
				v_data1_u8r = BMM050_SET_BITSLICE(
						v_data1_u8r,
						BMM050_CNTL_OPMODE,
						BMM050_NORMAL_MODE);
				comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
						p_bmm050->dev_addr,
						BMM050_CNTL_OPMODE__REG,
						&v_data1_u8r, 1);
			}
			break;
		case BMM050_SUSPEND_MODE:
			comres = bmm050_set_powermode(BMM050_OFF);
			break;
		case BMM050_FORCED_MODE:
			comres = bmm050_get_powermode(&v_data1_u8r);
			if (v_data1_u8r == BMM050_OFF) {
				comres = bmm050_set_powermode(BMM050_ON);
				p_bmm050->delay_msec(
						BMM050_DELAY_SUSPEND_SLEEP);
			}
			comres += p_bmm050->BMM050_BUS_READ_FUNC(
					p_bmm050->dev_addr,
					BMM050_CNTL_OPMODE__REG,
					&v_data1_u8r, 1);
			v_data1_u8r = BMM050_SET_BITSLICE(
					v_data1_u8r,
					BMM050_CNTL_OPMODE, BMM050_ON);
			comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
					p_bmm050->dev_addr,
					BMM050_CNTL_OPMODE__REG,
					&v_data1_u8r, 1);
			break;
		case BMM050_SLEEP_MODE:
			bmm050_get_powermode(&v_data1_u8r);
			if (v_data1_u8r == BMM050_OFF) {
				comres = bmm050_set_powermode(BMM050_ON);
				p_bmm050->delay_msec(
						BMM050_DELAY_SUSPEND_SLEEP);
			}
			comres += p_bmm050->BMM050_BUS_READ_FUNC(
					p_bmm050->dev_addr,
					BMM050_CNTL_OPMODE__REG,
					&v_data1_u8r, 1);
			v_data1_u8r = BMM050_SET_BITSLICE(
					v_data1_u8r,
					BMM050_CNTL_OPMODE,
					BMM050_SLEEP_MODE);
			comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
					p_bmm050->dev_addr,
					BMM050_CNTL_OPMODE__REG,
					&v_data1_u8r, 1);
			break;
		default:
			comres = E_BMM050_OUT_OF_RANGE;
			break;
		}
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_get_functional_state(
unsigned char *functional_state)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_CNTL_OPMODE__REG,
				&v_data_u8r, 1);
		*functional_state = BMM050_GET_BITSLICE(
				v_data_u8r, BMM050_CNTL_OPMODE);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_read_mdataXYZ(struct bmm050_mdata *mdata)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;

	unsigned char a_data_u8r[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	struct {
		BMM050_S16 raw_datax;
		BMM050_S16 raw_datay;
		BMM050_S16 raw_dataz;
		BMM050_U16 raw_datar;
	} raw_dataxyz;

	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
				BMM050_DATAX_LSB, a_data_u8r, 8);

		/* Reading data for X axis */
		a_data_u8r[0] = BMM050_GET_BITSLICE(a_data_u8r[0],
				BMM050_DATAX_LSB_VALUEX);
		raw_dataxyz.raw_datax = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[1])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[0]);

		/* Reading data for Y axis */
		a_data_u8r[2] = BMM050_GET_BITSLICE(a_data_u8r[2],
				BMM050_DATAY_LSB_VALUEY);
		raw_dataxyz.raw_datay = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[3])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[2]);

		/* Reading data for Z axis */
		a_data_u8r[4] = BMM050_GET_BITSLICE(a_data_u8r[4],
				BMM050_DATAZ_LSB_VALUEZ);
		raw_dataxyz.raw_dataz = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[5])) <<
					SHIFT_LEFT_7_POSITION) | a_data_u8r[4]);

		/* Reading data for Resistance*/
		a_data_u8r[6] = BMM050_GET_BITSLICE(a_data_u8r[6],
				BMM050_R_LSB_VALUE);
		raw_dataxyz.raw_datar = (BMM050_U16)((((BMM050_U16)
						a_data_u8r[7]) <<
					SHIFT_LEFT_6_POSITION) | a_data_u8r[6]);

		/* Compensation for X axis */
		mdata->datax = bmm050_compensate_X(raw_dataxyz.raw_datax,
				raw_dataxyz.raw_datar);

		/* Compensation for Y axis */
		mdata->datay = bmm050_compensate_Y(raw_dataxyz.raw_datay,
				raw_dataxyz.raw_datar);

		/* Compensation for Z axis */
		mdata->dataz = bmm050_compensate_Z(raw_dataxyz.raw_dataz,
				raw_dataxyz.raw_datar);

	    /* Output raw resistance value */
	    mdata->resistance = raw_dataxyz.raw_datar;
	}
	return comres;
}

/* In this function X and Y axis is remapped,
 * this API is only applicable for BMX055*/
BMM050_RETURN_FUNCTION_TYPE bmm050_read_bmx055_remapped_mdataXYZ(
struct bmm050_remapped_mdata *mdata)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;

	unsigned char a_data_u8r[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	struct {
		BMM050_S16 raw_datax;
		BMM050_S16 raw_datay;
		BMM050_S16 raw_dataz;
		BMM050_U16 raw_datar;
	} raw_dataxyz;

	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_BMX055_REMAPPED_DATAY_LSB, a_data_u8r, 8);

		/* Reading data for Y axis */
		a_data_u8r[0] = BMM050_GET_BITSLICE(a_data_u8r[0],
				BMM050_BMX055_REMAPPED_DATAY_LSB_VALUEY);
		raw_dataxyz.raw_datay = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[1])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[0]);


		/* Reading data for X axis */
		a_data_u8r[2] = BMM050_GET_BITSLICE(a_data_u8r[2],
				BMM050_BMX055_REMAPPED_DATAX_LSB_VALUEX);
		raw_dataxyz.raw_datax = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[3])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[2]);
		raw_dataxyz.raw_datax = -raw_dataxyz.raw_datax;

		/* Reading data for Z axis */
		a_data_u8r[4] = BMM050_GET_BITSLICE(a_data_u8r[4],
				BMM050_DATAZ_LSB_VALUEZ);
		raw_dataxyz.raw_dataz = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[5])) <<
					SHIFT_LEFT_7_POSITION) | a_data_u8r[4]);

		/* Reading data for Resistance*/
		a_data_u8r[6] = BMM050_GET_BITSLICE(a_data_u8r[6],
				BMM050_R_LSB_VALUE);
		raw_dataxyz.raw_datar = (BMM050_U16)((((BMM050_U16)
						a_data_u8r[7]) <<
					SHIFT_LEFT_6_POSITION) | a_data_u8r[6]);

		/* Compensation for X axis */
		mdata->datax = bmm050_compensate_X(raw_dataxyz.raw_datax,
				raw_dataxyz.raw_datar);

		/* Compensation for Y axis */
		mdata->datay = bmm050_compensate_Y(raw_dataxyz.raw_datay,
				raw_dataxyz.raw_datar);

		/* Compensation for Z axis */
		mdata->dataz = bmm050_compensate_Z(raw_dataxyz.raw_dataz,
				raw_dataxyz.raw_datar);

	    /* Output raw resistance value */
	    mdata->resistance = raw_dataxyz.raw_datar;
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_read_mdataXYZ_s32(
struct bmm050_mdata_s32 *mdata)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;

	unsigned char a_data_u8r[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	struct {
		BMM050_S16 raw_datax;
		BMM050_S16 raw_datay;
		BMM050_S16 raw_dataz;
		BMM050_U16 raw_datar;
	} raw_dataxyz;

	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
				BMM050_DATAX_LSB, a_data_u8r, 8);

		/* Reading data for X axis */
		a_data_u8r[0] = BMM050_GET_BITSLICE(a_data_u8r[0],
				BMM050_DATAX_LSB_VALUEX);
		raw_dataxyz.raw_datax = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[1])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[0]);

		/* Reading data for Y axis */
		a_data_u8r[2] = BMM050_GET_BITSLICE(a_data_u8r[2],
				BMM050_DATAY_LSB_VALUEY);
		raw_dataxyz.raw_datay = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[3])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[2]);

		/* Reading data for Z axis */
		a_data_u8r[4] = BMM050_GET_BITSLICE(a_data_u8r[4],
				BMM050_DATAZ_LSB_VALUEZ);
		raw_dataxyz.raw_dataz = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[5])) <<
					SHIFT_LEFT_7_POSITION) | a_data_u8r[4]);

		/* Reading data for Resistance*/
		if (!comres)
			mdata->drdy = BMM050_GET_BITSLICE(a_data_u8r[6],
					BMM050_DATA_RDYSTAT);

		a_data_u8r[6] = BMM050_GET_BITSLICE(a_data_u8r[6],
				BMM050_R_LSB_VALUE);
		raw_dataxyz.raw_datar = (BMM050_U16)((((BMM050_U16)
						a_data_u8r[7]) <<
					SHIFT_LEFT_6_POSITION) | a_data_u8r[6]);

		/* Compensation for X axis */
		mdata->datax = bmm050_compensate_X_s32(raw_dataxyz.raw_datax,
				raw_dataxyz.raw_datar);

		/* Compensation for Y axis */
		mdata->datay = bmm050_compensate_Y_s32(raw_dataxyz.raw_datay,
				raw_dataxyz.raw_datar);

		/* Compensation for Z axis */
		mdata->dataz = bmm050_compensate_Z_s32(raw_dataxyz.raw_dataz,
				raw_dataxyz.raw_datar);

	    /* Output raw resistance value */
	    mdata->resistance = raw_dataxyz.raw_datar;
	}
	return comres;
}

/* In this function X and Y axis is remapped,
 * this API is only applicable for BMX055*/
BMM050_RETURN_FUNCTION_TYPE bmm050_read_bmx055_remapped_mdataXYZ_s32(
struct bmm050_remapped_mdata_s32 *mdata)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;

	unsigned char a_data_u8r[8] = {0, 0, 0, 0, 0, 0, 0, 0 };

	struct {
		BMM050_S16 raw_datax;
		BMM050_S16 raw_datay;
		BMM050_S16 raw_dataz;
		BMM050_U16 raw_datar;
	} raw_dataxyz;

	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_BMX055_REMAPPED_DATAY_LSB, a_data_u8r, 8);

		/* Reading data for Y axis */
		a_data_u8r[0] = BMM050_GET_BITSLICE(a_data_u8r[0],
				BMM050_BMX055_REMAPPED_DATAY_LSB_VALUEY);
		raw_dataxyz.raw_datay = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[1])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[0]);


		/* Reading data for X axis */
		a_data_u8r[2] = BMM050_GET_BITSLICE(a_data_u8r[2],
				BMM050_BMX055_REMAPPED_DATAX_LSB_VALUEX);
		raw_dataxyz.raw_datax = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[3])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[2]);
		raw_dataxyz.raw_datax = -raw_dataxyz.raw_datax;

		/* Reading data for Z axis */
		a_data_u8r[4] = BMM050_GET_BITSLICE(a_data_u8r[4],
				BMM050_DATAZ_LSB_VALUEZ);
		raw_dataxyz.raw_dataz = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[5])) <<
					SHIFT_LEFT_7_POSITION) | a_data_u8r[4]);

		/* Reading data for Resistance*/
		a_data_u8r[6] = BMM050_GET_BITSLICE(a_data_u8r[6],
				BMM050_R_LSB_VALUE);
		raw_dataxyz.raw_datar = (BMM050_U16)((((BMM050_U16)
						a_data_u8r[7]) <<
					SHIFT_LEFT_6_POSITION) | a_data_u8r[6]);

		/* Compensation for X axis */
		mdata->datax = bmm050_compensate_X_s32(raw_dataxyz.raw_datax,
				raw_dataxyz.raw_datar);

		/* Compensation for Y axis */
		mdata->datay = bmm050_compensate_Y_s32(raw_dataxyz.raw_datay,
				raw_dataxyz.raw_datar);

		/* Compensation for Z axis */
		mdata->dataz = bmm050_compensate_Z_s32(raw_dataxyz.raw_dataz,
				raw_dataxyz.raw_datar);

	    /* Output raw resistance value */
	    mdata->resistance = raw_dataxyz.raw_datar;
	}
	return comres;
}

#ifdef ENABLE_FLOAT
BMM050_RETURN_FUNCTION_TYPE bmm050_read_mdataXYZ_float(
struct bmm050_mdata_float *mdata)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;

	unsigned char a_data_u8r[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	struct {
		BMM050_S16 raw_datax;
		BMM050_S16 raw_datay;
		BMM050_S16 raw_dataz;
		BMM050_U16 raw_datar;
	} raw_dataxyz;

	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
				BMM050_DATAX_LSB, a_data_u8r, 8);

		/* Reading data for X axis */
		a_data_u8r[0] = BMM050_GET_BITSLICE(a_data_u8r[0],
				BMM050_DATAX_LSB_VALUEX);
		raw_dataxyz.raw_datax = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[1])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[0]);

		/* Reading data for Y axis */
		a_data_u8r[2] = BMM050_GET_BITSLICE(a_data_u8r[2],
				BMM050_DATAY_LSB_VALUEY);
		raw_dataxyz.raw_datay = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[3])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[2]);

		/* Reading data for Z axis */
		a_data_u8r[4] = BMM050_GET_BITSLICE(a_data_u8r[4],
				BMM050_DATAZ_LSB_VALUEZ);
		raw_dataxyz.raw_dataz = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[5])) <<
					SHIFT_LEFT_7_POSITION) | a_data_u8r[4]);

		/* Reading data for Resistance*/
		a_data_u8r[6] = BMM050_GET_BITSLICE(a_data_u8r[6],
				BMM050_R_LSB_VALUE);
		raw_dataxyz.raw_datar = (BMM050_U16)((((BMM050_U16)
						a_data_u8r[7]) <<
					SHIFT_LEFT_6_POSITION) | a_data_u8r[6]);

		/* Compensation for X axis */
		mdata->datax = bmm050_compensate_X_float(raw_dataxyz.raw_datax,
				raw_dataxyz.raw_datar);

		/* Compensation for Y axis */
		mdata->datay = bmm050_compensate_Y_float(raw_dataxyz.raw_datay,
				raw_dataxyz.raw_datar);

		/* Compensation for Z axis */
		mdata->dataz = bmm050_compensate_Z_float(raw_dataxyz.raw_dataz,
				raw_dataxyz.raw_datar);

	    /* Output raw resistance value */
	    mdata->resistance = raw_dataxyz.raw_datar;
	}
	return comres;
}
#endif

/* In this function X and Y axis is remapped,
 * this API is only applicable for BMX055*/
#ifdef ENABLE_FLOAT
BMM050_RETURN_FUNCTION_TYPE bmm050_read_bmx055_remapped_mdataXYZ_float(
struct bmm050_remapped_mdata_float *mdata)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;

	unsigned char a_data_u8r[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	struct {
		BMM050_S16 raw_datax;
		BMM050_S16 raw_datay;
		BMM050_S16 raw_dataz;
		BMM050_U16 raw_datar;
	} raw_dataxyz;

	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_BMX055_REMAPPED_DATAY_LSB, a_data_u8r, 8);

		/* Reading data for Y axis */
		a_data_u8r[0] = BMM050_GET_BITSLICE(a_data_u8r[0],
				BMM050_BMX055_REMAPPED_DATAY_LSB_VALUEY);
		raw_dataxyz.raw_datay = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[1])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[0]);

		/* Reading data for X axis */
		a_data_u8r[2] = BMM050_GET_BITSLICE(a_data_u8r[2],
				BMM050_BMX055_REMAPPED_DATAX_LSB_VALUEX);
		raw_dataxyz.raw_datax = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[3])) <<
					SHIFT_LEFT_5_POSITION) | a_data_u8r[2]);
		raw_dataxyz.raw_datax = -raw_dataxyz.raw_datax;

		/* Reading data for Z axis */
		a_data_u8r[4] = BMM050_GET_BITSLICE(a_data_u8r[4],
				BMM050_DATAZ_LSB_VALUEZ);
		raw_dataxyz.raw_dataz = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[5])) <<
					SHIFT_LEFT_7_POSITION) | a_data_u8r[4]);

		/* Reading data for Resistance*/
		a_data_u8r[6] = BMM050_GET_BITSLICE(a_data_u8r[6],
				BMM050_R_LSB_VALUE);
		raw_dataxyz.raw_datar = (BMM050_U16)((((BMM050_U16)
						a_data_u8r[7]) <<
					SHIFT_LEFT_6_POSITION) | a_data_u8r[6]);

		/* Compensation for X axis */
		mdata->datax = bmm050_compensate_X_float(raw_dataxyz.raw_datax,
				raw_dataxyz.raw_datar);

		/* Compensation for Y axis */
		mdata->datay = bmm050_compensate_Y_float(raw_dataxyz.raw_datay,
				raw_dataxyz.raw_datar);

		/* Compensation for Z axis */
		mdata->dataz = bmm050_compensate_Z_float(raw_dataxyz.raw_dataz,
				raw_dataxyz.raw_datar);

	    /* Output raw resistance value */
	    mdata->resistance = raw_dataxyz.raw_datar;
	}
	return comres;
}
#endif

BMM050_RETURN_FUNCTION_TYPE bmm050_read_register(unsigned char addr,
		unsigned char *data, unsigned char len)
{
		BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
		addr, data, len);
	}
		return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_write_register(unsigned char addr,
	    unsigned char *data, unsigned char len)
{
		BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_WRITE_FUNC(p_bmm050->dev_addr,
		addr, data, len);
	}
		return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_set_selftest(unsigned char selftest)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data1_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr, BMM050_CNTL_S_TEST__REG,
				&v_data1_u8r, 1);
		v_data1_u8r = BMM050_SET_BITSLICE(
				v_data1_u8r, BMM050_CNTL_S_TEST, selftest);
		comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
				p_bmm050->dev_addr, BMM050_CNTL_S_TEST__REG,
				&v_data1_u8r, 1);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_get_self_test_XYZ(
unsigned char *self_testxyz)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char a_data_u8r[5] = {0, 0, 0, 0, 0};
	unsigned char v_result_u8r = 0x00;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr, BMM050_DATAX_LSB_TESTX__REG,
				a_data_u8r, 5);

		v_result_u8r = BMM050_GET_BITSLICE(a_data_u8r[4],
				BMM050_DATAZ_LSB_TESTZ);

		v_result_u8r = (v_result_u8r << 1);
		v_result_u8r = (v_result_u8r | BMM050_GET_BITSLICE(
					a_data_u8r[2], BMM050_DATAY_LSB_TESTY));

		v_result_u8r = (v_result_u8r << 1);
		v_result_u8r = (v_result_u8r | BMM050_GET_BITSLICE(
					a_data_u8r[0], BMM050_DATAX_LSB_TESTX));

		*self_testxyz = v_result_u8r;
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_set_spi3(unsigned char value)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data1_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_POWER_CNTL_SPI3_EN__REG, &v_data1_u8r, 1);
		v_data1_u8r = BMM050_SET_BITSLICE(v_data1_u8r,
			BMM050_POWER_CNTL_SPI3_EN, value);
		comres += p_bmm050->BMM050_BUS_WRITE_FUNC(p_bmm050->dev_addr,
		    BMM050_POWER_CNTL_SPI3_EN__REG, &v_data1_u8r, 1);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_set_datarate(unsigned char data_rate)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data1_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_CNTL_DR__REG,
				&v_data1_u8r, 1);
		v_data1_u8r = BMM050_SET_BITSLICE(v_data1_u8r,
				BMM050_CNTL_DR, data_rate);
		comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
				p_bmm050->dev_addr,
				BMM050_CNTL_DR__REG,
				&v_data1_u8r, 1);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_get_datarate(unsigned char *data_rate)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_CNTL_DR__REG,
				&v_data_u8r, 1);
		*data_rate = BMM050_GET_BITSLICE(v_data_u8r,
				BMM050_CNTL_DR);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_perform_advanced_selftest(
BMM050_S16 *diff_z)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	BMM050_S16 result_positive = BMM050_Zero_U8X;
	BMM050_S16 result_negative  = BMM050_Zero_U8X;
	struct bmm050_mdata mdata = {0, 0, 0, 0};
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		/* set sleep mode to prepare for forced measurement.
		 * If sensor is off, this will turn it on
		 * and respect needed delays. */
		p_bmm050->delay_msec(100);
		comres = bmm050_set_functional_state(BMM050_SUSPEND_MODE);

		p_bmm050->delay_msec(20);
		comres = bmm050_set_functional_state(BMM050_SLEEP_MODE);

		p_bmm050->delay_msec(20);

		/* set normal accuracy mode */
		comres += bmm050_set_repetitions_Z(BMM050_LOWPOWER_REPZ);
		/* 14 repetitions Z in normal accuracy mode */

		/* disable X, Y channel */
		comres += bmm050_set_control_measurement_x(
				BMM050_CHANNEL_DISABLE);
		comres += bmm050_set_control_measurement_y(
				BMM050_CHANNEL_DISABLE);

		/* enable positive current and force a
		 * measurement with positive field */
		comres += bmm050_set_adv_selftest(
				BMM050_ADVANCED_SELFTEST_POSITIVE);
		comres += bmm050_set_functional_state(BMM050_FORCED_MODE);
		/* wait for measurement to complete */
		p_bmm050->delay_msec(5);

		/* read result from positive field measurement */
		comres += bmm050_read_mdataXYZ(&mdata);
		result_positive = mdata.dataz;

		/* enable negative current and force a
		 * measurement with negative field */
		comres += bmm050_set_adv_selftest(
				BMM050_ADVANCED_SELFTEST_NEGATIVE);
		comres += bmm050_set_functional_state(BMM050_FORCED_MODE);
		p_bmm050->delay_msec(5); /* wait for measurement to complete */

		/* read result from negative field measurement */
		comres += bmm050_read_mdataXYZ(&mdata);
		result_negative = mdata.dataz;

		/* turn off self test current */
		comres += bmm050_set_adv_selftest(
				BMM050_ADVANCED_SELFTEST_OFF);

		/* enable X, Y channel */
		comres += bmm050_set_control_measurement_x(
				BMM050_CHANNEL_ENABLE);
		comres += bmm050_set_control_measurement_y(
				BMM050_CHANNEL_ENABLE);

		/* write out difference in positive and negative field.
		 * This should be ~ 200 mT = 3200 LSB */
		*diff_z = (result_positive - result_negative);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_init_trim_registers(void)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char a_data_u8r[2] = {0, 0};
	comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_X1, (unsigned char *)&p_bmm050->dig_x1, 1);
	comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_Y1, (unsigned char *)&p_bmm050->dig_y1, 1);
	comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_X2, (unsigned char *)&p_bmm050->dig_x2, 1);
	comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_Y2, (unsigned char *)&p_bmm050->dig_y2, 1);
	comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_XY1, (unsigned char *)&p_bmm050->dig_xy1, 1);
	comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_XY2, (unsigned char *)&p_bmm050->dig_xy2, 1);

	/* shorts can not be recast into (unsigned char*)
	 * due to possible mix up between trim data
	 * arrangement and memory arrangement */

	comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_Z1_LSB, a_data_u8r, 2);
	p_bmm050->dig_z1 = (BMM050_U16)((((BMM050_U16)((unsigned char)
						a_data_u8r[1])) <<
				SHIFT_LEFT_8_POSITION) | a_data_u8r[0]);

	comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_Z2_LSB, a_data_u8r, 2);
	p_bmm050->dig_z2 = (BMM050_S16)((((BMM050_S16)(
						(signed char)a_data_u8r[1])) <<
				SHIFT_LEFT_8_POSITION) | a_data_u8r[0]);

	comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_Z3_LSB, a_data_u8r, 2);
	p_bmm050->dig_z3 = (BMM050_S16)((((BMM050_S16)(
						(signed char)a_data_u8r[1])) <<
				SHIFT_LEFT_8_POSITION) | a_data_u8r[0]);

	comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_Z4_LSB, a_data_u8r, 2);
	p_bmm050->dig_z4 = (BMM050_S16)((((BMM050_S16)(
						(signed char)a_data_u8r[1])) <<
				SHIFT_LEFT_8_POSITION) | a_data_u8r[0]);

	comres += p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_DIG_XYZ1_LSB, a_data_u8r, 2);
	a_data_u8r[1] = BMM050_GET_BITSLICE(a_data_u8r[1], BMM050_DIG_XYZ1_MSB);
	p_bmm050->dig_xyz1 = (BMM050_U16)((((BMM050_U16)
					((unsigned char)a_data_u8r[1])) <<
				SHIFT_LEFT_8_POSITION) | a_data_u8r[0]);
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_set_adv_selftest(
unsigned char adv_selftest)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data1_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		switch (adv_selftest) {
		case BMM050_ADVANCED_SELFTEST_OFF:
			comres = p_bmm050->BMM050_BUS_READ_FUNC(
					p_bmm050->dev_addr,
					BMM050_CNTL_ADV_ST__REG,
					&v_data1_u8r, 1);
			v_data1_u8r = BMM050_SET_BITSLICE(
					v_data1_u8r,
					BMM050_CNTL_ADV_ST,
					BMM050_ADVANCED_SELFTEST_OFF);
			comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
					p_bmm050->dev_addr,
					BMM050_CNTL_ADV_ST__REG,
					&v_data1_u8r, 1);
			break;
		case BMM050_ADVANCED_SELFTEST_POSITIVE:
			comres = p_bmm050->BMM050_BUS_READ_FUNC(
					p_bmm050->dev_addr,
					BMM050_CNTL_ADV_ST__REG,
					&v_data1_u8r, 1);
			v_data1_u8r = BMM050_SET_BITSLICE(
					v_data1_u8r,
					BMM050_CNTL_ADV_ST,
					BMM050_ADVANCED_SELFTEST_POSITIVE);
			comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
					p_bmm050->dev_addr,
					BMM050_CNTL_ADV_ST__REG,
					&v_data1_u8r, 1);
			break;
		case BMM050_ADVANCED_SELFTEST_NEGATIVE:
			comres = p_bmm050->BMM050_BUS_READ_FUNC(
					p_bmm050->dev_addr,
					BMM050_CNTL_ADV_ST__REG,
					&v_data1_u8r, 1);
			v_data1_u8r = BMM050_SET_BITSLICE(
					v_data1_u8r,
					BMM050_CNTL_ADV_ST,
					BMM050_ADVANCED_SELFTEST_NEGATIVE);
			comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
					p_bmm050->dev_addr,
					BMM050_CNTL_ADV_ST__REG,
					&v_data1_u8r, 1);
			break;
		default:
			break;
		}
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_get_adv_selftest(
unsigned char *adv_selftest)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_CNTL_ADV_ST__REG, &v_data_u8r, 1);
		*adv_selftest = BMM050_GET_BITSLICE(v_data_u8r,
			BMM050_CNTL_ADV_ST);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_get_presetmode(
unsigned char *mode)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char data_rate = BMM050_Zero_U8X;
	unsigned char repetitionsxy = BMM050_Zero_U8X;
	unsigned char repetitionsz = BMM050_Zero_U8X;

	/* Get the current data rate */
	comres = bmm050_get_datarate(&data_rate);
	/* Get the preset number of XY Repetitions */
	comres += bmm050_get_repetitions_XY(&repetitionsxy);
	/* Get the preset number of Z Repetitions */
	comres += bmm050_get_repetitions_Z(&repetitionsz);
	if ((data_rate == BMM050_LOWPOWER_DR) && (
		repetitionsxy == BMM050_LOWPOWER_REPXY) && (
		repetitionsz == BMM050_LOWPOWER_REPZ)) {
		*mode = BMM050_PRESETMODE_LOWPOWER;
	} else {
		if ((data_rate == BMM050_REGULAR_DR) && (
			repetitionsxy == BMM050_REGULAR_REPXY) && (
			repetitionsz == BMM050_REGULAR_REPZ)) {
			*mode = BMM050_PRESETMODE_REGULAR;
		} else {
			if ((data_rate == BMM050_HIGHACCURACY_DR) && (
				repetitionsxy == BMM050_HIGHACCURACY_REPXY) && (
				repetitionsz == BMM050_HIGHACCURACY_REPZ)) {
					*mode = BMM050_PRESETMODE_HIGHACCURACY;
			} else {
				if ((data_rate == BMM050_ENHANCED_DR) && (
				repetitionsxy == BMM050_ENHANCED_REPXY) && (
				repetitionsz == BMM050_ENHANCED_REPZ)) {
					*mode = BMM050_PRESETMODE_ENHANCED;
				} else {
					*mode = E_BMM050_UNDEFINED_MODE;
				}
			}
		}
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_get_powermode(unsigned char *mode)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_POWER_CNTL_PCB__REG,
				&v_data_u8r, 1);
		*mode = BMM050_GET_BITSLICE(v_data_u8r,
				BMM050_POWER_CNTL_PCB);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_set_powermode(unsigned char mode)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_POWER_CNTL_PCB__REG,
				&v_data_u8r, 1);
		v_data_u8r = BMM050_SET_BITSLICE(v_data_u8r,
				BMM050_POWER_CNTL_PCB, mode);
		comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
				p_bmm050->dev_addr,
				BMM050_POWER_CNTL_PCB__REG,
				&v_data_u8r, 1);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_get_repetitions_XY(
unsigned char *no_repetitions_xy)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_NO_REPETITIONS_XY,
				&v_data_u8r, 1);
		*no_repetitions_xy = v_data_u8r;
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_set_repetitions_XY(
unsigned char no_repetitions_xy)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		v_data_u8r = no_repetitions_xy;
		comres = p_bmm050->BMM050_BUS_WRITE_FUNC(
				p_bmm050->dev_addr,
				BMM050_NO_REPETITIONS_XY,
				&v_data_u8r, 1);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_get_repetitions_Z(
unsigned char *no_repetitions_z)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_NO_REPETITIONS_Z,
				&v_data_u8r, 1);
		*no_repetitions_z = v_data_u8r;
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_set_repetitions_Z(
unsigned char no_repetitions_z)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		v_data_u8r = no_repetitions_z;
		comres = p_bmm050->BMM050_BUS_WRITE_FUNC(p_bmm050->dev_addr,
				BMM050_NO_REPETITIONS_Z, &v_data_u8r, 1);
	}
	return comres;
}

BMM050_S16 bmm050_compensate_X(BMM050_S16 mdata_x, BMM050_U16 data_r)
{
	BMM050_S16 inter_retval = BMM050_Zero_U8X;
	if (mdata_x != BMM050_FLIP_OVERFLOW_ADCVAL  /* no overflow */
	   ) {
		inter_retval = ((BMM050_S16)(((BMM050_U16)
				((((BMM050_S32)p_bmm050->dig_xyz1) << 14)/
				 (data_r != 0 ? data_r : p_bmm050->dig_xyz1))) -
				((BMM050_U16)0x4000)));
		inter_retval = ((BMM050_S16)((((BMM050_S32)mdata_x) *
				((((((((BMM050_S32)p_bmm050->dig_xy2) *
			      ((((BMM050_S32)inter_retval) *
				((BMM050_S32)inter_retval)) >> 7)) +
			     (((BMM050_S32)inter_retval) *
			      ((BMM050_S32)(((BMM050_S16)p_bmm050->dig_xy1)
			      << 7)))) >> 9) +
			   ((BMM050_S32)0x100000)) *
			  ((BMM050_S32)(((BMM050_S16)p_bmm050->dig_x2) +
			  ((BMM050_S16)0xA0)))) >> 12)) >> 13)) +
			(((BMM050_S16)p_bmm050->dig_x1) << 3);
	} else {
		/* overflow */
		inter_retval = BMM050_OVERFLOW_OUTPUT;
	}
	return inter_retval;
}

BMM050_S32 bmm050_compensate_X_s32 (BMM050_S16 mdata_x, BMM050_U16 data_r)
{
	BMM050_S32 retval = BMM050_Zero_U8X;

	retval = bmm050_compensate_X(mdata_x, data_r);
	if (retval == (BMM050_S32)BMM050_OVERFLOW_OUTPUT)
		retval = BMM050_OVERFLOW_OUTPUT_S32;
	return retval;
}

#ifdef ENABLE_FLOAT
float bmm050_compensate_X_float (BMM050_S16 mdata_x, BMM050_U16 data_r)
{
	float inter_retval = BMM050_Zero_U8X;
	if (mdata_x != BMM050_FLIP_OVERFLOW_ADCVAL	/* no overflow */
	   ) {
		if (data_r != 0) {
			inter_retval = ((((float)p_bmm050->dig_xyz1)*16384.0f
				/data_r)-16384.0f);
		} else {
			inter_retval = 0;
		}
		inter_retval = (((mdata_x * ((((((float)p_bmm050->dig_xy2) *
			(inter_retval*inter_retval / 268435456.0f) +
			inter_retval*((float)p_bmm050->dig_xy1)/16384.0f))
			+ 256.0f) *	(((float)p_bmm050->dig_x2) + 160.0f)))
			/ 8192.0f) + (((float)p_bmm050->dig_x1) * 8.0f))/16.0f;
	} else {
		inter_retval = BMM050_OVERFLOW_OUTPUT_FLOAT;
	}
	return inter_retval;
}
#endif

BMM050_S16 bmm050_compensate_Y(BMM050_S16 mdata_y, BMM050_U16 data_r)
{
	BMM050_S16 inter_retval = BMM050_Zero_U8X;
	if (mdata_y != BMM050_FLIP_OVERFLOW_ADCVAL  /* no overflow */
	   ) {
		inter_retval = ((BMM050_S16)(((BMM050_U16)(((
			(BMM050_S32)p_bmm050->dig_xyz1) << 14)/
			(data_r != 0 ?
			 data_r : p_bmm050->dig_xyz1))) -
			((BMM050_U16)0x4000)));
		inter_retval = ((BMM050_S16)((((BMM050_S32)mdata_y) *
				((((((((BMM050_S32)
				       p_bmm050->dig_xy2) *
				      ((((BMM050_S32) inter_retval) *
					((BMM050_S32)inter_retval)) >> 7)) +
				     (((BMM050_S32)inter_retval) *
				      ((BMM050_S32)(((BMM050_S16)
				      p_bmm050->dig_xy1) << 7)))) >> 9) +
				   ((BMM050_S32)0x100000)) *
				  ((BMM050_S32)(((BMM050_S16)p_bmm050->dig_y2)
					  + ((BMM050_S16)0xA0))))
				 >> 12)) >> 13)) +
			(((BMM050_S16)p_bmm050->dig_y1) << 3);
	} else {
		/* overflow */
		inter_retval = BMM050_OVERFLOW_OUTPUT;
	}
	return inter_retval;
}

BMM050_S32 bmm050_compensate_Y_s32 (BMM050_S16 mdata_y, BMM050_U16 data_r)
{
	BMM050_S32 retval = BMM050_Zero_U8X;

	retval = bmm050_compensate_Y(mdata_y, data_r);
	if (retval == BMM050_OVERFLOW_OUTPUT)
		retval = BMM050_OVERFLOW_OUTPUT_S32;
	return retval;
}

#ifdef ENABLE_FLOAT
float bmm050_compensate_Y_float(BMM050_S16 mdata_y, BMM050_U16 data_r)
{
	float inter_retval = BMM050_Zero_U8X;
	if (mdata_y != BMM050_FLIP_OVERFLOW_ADCVAL /* no overflow */
	   ) {
		if (data_r != 0) {
			inter_retval = ((((float)p_bmm050->dig_xyz1)*16384.0f
			/data_r)-16384.0f);
		} else {
			inter_retval = 0;
		}
		inter_retval = (((mdata_y * ((((((float)p_bmm050->dig_xy2) *
			(inter_retval*inter_retval / 268435456.0f) +
			inter_retval * ((float)p_bmm050->dig_xy1)/16384.0f)) +
			256.0f) * (((float)p_bmm050->dig_y2) + 160.0f)))
			/ 8192.0f) + (((float)p_bmm050->dig_y1) * 8.0f))/16.0f;
	} else {
		/* overflow, set output to 0.0f */
		inter_retval = BMM050_OVERFLOW_OUTPUT_FLOAT;
	}
	return inter_retval;
}
#endif

BMM050_S16 bmm050_compensate_Z(BMM050_S16 mdata_z, BMM050_U16 data_r)
{
	BMM050_S32 retval = BMM050_Zero_U8X;
	if ((mdata_z != BMM050_HALL_OVERFLOW_ADCVAL)	/* no overflow */
	   ) {
		retval = (((((BMM050_S32)(mdata_z - p_bmm050->dig_z4)) << 15) -
					((((BMM050_S32)p_bmm050->dig_z3) *
					  ((BMM050_S32)(((BMM050_S16)data_r) -
						  ((BMM050_S16)
						   p_bmm050->dig_xyz1))))>>2))/
				(p_bmm050->dig_z2 +
				 ((BMM050_S16)(((((BMM050_S32)
					 p_bmm050->dig_z1) *
					 ((((BMM050_S16)data_r) << 1)))+
						 (1<<15))>>16))));
		/* saturate result to +/- 2 mT */
		if (retval > BMM050_POSITIVE_SATURATION_Z) {
			retval =  BMM050_POSITIVE_SATURATION_Z;
		} else {
			if (retval < BMM050_NEGATIVE_SATURATION_Z)
				retval = BMM050_NEGATIVE_SATURATION_Z;
		}
	} else {
		/* overflow */
		retval = BMM050_OVERFLOW_OUTPUT;
	}
	return (BMM050_S16)retval;
}

BMM050_S32 bmm050_compensate_Z_s32(BMM050_S16 mdata_z, BMM050_U16 data_r)
{
	BMM050_S32 retval = BMM050_Zero_U8X;
	if (mdata_z != BMM050_HALL_OVERFLOW_ADCVAL) {
		retval = (((((BMM050_S32)(mdata_z - p_bmm050->dig_z4)) << 15) -
		((((BMM050_S32)p_bmm050->dig_z3) *
		((BMM050_S32)(((BMM050_S16)data_r) -
		((BMM050_S16)p_bmm050->dig_xyz1))))>>2))/
		(p_bmm050->dig_z2 +
		((BMM050_S16)(((((BMM050_S32)p_bmm050->dig_z1) *
		((((BMM050_S16)data_r) << 1)))+(1<<15))>>16))));
	} else {
		retval = BMM050_OVERFLOW_OUTPUT_S32;
	}
		return retval;
}

#ifdef ENABLE_FLOAT
float bmm050_compensate_Z_float (BMM050_S16 mdata_z, BMM050_U16 data_r)
{
	float inter_retval = BMM050_Zero_U8X;
	if (mdata_z != BMM050_HALL_OVERFLOW_ADCVAL /* no overflow */
	   ) {
		inter_retval = ((((((float)mdata_z)-((float)p_bmm050->dig_z4))*
		131072.0f)-(((float)p_bmm050->dig_z3)*(((float)data_r)-
		((float)p_bmm050->dig_xyz1))))/((((float)p_bmm050->dig_z2)+
		((float)p_bmm050->dig_z1)*((float)data_r)/32768.0)*4.0))/16.0;
	} else {
		/* overflow, set output to 0.0f */
		inter_retval = BMM050_OVERFLOW_OUTPUT_FLOAT;
	}
	return inter_retval;
}
#endif

BMM050_RETURN_FUNCTION_TYPE bmm050_set_control_measurement_x(
unsigned char enable_disable)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data1_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_SENS_CNTL_CHANNELX__REG,
				&v_data1_u8r, 1);
		v_data1_u8r = BMM050_SET_BITSLICE(v_data1_u8r,
				BMM050_SENS_CNTL_CHANNELX,
				enable_disable);
		comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
				p_bmm050->dev_addr,
				BMM050_SENS_CNTL_CHANNELX__REG,
				&v_data1_u8r, 1);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_set_control_measurement_y(
unsigned char enable_disable)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data1_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_SENS_CNTL_CHANNELY__REG,
				&v_data1_u8r, 1);
		v_data1_u8r = BMM050_SET_BITSLICE(
				v_data1_u8r,
				BMM050_SENS_CNTL_CHANNELY,
				enable_disable);
		comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
				p_bmm050->dev_addr,
				BMM050_SENS_CNTL_CHANNELY__REG,
				&v_data1_u8r, 1);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_soft_reset(void)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char v_data_u8r = BMM050_Zero_U8X;
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		v_data_u8r = BMM050_ON;

		comres = p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_POWER_CNTL_SRST7__REG,
				&v_data_u8r, 1);
		v_data_u8r = BMM050_SET_BITSLICE(v_data_u8r,
				BMM050_POWER_CNTL_SRST7,
				BMM050_SOFT_RESET7_ON);
		comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
				p_bmm050->dev_addr,
				BMM050_POWER_CNTL_SRST7__REG, &v_data_u8r, 1);

		comres += p_bmm050->BMM050_BUS_READ_FUNC(
				p_bmm050->dev_addr,
				BMM050_POWER_CNTL_SRST1__REG,
				&v_data_u8r, 1);
		v_data_u8r = BMM050_SET_BITSLICE(v_data_u8r,
				BMM050_POWER_CNTL_SRST1,
				BMM050_SOFT_RESET1_ON);
		comres += p_bmm050->BMM050_BUS_WRITE_FUNC(
				p_bmm050->dev_addr,
				BMM050_POWER_CNTL_SRST1__REG,
				&v_data_u8r, 1);

		p_bmm050->delay_msec(BMM050_DELAY_SOFTRESET);
	}
	return comres;
}

BMM050_RETURN_FUNCTION_TYPE bmm050_get_raw_xyz(struct bmm050_mdata *mdata)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char a_data_u8r[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
				BMM050_DATAX_LSB, a_data_u8r, 8);

		a_data_u8r[0] = BMM050_GET_BITSLICE(a_data_u8r[0],
				BMM050_DATAX_LSB_VALUEX);
		mdata->datax = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[1]))
					<< SHIFT_LEFT_5_POSITION)
				| a_data_u8r[0]);

		a_data_u8r[2] = BMM050_GET_BITSLICE(a_data_u8r[2],
				BMM050_DATAY_LSB_VALUEY);
		mdata->datay = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[3]))
					<< SHIFT_LEFT_5_POSITION)
				| a_data_u8r[2]);

		a_data_u8r[4] = BMM050_GET_BITSLICE(a_data_u8r[4],
				BMM050_DATAZ_LSB_VALUEZ);
		mdata->dataz = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[5]))
					<< SHIFT_LEFT_7_POSITION)
				| a_data_u8r[4]);

		a_data_u8r[6] = BMM050_GET_BITSLICE(a_data_u8r[6],
				BMM050_R_LSB_VALUE);
		mdata->resistance = (BMM050_U16)((((BMM050_U16)
						a_data_u8r[7]) <<
					SHIFT_LEFT_6_POSITION) | a_data_u8r[6]);
	}
	return comres;
}

/* In this function X and Y axis is remapped,
 * this API is only applicable for BMX055*/
BMM050_RETURN_FUNCTION_TYPE bmm050_get_bmx055_remapped_raw_xyz(
struct bmm050_remapped_mdata *mdata)
{
	BMM050_RETURN_FUNCTION_TYPE comres = BMM050_Zero_U8X;
	unsigned char a_data_u8r[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	if (p_bmm050 == BMM050_NULL) {
		return  E_BMM050_NULL_PTR;
	} else {
		comres = p_bmm050->BMM050_BUS_READ_FUNC(p_bmm050->dev_addr,
			BMM050_BMX055_REMAPPED_DATAY_LSB, a_data_u8r, 8);

		a_data_u8r[0] = BMM050_GET_BITSLICE(a_data_u8r[0],
				BMM050_BMX055_REMAPPED_DATAY_LSB_VALUEY);
		mdata->datay = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[1]))
					<< SHIFT_LEFT_5_POSITION)
				| a_data_u8r[0]);


		a_data_u8r[2] = BMM050_GET_BITSLICE(a_data_u8r[2],
				BMM050_BMX055_REMAPPED_DATAX_LSB_VALUEX);
		mdata->datax = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[3]))
					<< SHIFT_LEFT_5_POSITION)
				| a_data_u8r[2]);
		mdata->datax = -mdata->datax;

		a_data_u8r[4] = BMM050_GET_BITSLICE(a_data_u8r[4],
				BMM050_DATAZ_LSB_VALUEZ);
		mdata->dataz = (BMM050_S16)((((BMM050_S16)
						((signed char)a_data_u8r[5]))
					<< SHIFT_LEFT_7_POSITION)
				| a_data_u8r[4]);

		a_data_u8r[6] = BMM050_GET_BITSLICE(a_data_u8r[6],
				BMM050_R_LSB_VALUE);
		mdata->resistance = (BMM050_U16)((((BMM050_U16)
						a_data_u8r[7]) <<
					SHIFT_LEFT_6_POSITION) | a_data_u8r[6]);
	}
	return comres;
}
