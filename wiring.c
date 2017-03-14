/*
* IoT Hub Raspberry Pi C - Microsoft Sample Code - Copyright (c) 2017 - Licensed MIT
*/
#include "./wiring.h"

static unsigned int BMEInitMark = 0;

#if SIMULATED_DATA
int readMessage(int messageId, char *payload)
{
    snprintf(payload, 
        sizeof(payload), "{ messageId: %d, temperature: %f, humidity: %f }", 
        messageId, 
        random(20, 30), 
        random(60, 80));
    return 1;
}

float random(int min, int max)
{
    srand((unsigned int)time(NULL));
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX * (max - min)));
}
#else
int mask_check(int check, int mask)
{
    return (check & mask) == mask;
}

// check whether the BMEInitMark's corresponding mark bit is set, if not, try to invoke corresponding init()
int check_bme_init()
{
    // wiringPiSetup == 0 is successful
    if (mask_check(BMEInitMark, WIRINGPI_SETUP) != 1 && wiringPiSetup() != 0)
    {
        return -1;
    }
    BMEInitMark |= WIRINGPI_SETUP;

    // wiringPiSetup < 0 means error
    if (mask_check(BMEInitMark, SPI_SETUP) != 1 && wiringPiSPISetup(SPI_CHANNEL, SPI_CLOCK) < 0)
    {
        return -1;
    }
    BMEInitMark |= SPI_SETUP;

    // bme280_init == 1 is successful
    if (mask_check(BMEInitMark, BME_INIT) != 1 && bme280_init(SPI_CHANNEL) != 1)
    {
        return -1;
    }
    BMEInitMark |= BME_INIT;
    return 1;
}

// check the BMEInitMark value is equal to the (WIRINGPI_SETUP | SPI_SETUP | BME_INIT)

int readMessage(int messageId, char *payload)
{
    if (check_bme_init() != 1)
    {
        // setup failed
        return -1;
    }

    float temperature, humidity, pressure;
    if (bme280_read_sensors(&temperature, &pressure, &humidity) != 1)
    {
        return -1;
    }

    snprintf(payload, BUFFER_SIZE, "{ messageId: %d, temperature: %f, humidity: %f }", messageId, temperature, humidity);
    return 1;
}
#endif

void blinkLED()
{
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
}

void setupWiring()
{
    if (wiringPiSetup() == 0)
    {
        BMEInitMark |= WIRINGPI_SETUP;
    }
    pinMode(LED_PIN, OUTPUT);
}
