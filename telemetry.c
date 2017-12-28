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
static const char *BODY_TEMPLATE_FULL =
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

static const char *BODY_TEMPLATE_MIN =
"{"
    "\"data\": {"
        "\"baseType\": \"EventData\","
        "\"baseData\": {"
            "\"properties\": {"
                "\"language\": \"C\","
                "\"device\": \"RaspberryPi\""
            "},"
            "\"name\": \"%s\""
        "}"
    "},"
    "\"time\": \"%s\","
    "\"name\": \"%s\","
    "\"iKey\": \"%s\","
    "\"tags\": {"
        "\"ai.location.ip\": \"0.0.0.0\""
    "}"
"}";

static const char *PROMPT_TEXT = "\nMicrosoft would like to collect data about how users use Azure IoT " \
"samples and some problems they encounter. Microsoft uses this information to improve "\
"our tooling experience. Participation is voluntary and when you choose to participate " \
"your device automatically sends information to Microsoft about how you use Azure IoT "\
"samples. If you want to change this setting after first time, please delete the "\
"telemetry.config file and restart the program. "\
"\n\nSelect y to enable data collection (y/n, default is y). ";

static int enabled = 0;

void initial_telemetry()
{
    FILE *file;
    file = fopen("telemetry.config", "r");
    if (file != NULL)
    {
        char config = (char)fgetc(file);
        enabled = config == 'y';
        fclose(file);
    }
    else
    {
        printf("%s", PROMPT_TEXT);
        char answer = getchar();
        file = fopen("telemetry.config", "w");
        if (answer == 'n')
        {
            fputc('n', file);
            send_telemetry_data_without_sensitive_info("no");
            enabled = 0;
        }
        else
        {
            fputc('y', file);
            send_telemetry_data_without_sensitive_info("yes");
            enabled = 1;
        }
        fclose(file);
    }
}

void get_mac_address_hash(char outputBuffer[], int len)
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

        sha256(mac, outputBuffer, len);
    }

    close(fd);
}

void sha256(const char *str, char outputBuffer[], int len)
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
        snprintf(outputBuffer + (i * 2), len, "%02x", hash[i]);
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

int substr_count(const char * str, const char * substr)
{
    const char *tmp = str;
    int count = 0;
    while (tmp = strstr(tmp, substr))
    {
        count++;
        tmp += strlen(substr);
    }
    return count;
}

void send_telemetry_data_without_sensitive_info(const char *event)
{
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, PATH);

        time_t t = time(NULL);
        char *cur_time = ctime(&t);
        int tlen = strlen(cur_time) - 1;
        cur_time[tlen] = 0;

        int str_placeholder_count = substr_count(BODY_TEMPLATE_MIN, "%s");
        int size = strlen(BODY_TEMPLATE_MIN) + strlen(IKEY) - strlen("%s") * str_placeholder_count
            + strlen(EVENT) + strlen(event) + tlen + 1;

        char *data = (char *)malloc(size * sizeof(char));
        if (data != NULL)
        {
            snprintf(data, size, BODY_TEMPLATE_MIN, event, cur_time, EVENT, IKEY);
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

void send_telemetry_data(const char *iotHubName, const char *event, const char *message)
{
    if (!enabled)
    {
        return;
    }

    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, PATH);

        char hash_mac[HASH_LEN];
        unsigned char hash_iothub_name[HASH_LEN];
        get_mac_address_hash(hash_mac, sizeof(hash_mac));
        sha256(iotHubName, hash_iothub_name, sizeof(hash_iothub_name));

        time_t t = time(NULL);
        char *cur_time = ctime(&t);
        int tlen = strlen(cur_time) - 1;
        cur_time[tlen] = 0;

        uname(&platform);
        char os_release[LINE_BUFSIZE];
        char os_platform[LINE_BUFSIZE];
        const char prefix[] = "ID_LIKE=";

        exec_command("/usr/bin/lsb_release -r| grep -oP \'\\d\\.\\d\' | tr -d \'\\r\\n\'", os_release);
        exec_command("cat /etc/os-release | grep -oP \'^ID_LIKE=.*$\' | tr -d \'\\r\\n\'", os_platform);
        int str_placeholder_count = substr_count(BODY_TEMPLATE_FULL, "%s");
        int platform_str_offset =  strlen(prefix);
        int size = strlen(BODY_TEMPLATE_FULL) + strlen(LANGUAGE) + strlen(DEVICE) + strlen(MCU) + strlen(EVENT)
             + strlen(IKEY) - strlen("%s") * str_placeholder_count + strlen(hash_iothub_name)
             + strlen(hash_mac) + strlen(platform.sysname) + strlen(os_platform + platform_str_offset)
             + strlen(os_release) + strlen(message) + strlen(event) + tlen + 1;
        char *data = (char *)malloc(size * sizeof(char));
        if (data != NULL)
        {
            snprintf(data, size, BODY_TEMPLATE_FULL, LANGUAGE, DEVICE, MCU, message, hash_mac, hash_iothub_name,
                platform.sysname, os_platform + platform_str_offset, os_release, event, cur_time, EVENT, IKEY);
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
