/*
* IoT Hub Raspberry Pi C - Microsoft Sample Code - Copyright (c) 2017 - Licensed MIT
*/
#ifndef TELEMETRY_H_
#define TELEMETRY_H_

#define LINE_BUFSIZE 128

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <linux/if.h>
#include <netdb.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <unistd.h>

void send_telemetry_data(const char *iotHubName, const char *event, const char *message);
void get_mac_address_hash(char outputBuffer[], int len);
void sha256(const char *str, char outputBuffer[], int len);
void exec_command(char command [], char result []);
int substr_count(const char * str, const char * substr);
void initial_telemetry();
void send_telemetry_data_without_sensitive_info(const char *event);

#endif  // TELEMETRY_H_
