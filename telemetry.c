#include "telemetry.h"

static const char *PATH = "https://dc.services.visualstudio.com/v2/track";
static const char *IKEY = "0823bae8-a3b8-4fd5-80e5-f7272a2377a9";
static const char *EVENT = "AIEVENT";
static const char *DEVICE_TYPE = "Raspberry Pi C SDK";
static const char *VERSION = "Raspberry Pi 3";
static const char *MCU = "STM32F412";
static const char *MAC_TEMPLATE = "%02X-%02X-%02X-%02X-%02X-%02X";
static const char *BODY_TEMPLATE = 
"{"
    "\"data\": {"
        "\"baseType\": \"EventData\","
        "\"baseData\": {"
            "\"properties\": {"
                "\"device_type\": \"%s\","
                "\"hardware_version\": \"%s\","
                "\"mcu\": \"%s\","
                "\"message\":\"%s\","
                "\"hash_mac_address\": \"%s\","
                "\"hash_iothub_name\":\"%s\""
            "},"
            "\"name\": \"%s\""
        "}"
    "},"
    "\"time\": \"%s\","
    "\"name\": \"%s\","
    "\"iKey\": \"%s\""
"}";

void get_mac_address_hash(char outputBuffer[65])
{
    struct ifreq s;
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    strcpy(s.ifr_name, "eth0");
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
    {
        char mac[18];
        sprintf(mac, MAC_TEMPLATE, (unsigned char)s.ifr_addr.sa_data[0], (unsigned char)s.ifr_addr.sa_data[1], (unsigned char)s.ifr_addr.sa_data[2],
                (unsigned char)s.ifr_addr.sa_data[3], (unsigned char)s.ifr_addr.sa_data[4], (unsigned char)s.ifr_addr.sa_data[5]);
        mac[17] = '\n';

        sha256(mac, outputBuffer);
    }
}

void sha256(const char *string, char outputBuffer[65])
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
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

        char hash_mac[65];
        unsigned char hash_iothub_name[65];
        get_mac_address_hash(hash_mac);
        sha256(iotHubName, hash_iothub_name);

        time_t t = time(NULL);
        char *_ctime = ctime(&t);
        int tlen = strlen(_ctime) - 1;
        _ctime[tlen] = 0;

        int size = strlen(BODY_TEMPLATE) + strlen(DEVICE_TYPE) + strlen(VERSION) + strlen(MCU) + strlen(EVENT) + strlen(IKEY) - 20 + strlen(hash_iothub_name) + strlen(hash_mac) + strlen(message) + strlen(event) + tlen + 1;
        char *data = (char *)malloc(size * sizeof(char));

        sprintf(data, BODY_TEMPLATE, DEVICE_TYPE, VERSION, MCU, message, hash_mac, hash_iothub_name, event, _ctime, EVENT, IKEY);

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
