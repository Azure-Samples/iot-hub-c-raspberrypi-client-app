/*
* IoT Hub Raspberry Pi C - Microsoft Sample Code - Copyright (c) 2017 - Licensed MIT
*/

///////////////////////////////////////////////////////////////////////////////
//
// bme280.c:
// SPI based interface to read temperature, pressure and humidity samples from
// a BME280 module.
//
///////////////////////////////////////////////////////////////////////////////

#include "./bme280.h"
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define SENSOR_MODULE_MAX_XFER_LEN (128)
static int Num_allowed_retries__i = 3;
static int Chip_enable_selected__i = -1;

// #define SHOW_DEBUG_OUTPUT


///////////////////////////////////////////////////////////////////////////////
// Device registers
enum
{
    eBME280reg_DIG_T1   = 0x88
  , eBME280reg_DIG_T2   = 0x8A
  , eBME280reg_DIG_T3   = 0x8C

  , eBME280reg_DIG_P1   = 0x8E
  , eBME280reg_DIG_P2   = 0x90
  , eBME280reg_DIG_P3   = 0x92
  , eBME280reg_DIG_P4   = 0x94
  , eBME280reg_DIG_P5   = 0x96
  , eBME280reg_DIG_P6   = 0x98
  , eBME280reg_DIG_P7   = 0x9A
  , eBME280reg_DIG_P8   = 0x9C
  , eBME280reg_DIG_P9   = 0x9E

  , eBME280reg_DIG_H1   = 0xA1
  , eBME280reg_DIG_H2   = 0xE1
  , eBME280reg_DIG_H3   = 0xE3
  , eBME280reg_DIG_H4   = 0xE4
  , eBME280reg_DIG_H5   = 0xE5
  , eBME280reg_DIG_H6   = 0xE7

  , eBME280reg_CHIPID   = 0xD0
  , eBME280reg_VERSION  = 0xD1
  , eBME280reg_SWRESET  = 0xE0

  , eBME280reg_STATUS   = 0xF3
  , eBME280reg_CONTROL  = 0xF4
  , eBME280reg_CONFIG   = 0xF5
  , eBME280reg_PRESDATA = 0xF7
  , eBME280reg_TEMPDATA = 0xFA
};


// Calibration data as read from the device.
typedef struct
{
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;

  uint16_t dig_P1;
  int16_t  dig_P2;
  int16_t  dig_P3;
  int16_t  dig_P4;
  int16_t  dig_P5;
  int16_t  dig_P6;
  int16_t  dig_P7;
  int16_t  dig_P8;
  int16_t  dig_P9;

  uint8_t  dig_H1;
  int16_t  dig_H2;
  uint16_t dig_H3;
  int16_t  dig_H4;
  int16_t  dig_H5;
  int8_t   dig_H6;
} bme280_calib_data_t;
bme280_calib_data_t Calib_data;


///////////////////////////////////////////////////////////////////////////////
int bme280_read(const uint8_t Register__u8, uint8_t * Data__u8p, uint8_t Num_bytes__u8)
{
  if (Chip_enable_selected__i == -1) { return 0; }
  if (Num_bytes__u8 >= SENSOR_MODULE_MAX_XFER_LEN) { return 0; }

  uint8_t Buffer__u8a[SENSOR_MODULE_MAX_XFER_LEN];
  memset(Buffer__u8a, 0, SENSOR_MODULE_MAX_XFER_LEN);

  // Set bit 7 high to tell it to read.
  Buffer__u8a[0] = (0x80 | Register__u8);
  int Result__i =
    wiringPiSPIDataRW(Chip_enable_selected__i, Buffer__u8a, Num_bytes__u8 + 1);
  int Out_idx__i = 0;
  while (Out_idx__i < (Result__i - 1))
  {
    Data__u8p[Out_idx__i] = Buffer__u8a[Out_idx__i + 1];
    Out_idx__i++;
  }

  return Result__i - 1;
}

