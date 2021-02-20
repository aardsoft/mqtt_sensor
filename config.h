/**
 * @file config.h
 * @copyright 2021
 * @author Bernd Wachter <bernd-github@wachter.fi>
 * @date 2021
 */

#ifndef _CONFIG_H
#define _CONFIG_H

#include <Arduino.h>

#define CONFIG_MAGIC 0x100B
#define APP_MAGIC 0x100A

#ifndef MQTT_USER_MAX
#define MQTT_USER_MAX 17
#endif

#ifndef MQTT_PASSWORD_MAX
#define MQTT_PASSWORD_MAX 17
#endif

#ifndef MQTT_HOST_MAX
#define MQTT_HOST_MAX 32
#endif

struct ConfigData {
    // magic to identify if the configuration is compatible
    unsigned int magic;
    // MAC address used by this device
    char mac[18];
    // App specific magic to verify that the configuration belongs to this app
    byte app_magic;

    char mqtt_user[MQTT_USER_MAX];
    char mqtt_pass[MQTT_PASSWORD_MAX];
    char mqtt_host[MQTT_HOST_MAX];

    int mqtt_port;
    long report_interval;
    boolean mqtt_retain;
    // node name reported by this device
    char node[30];
};

#define CONFIG_STRUCT ConfigData

#endif
