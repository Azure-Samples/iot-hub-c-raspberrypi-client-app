---
services: iot-hub
platforms: C
author: shizn
---

# IoT Hub Raspberry Pi 3 Client application
[![Build Status](https://travis-ci.com/Azure-Samples/iot-hub-c-raspberrypi-client-app.svg?token=5ZpmkzKtuWLEXMPjmJ6P&branch=master)](https://travis-ci.com/Azure-Samples/iot-hub-c-raspberrypi-client-app)

> This repo contains the source code to help you get started with Azure IoT using the Microsoft IoT Pack for Raspberry Pi 3 Starter Kit. You will find the [full tutorial on Docs.microsoft.com](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-raspberry-pi-kit-c-get-started).

This repo contains an arduino application that runs on Raspberry Pi 3 with a BME280 temperature&humidity sensor, and then sends these data to your IoT hub. At the same time, this application receives Cloud-to-Device messages from your IoT hub, and takes actions according to the C2D command.

## Set up your Pi
### Enable SSH on your Pi
Follow [this page](https://www.raspberrypi.org/documentation/remote-access/ssh/) to enable SSH on your Pi.

### Enable SPI on your Pi
Follow [this page](https://www.raspberrypi.org/documentation/configuration/raspi-config.md) to enable SPI on your Pi

## Connect your sensor with your Pi
### Connect with a physical BEM280 sensor and LED
You can follow the image to connect your BME280 and a LED with your Raspberry Pi 3.

![BME280](https://docs.microsoft.com/en-us/azure/iot-hub/media/iot-hub-raspberry-pi-kit-c-get-started/3_raspberry-pi-sensor-connection.png)

## Download and setup the client app

1. Clone the client application to local:

   ```bash
   sudo apt-get install git-core

   git clone https://github.com/Azure-Samples/iot-hub-c-raspberrypi-client-app.git
   ```

2. Run setup script:

   ```bash
   cd ./iot-hub-c-raspberrypi-client-app

   sudo chmod u+x setup.sh

   sudo ./setup.sh
   ```

   **If you don't have a physical BME280, you can use '--simulated-data' as command line parameter to simulate temperature&humidity data.**

   ```bash
   sudo ./setup.sh --simulated-data
   ```

## Run your client application
Run the client application with root priviledge, and you also need provide your Azure IoT hub device connection string, note your connection should be quoted in the command.

```bash
sudo ./app '<your Azure IoT hub device connection string>'
```

### Send Cloud-to-Device command
You can send a C2D message to your device. You can see the device prints out the message and blinks once when receiving the message.

### Send Device Method command
You can send `start` or `stop` device method command to your Pi to start/stop sending message to your IoT hub.