///////////////////////////////////////////////////////////////////////////////
int bme280_write(const uint8_t Register__u8, const uint8_t * Data__u8p, uint8_t Num_bytes__u8)
{
  if (Chip_enable_selected__i == -1) { return 0; }
  if (Num_bytes__u8 > SENSOR_MODULE_MAX_XFER_LEN) { return 0; }

  uint8_t Buffer__u8a[SENSOR_MODULE_MAX_XFER_LEN];

  uint8_t Write_idx__u8 = 0;
  while (Write_idx__u8 < Num_bytes__u8)
  {
    // Set bit 7 low to tell it to write.
    Buffer__u8a[Write_idx__u8 * 2] = (0x7F & (Register__u8 + Write_idx__u8));
    Buffer__u8a[Write_idx__u8 * 2 + 1] = *Data__u8p;

    Write_idx__u8++;
    Data__u8p++;
  }

  int Result__i = wiringPiSPIDataRW(Chip_enable_selected__i,
    Buffer__u8a, Num_bytes__u8 * 2);

  return Result__i / 2;
}

///////////////////////////////////////////////////////////////////////////////
int bme280_init(int Chip_enable_to_use__i)
{
  #ifdef SHOW_DEBUG_OUTPUT
  printf("bme280_init(%i)\n", Chip_enable_to_use__i);
  #endif

  if ((Chip_enable_to_use__i < 0) || (Chip_enable_to_use__i > 1))
  {
    return 0;
  }
  Chip_enable_selected__i = Chip_enable_to_use__i;

  // Verify that the chip is really a BME280.
  uint8_t ID_value__u8 = 0;
  int Bytes_read__i = bme280_read(eBME280reg_CHIPID, &ID_value__u8, 1);
  if (Bytes_read__i != 1)
  {
    return 0;
  }
  #ifdef SHOW_DEBUG_OUTPUT
  printf("Read 0x%02x from register 0x%02x\n", ID_value__u8, eBME280reg_CHIPID);
  #endif

  if (ID_value__u8 != 0x60)
  {
    #ifdef SHOW_DEBUG_OUTPUT
    printf("This is not a BME280. Expecting an ID register value of 0x%02x\n",
      0x60);
    #endif
    return 0;
  }

  #define T_P_CALIB_NUM_BYTES (24)
  Bytes_read__i = bme280_read(eBME280reg_DIG_T1, (uint8_t *)&Calib_data,
    T_P_CALIB_NUM_BYTES);
  if (Bytes_read__i != T_P_CALIB_NUM_BYTES)
  {
    #ifdef SHOW_DEBUG_OUTPUT
    printf("Err: Only read %i out of %i calibration data bytes.\n",
      Bytes_read__i, T_P_CALIB_NUM_BYTES);
    #endif
    return 0;
  }
  uint8_t Hum_calib_buf__u8a[9];
  Bytes_read__i += bme280_read(eBME280reg_DIG_H1, &Hum_calib_buf__u8a[0], 1);
  if (Bytes_read__i != T_P_CALIB_NUM_BYTES + 1)
  {
    #ifdef SHOW_DEBUG_OUTPUT
    printf("Err: Only read %i out of %i calibration data bytes.\n",
      Bytes_read__i, T_P_CALIB_NUM_BYTES + 1);
    #endif
    return 0;
  }
  Bytes_read__i += bme280_read(eBME280reg_DIG_H2, &Hum_calib_buf__u8a[1], 7);
  if (Bytes_read__i != T_P_CALIB_NUM_BYTES + 8)
  {
    #ifdef SHOW_DEBUG_OUTPUT
    printf("Err: Only read %i out of %i calibration data bytes.\n",
      Bytes_read__i, T_P_CALIB_NUM_BYTES + 8);
    #endif
    return 0;
  }
  #ifdef SHOW_DEBUG_OUTPUT
  printf("Read %i calibration data bytes starting at 0x%02x.\n",
    Bytes_read__i, eBME280reg_DIG_T1);
  #endif

  // Decode the humidity compensation constants.
  Calib_data.dig_H1 = Hum_calib_buf__u8a[0];
  Calib_data.dig_H2 = (int16_t)(((uint16_t)Hum_calib_buf__u8a[1])
    + (((uint16_t)Hum_calib_buf__u8a[2]) << 8));
  Calib_data.dig_H3 = Hum_calib_buf__u8a[3];
  Calib_data.dig_H4 = (int16_t)((((uint16_t)Hum_calib_buf__u8a[4]) << 4)
    + (((uint16_t)Hum_calib_buf__u8a[5]) & 0x0F));
  Calib_data.dig_H5 = (int16_t)((((uint16_t)Hum_calib_buf__u8a[5]) >> 4)
    + (((uint16_t)Hum_calib_buf__u8a[6]) << 4));
  Calib_data.dig_H6 = (int8_t)Hum_calib_buf__u8a[7];

  // bits 7~5 = 001 = temperature oversampling * 1
  // bits 4~2 = 111 = pressure oversampling * 16
  // bits 1~0 = 11  = normal power mode
  const uint8_t Control_setting__u8 = 0x3F;
  uint8_t Bytes_written__u8 = bme280_write(eBME280reg_CONTROL,
    &Control_setting__u8, 1);
  if (Bytes_written__u8 != 1)
  {
    #ifdef SHOW_DEBUG_OUTPUT
    printf("Err: Could not write 0x%02x to register 0x%02x.\n",
      Control_setting__u8, eBME280reg_CONTROL);
    #endif
    return 0;
  }
  #ifdef SHOW_DEBUG_OUTPUT
  printf("Wrote 0x%02x to configuration register 0x%02x.\n",
    Control_setting__u8, eBME280reg_CONTROL);
  #endif

  return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Returns temperature in DegC, resolution is 0.01 DegC.
// For example: Output value of “5123” equals 51.23 DegC.
// t_fine is stored globally since it is also used by the pressure comp calc.
// Note: Must call this before calling compensate_P or compensate_H because of
// the global t_fine variable.
int32_t t_fine = 0;
int32_t bme280_compensate_T_int32(int32_t adc_T)
{
  int32_t var1, var2, T;
  var1 = ((((adc_T >> 3) - ((int32_t)Calib_data.dig_T1 << 1)))
    * ((int32_t)Calib_data.dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)Calib_data.dig_T1))
    * ((adc_T >> 4) - ((int32_t)Calib_data.dig_T1))) >> 12)
    * ((int32_t)Calib_data.dig_T3)) >> 14;
  t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
  return T;
}

