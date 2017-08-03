/*
* IoT Hub Raspberry Pi C - Microsoft Sample Code - Copyright (c) 2017 - Licensed MIT
*/
#include "./telemetry.h"

static struct utsname platform;
static const char *PATH = "https://dc.services.visualstudio.com/v2/track";
static const char *IKEY = "0823bae8-a3b8-4fd5-80e5-f7272a2377a9";
static const char *EVENT = "AIEVENT";
static const char *LANGUAGE = "C";
static const char *DEVICE = "RaspberryPi";
static const char *MCU = "STM32F412";
static const char *MAC_TEMPLATE = "%02X-%02X-%02X-%02X-%02X-%02X";
static const int HASH_LEN = 65;
static const char *BODY_TEMPLATE =
"{"
    "\"data\": {"
        "\"baseType\": \"EventData\","
        "\"baseData\": {"
            "\"properties\": {"
                "\"language\": \"%s\","
                "\"device\": \"%s\","
                "\"mcu\": \"%s\","
                "\"message\": \"%s\","
                "\"mac\": \"%s\","
                "\"iothub\": \"%s\","
                "\"osType\": \"%s\","
                "\"osPlatform\": \"%s\","
                "\"osRelease\": \"%s\""
            "},"
            "\"name\": \"%s\""
        "}"
    "},"
    "\"time\": \"%s\","
    "\"name\": \"%s\","
    "\"iKey\": \"%s\""
"}";

void get_mac_address_hash(char outputBuffer[])
{
    struct ifreq s;
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    snprintf(s.ifr_name, sizeof(s.ifr_name), "eth0");
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
    {
        char mac[18];
        snprintf(mac, sizeof(mac), MAC_TEMPLATE, (unsigned char)s.ifr_addr.sa_data[0],
            (unsigned char)s.ifr_addr.sa_data[1], (unsigned char)s.ifr_addr.sa_data[2],
            (unsigned char)s.ifr_addr.sa_data[3], (unsigned char)s.ifr_addr.sa_data[4],
            (unsigned char)s.ifr_addr.sa_data[5]);
        mac[17] = '\n';

        sha256(mac, outputBuffer);
    }

    close(fd);
}

void sha256(const char *str, char outputBuffer[])
{
    if (str == NULL)
    {
        outputBuffer[0] = '\0';
        return;
    }
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, strlen(str));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        snprintf(outputBuffer + (i * 2), sizeof(outputBuffer), "%02x", hash[i]);
    }

    outputBuffer[HASH_LEN - 1] = 0;
}

void exec_command(char command [], char result [])
{
    FILE * pipe = popen(command, "r");
    if (pipe == NULL)
    {
        result = "unknown";
        return;
    }

    fgets(result, LINE_BUFSIZE, pipe);
    if (strlen(result) == 0)
    {
        result = "unknown";
    }
}

void send_telemetry_data(const char *iotHubName, const char *event, const char *message)
{
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, PATH);

        char hash_mac[HASH_LEN];
        unsigned char hash_iothub_name[HASH_LEN];
        get_mac_address_hash(hash_mac);
        sha256(iotHubName, hash_iothub_name);

        time_t t = time(NULL);
        char *cur_time = ctime(&t);
        int tlen = strlen(cur_time) - 1;
        cur_time[tlen] = 0;

        uname(&platform);
        char osRelease[10];
        char osPlatform[20];

        exec_command("/usr/bin/lsb_release -r| grep -oP \'\\d\\.\\d\' | tr -d \'\\r\\n\'", osRelease);
        exec_command("cat /etc/os-release | grep -oP \'^ID_LIKE=.*$\' | tr -d \'\\r\\n\'", osPlatform);

        int size = strlen(BODY_TEMPLATE) + strlen(LANGUAGE) + strlen(DEVICE) + strlen(MCU) + strlen(EVENT)
             + strlen(IKEY) - strlen("%s") * 13 + strlen(hash_iothub_name) + strlen(hash_mac) + strlen(platform.sysname)
             + strlen(osPlatform + strlen("ID_LIKE=")) + strlen(osRelease) + strlen(message) + strlen(event) + tlen + 1;
        char *data = (char *)malloc(size * sizeof(char));
        if (data != NULL)
        {
            snprintf(data, size, BODY_TEMPLATE, LANGUAGE, DEVICE, MCU, message, hash_mac, hash_iothub_name,
                platform.sysname, osPlatform + strlen("ID_LIKE="), osRelease, event, cur_time, EVENT, IKEY);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            // hide curl output
            FILE *devnull = fopen("/dev/null", "w+");
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, devnull);

            res = curl_easy_perform(curl);
            fclose(devnull);

            curl_easy_cleanup(curl);

            free(data);
        }
    }
}
