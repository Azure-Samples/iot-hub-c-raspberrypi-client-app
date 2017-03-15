/*
* IoT Hub Raspberry Pi C - Microsoft Sample Code - Copyright (c) 2017 - Licensed MIT
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "./config.h"
#include "./wiring.h"

#include <azure_c_shared_utility/platform.h>
#include <azure_c_shared_utility/threadapi.h>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <iothub_client.h>
#include <iothub_client_options.h>
#include <iothub_message.h>
#include <iothubtransportmqtt.h>

const char *onSuccess = "\"Successfully invoke device method\"";
const char *notFound = "\"No method found\"";

static bool messagePending = false;
static bool sendingMessage = true;

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

static void start()
{
    sendingMessage = true;
}

static void stop()
{
    sendingMessage = false;
}

int deviceMethodCallback(
    const char *methodName,
    const unsigned char *payload,
    size_t size,
    unsigned char **response,
    size_t *response_size,
    void *userContextCallback)
{
    (void)printf("Try to invoke method %s\r\n", methodName);
    const char *responseMessage = onSuccess;
    int result = 200;

    if (strcmp(methodName, "start") == 0)
    {
        start();
    }
    else if (strcmp(methodName, "stop") == 0)
    {
        stop();
    }
    else
    {
        (void)printf("No method %s found\r\n", methodName);
        responseMessage = notFound;
        result = 404;
    }

    *response_size = strlen(responseMessage);
    *response = (unsigned char *)malloc(*response_size);
    strncpy((char *)(*response), responseMessage, *response_size);

    return result;
}

IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
    const unsigned char *buffer = NULL;
    size_t size = 0;

    if (IOTHUB_MESSAGE_OK != IoTHubMessage_GetByteArray(message, &buffer, &size))
    {
        return IOTHUBMESSAGE_ABANDONED;
    }

    // message needs to be converted to zero terminated string
    char *temp = malloc(size + 1);

    if (temp == NULL)
    {
        return IOTHUBMESSAGE_ABANDONED;
    }

    strncpy(temp, buffer, size);
    temp[size] = '\0';

    printf("Receiving message: %s\r\n", temp);

    return IOTHUBMESSAGE_ACCEPTED;
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

            // set C2D and device method callback
            IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);
            IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, deviceMethodCallback, NULL);

            int count = 0;
            while (true)
            {
                if (sendingMessage && !messagePending)
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
