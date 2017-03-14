/*
* IoT Hub Raspberry Pi C - Microsoft Sample Code - Copyright (c) 2017 - Licensed MIT
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "wiring.h"

#include "./azure_c_shared_utility/platform.h"
#include "./azure_c_shared_utility/threadapi.h"
#include "./azure_c_shared_utility/crt_abstractions.h"
#include "./iothub_client.h"
#include "./iothub_client_options.h"
#include "./iothub_message.h"
#include "./iothubtransportmqtt.h"

static bool messagePending = false;

static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
    if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
    {
        blinkLED();
    }
    else
    {
        (void)printf("Failed to send message to Azure IoT Hub\r\n");
    }

    messagePending = false;
}

static void sendMessages(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char *buffer)
{
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, strlen(buffer));
    if (messageHandle == NULL)
    {
        (void)printf("Unable to create a new IoTHubMessage\r\n");
    }
    else
    {
        (void)printf("Sending message: %s\r\n", buffer);
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, NULL) != IOTHUB_CLIENT_OK)
        {
            (void)printf("Failed to send message to Azure IoT Hub\r\n");
        }
        else
        {
            messagePending = true;
            (void)printf("Message sent to Azure IoT Hub\r\n");
        }

        IoTHubMessage_Destroy(messageHandle);
    }
}

static char *get_device_id(char *str)
{
    char *substr = strstr(str, "DeviceId=");

    if (substr == NULL)
        return NULL;

    // skip "DeviceId="
    substr += 9;

    char *semicolon = strstr(substr, ";");
    int length = semicolon == NULL ? strlen(substr) : semicolon - substr;
    char *device_id = calloc(1, length + 1);
    memcpy(device_id, substr, length);
    device_id[length] = '\0';

    return device_id;
}

static char *readFile(char *fileName)
{
    FILE *fp;
    int size;
    char *buffer;

    fp = fopen(fileName, "rb");

    if (fp == NULL)
    {
        (void)printf("ERROR: File %s doesn't exist!\n", fileName);
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    // Allocate memory for entire content
    buffer = calloc(1, size + 1);

    if (buffer == NULL)
    {
        fclose(fp);
        (void)printf("ERROR: Failed to allocate memory.\n");
        return NULL;
    }

    // Read the file into the buffer
    if (1 != fread(buffer, size, 1, fp))
    {
        fclose(fp);
        free(buffer);
        (void)printf("ERROR: Failed to read the file %s into memory.\n", fileName);
        return NULL;
    }

    fclose(fp);

    return buffer;
}

static bool setX509Certificate(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char *deviceId)
{
    char certName[256];
    char keyName[256];
    char cwd[1024];

    snprintf(certName, sizeof(certName), "%s/%s-cert.pem", CREDENTIAL_PATH, deviceId);
    snprintf(keyName, sizeof(keyName), "%s/%s-key.pem", CREDENTIAL_PATH, deviceId);

    char *x509certificate = readFile(certName);
    char *x509privatekey = readFile(keyName);

    if (x509certificate == NULL ||
        x509privatekey == NULL ||
        IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_X509_CERT, x509certificate) != IOTHUB_CLIENT_OK ||
        IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, x509privatekey) != IOTHUB_CLIENT_OK)
    {
        (void)printf("ERROR: Failed to set options for x509.\n");
        return false;
    }

    free(x509certificate);
    free(x509privatekey);

    return true;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        (void)printf("Usage: %s <IoT hub device connection string>\r\n", argv[0]);
        return 1;
    }

    setupWiring();

    char device_id[257];
    char *device_id_src = get_device_id(argv[1]);

    if (device_id_src == NULL)
    {
        (void)printf("ERROR: Cannot parse device id from IoT device connection string\n");
        return 1;
    }
    snprintf(device_id, sizeof(device_id), "%s", device_id_src);
    free(device_id_src);

    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

    if (platform_init() != 0)
    {
        (void)printf("Failed to initialize the platform.\r\n");
    }
    else
    {
        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(argv[1], MQTT_Protocol)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
            if (strstr(argv[1], "x509=true") != NULL)
            {
                // Use X.509 certificate authentication.
                if (!setX509Certificate(iotHubClientHandle, device_id))
                {
                    return 1;
                }
            }

            int count = 0;
            while (true)
            {
                if (!messagePending)
                {
                    ++count;
                    char buffer[BUFFER_SIZE];
                    if (buffer != NULL)
                    {
                        if (readMessage(count, buffer) == 1)
                        {
                            sendMessages(iotHubClientHandle, buffer);
                        }
                        else
                        {
                            (void)printf("Failed to read message\r\n");
                        }
                    }
                    delay(INTERVAL);
                }
                IoTHubClient_LL_DoWork(iotHubClientHandle);
            }

            IoTHubClient_LL_Destroy(iotHubClientHandle);
        }
        platform_deinit();
    }

    return 0;
}
