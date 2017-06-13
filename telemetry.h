#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netdb.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

void send_telemetry_data(const char *iotHubName, const char *event, const char *message);
void get_mac_address_hash(char outputBuffer[65]);
void sha256(const char *string, char outputBuffer[65]);