///////////////////////////////////////////////////////////////////////////////
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24
// integer bits and 8 fractional bits).
// For example: Output value of “24674867” represents 24674867/256 = 96386.2 Pa
// = 963.862 hPa
// Note: Must call compensate_T before calling this because of
// the global t_fine variable.
uint32_t bme280_compensate_P_int64(int32_t adc_P)
{
  int64_t var1, var2, p;
  var1 = ((int64_t)t_fine) - 128000LL;
  var2 = var1 * var1 * (int64_t)Calib_data.dig_P6;
  var2 = var2 + ((var1*(int64_t)Calib_data.dig_P5) << 17);
  var2 = var2 + (((int64_t)Calib_data.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)Calib_data.dig_P3)>>8) + ((var1 * (int64_t)Calib_data.dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)Calib_data.dig_P1) >> 33;
  if (var1 == 0)
  {
    // Avoid divide by zero exception.
    return 0;
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)Calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)Calib_data.dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)Calib_data.dig_P7) << 4);
  return (uint32_t)p;
}

///////////////////////////////////////////////////////////////////////////////
// Returns humidity as a relative percentage.
// Encoded as Q22.10 format (22 integer bits and 10 fractional bits).
// For example: Output value of “47445” represents 47445/1024 = 46.333 %RH
// Note: Must call compensate_T before calling this because of
// the global t_fine variable.
uint32_t bme280_compensate_H_int32(int32_t adc_H)
{
  int32_t v_x1_u32r;
  v_x1_u32r = (t_fine - ((int32_t)76800L));
  v_x1_u32r = (((((adc_H << 14) - (((int32_t)Calib_data.dig_H4) << 20)
    - (((int32_t)Calib_data.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15)
    * (((((((v_x1_u32r * ((int32_t)Calib_data.dig_H6)) >> 10)
    * (((v_x1_u32r * ((int32_t)Calib_data.dig_H3)) >> 11)
    + ((int32_t)32768))) >> 10) + ((int32_t)2097152))
    * ((int32_t)Calib_data.dig_H2) + 8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7)
    * ((int32_t)Calib_data.dig_H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  return (uint32_t)(v_x1_u32r >> 12);
}

///////////////////////////////////////////////////////////////////////////////
int bme280_read_sensors(float * Temp_c__fp, float * Pres_Pa__fp,
  float * Hum_pct__fp)
{
  int Return_status__i = 0;

  // Make sure the sensor isn't busy updating values.
  uint8_t Status__u8 = 0x01;
  while ((Status__u8 & 0x01) != 0)
  {
    uint8_t Num_bytes_read__u8 = bme280_read(eBME280reg_STATUS, &Status__u8, 1);
    if (Num_bytes_read__u8 != 1)
    {
        printf("failed to read 8 bytes\r\n");
      return Return_status__i;
    }
  }

  const uint8_t Num_bytes_to_read__u8 = 8;
  uint8_t Buffer__u8a[Num_bytes_to_read__u8];
  int Num_retries__i = 0;
  while (Num_retries__i <= Num_allowed_retries__i)
  {
    uint8_t Register__u8 = eBME280reg_PRESDATA;
    int Num_bytes_read__i = bme280_read(Register__u8, Buffer__u8a,
      Num_bytes_to_read__u8);
    if (Num_bytes_read__i ==  (int)Num_bytes_to_read__u8)
    {
      // Decode the fields.

      // Pressure is in registers 0xf7 ~ 0xf9.
      // Most Significant Bits [19:12] of Pressure ADC value.
      int32_t Pressure_raw_adc__i32 = ((int32_t)Buffer__u8a[0]) << 12;
      // Mid/lower Significant Bits [11:4] of Pressure ADC value.
      Pressure_raw_adc__i32 += ((int32_t)Buffer__u8a[1]) << 4;
      // Least Significant Bits [3]|[3:2]|[3:1]|[3:0], depending on the
      // resolution as determined by the oversampling setting.
      Pressure_raw_adc__i32 += ((int32_t)Buffer__u8a[2]) & 0x04;

      // Temperature is in registers 0xfa ~ 0xfc.
      // Most Significant Bits [19:12] of Temperature ADC value.
      int32_t Temperature_raw_adc__i32 = ((int32_t)Buffer__u8a[3]) << 12;
      // Mid/lower Significant Bits [11:4] of Temperature ADC value.
      Temperature_raw_adc__i32 += ((int32_t)Buffer__u8a[4]) << 4;
      // Least Significant Bits [3]|[3:2]|[3:1]|[3:0], depending on the
      // resolution as determined by the oversampling setting.
      Temperature_raw_adc__i32 += ((int32_t)Buffer__u8a[5]) & 0x04;

      // Humidity is in registers 0xfd ~ 0xfe.
      // Most Significant Bits [15:8] of Humidity ADC value.
      int32_t Humidity_raw_adc__i32 = (((int32_t)Buffer__u8a[6]) << 8);
      // Least Significant Bits [7:0] of Humidity ADC value.
      Humidity_raw_adc__i32 += ((int32_t)Buffer__u8a[7]);

      *Temp_c__fp = bme280_compensate_T_int32(Temperature_raw_adc__i32) / 100.0;
      *Pres_Pa__fp = bme280_compensate_P_int64(Pressure_raw_adc__i32) / 256.0;
      *Hum_pct__fp = bme280_compensate_H_int32(Humidity_raw_adc__i32) / 1024.0;

      Return_status__i = 1;
      break;
    }

    Num_retries__i++;
    delay(1);
  }

  return Return_status__i;
}
