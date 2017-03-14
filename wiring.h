#include <stdio.h>
#include <time.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "config.h"

#define WIRINGPI_SETUP 1

#if !SIMULATED_DATA
#include "bme280.h"

#define SPI_CHANNEL 0
#define SPI_CLOCK 1000000L

#define SPI_SETUP 1 << 2
#define BME_INIT 1 << 3
#endif

int readMessage(int messageId, char *payload);
void blinkLED();
void setupWiring();