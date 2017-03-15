/*
* IoT Hub Raspberry Pi C - Microsoft Sample Code - Copyright (c) 2017 - Licensed MIT
*/

///////////////////////////////////////////////////////////////////////////////
//
// bme280.h:
// SPI based interface to read temperature, pressure and humidity samples from
// a BME280 module.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef BME280_H_
#define BME280_H_


///////////////////////////////////////////////////////////////////////////////
// Call this after setting the chip select (or SPI Enable) pin (via
// bme280_set_cs_pin()), and before calling the bmp280_read function.
// Return: 0 if the module was not found.
//         1 if the module was readable, and verified to be a BMP280, and the
//           calibration data was read.
int bme280_init(int Chip_enable_to_use__i);

///////////////////////////////////////////////////////////////////////////////
// Prerequisite:
// You must call wiringPiSetup before calling this function. For example:
//  int Result__i = wiringPiSetup();
//  if (Result__i != 0) exit(Result__i);
// You must call wiringPiSPISetup before calling this function. For example:
//  int Spi_fd__i = wiringPiSPISetup(Spi_channel__i, Spi_clock__i);
//  if (Spi_fd__i < 0)
//  {
//    printf("Can't setup SPI, error %i calling wiringPiSPISetup(%i, %i)  %s\n",
//      Spi_fd__i, Spi_channel__i, Spi_clock__i, strerror(Spi_fd__i));
//    exit(Spi_fd__i);
//  }
//
// Param: Temp_C__fp  Pointer to a float to receive the current temperature in
//                    degrees Celcius. Only set if read is successful.
// Param: Pres_Pa__fp  Pointer to a float to receive the current pressure
//                     as hPa. Only set if read is successful.
// Param: Hum_pct__fp  Pointer to a float to receive the current humidity
//                     as a percentage. Only set if read is successful.
// Return: If wiringPi gets an error, this will be < 0
//         If the read attempts fail, this will be 1
//         If the read succeeds within the available retries, returns 0
int bme280_read_sensors(float * Temp_C__fp, float * Pres_Pa__fp, float * Hum_pct__fp);

#endif  // BME280_H_
