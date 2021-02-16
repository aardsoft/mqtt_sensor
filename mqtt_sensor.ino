#define debug_log 0

#include "config.h"

#include <MACTool.h>
#include <EEPROM.h>
#ifdef UIP
#include <UIPEthernet.h>
#endif
#ifdef ETHERNET
#include <Ethernet.h>
#endif
#include <PubSubClient.h>
#include <EnvironmentMonitor.h>
#include <PersistentConfiguration.h>

#include <avr/wdt.h>
#include <math.h>

#include "mqtt.h"

#if debug_log == 1
#define debug_print(msg) Serial.print(msg)
#define debug_println(msg) Serial.println(msg)
#else
#define debug_print(msg)
#define debug_println(msg)
#endif

// allow overriding mqtt connection settings via mqtt
#ifndef MQTT_CONFIG
#define MQTT_CONFIG 1
#endif

#ifndef MQTT_USER
#define MQTT_USER "arduino"
#endif

#ifndef MQTT_BASE_TOPIC
#define MQTT_BASE_TOPIC "mqtt_sensor"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_MAX_CONNECTION_ATTEMPTS
#define MQTT_MAX_CONNECTION_ATTEMPTS 3
#endif

void mqtt_callback(char* topic, byte* payload, unsigned int length);

int LED=13;

// static shared buffers
char ltoa_buf[20];
char itoa_buf[10];

// uptime counters for the system and the MQTT connection
unsigned long uptime=0;
unsigned long mqtt_uptime=0;

// connection attempt counter to trigger reset on persistent failure
byte mqtt_connect_counter=0;

// base string used for building MQTT topics
String node_topic(MQTT_BASE_TOPIC);

PersistentConfiguration c;

EthernetClient ethClient;
PubSubClient client;

EnvironmentMonitor mon;
measurement *measurements;
byte measurement_p=0;

void mqtt_callback(char* topic, byte* payload, unsigned int length){
  String tmp_topic=node_topic+"?";
  /*
   * handle message arrived
   *
   * inumber   set the report interval
   * nnodename configure a new node name
   * r         reset by triggering watchdog
   * t0|1      set the retain flag
   * m         mqtt-settings with following sub-keys
   * muuser    mqtt user
   * mppass    mqtt password
   * msserver  mqtt server
   * mtport    mqtt port
   */

  switch (payload[0]){
    case 'i':
      if (length<10){
        strlcpy(itoa_buf, payload+1, length);
        config.report_interval=atoi(itoa_buf);
        state.config_changed=true;
        client.publish(tmp_topic.c_str(), "iOK");
      } else
        client.publish(tmp_topic.c_str(), "iE");
      break;
    case 'r':
      delay(10000);
    case 'n':
      if (length<=sizeof(config.node)){
        strlcpy(config.node, payload+1, length);
        state.config_changed=true;
        client.publish(tmp_topic.c_str(), "nOK");
      } else {
        client.publish(tmp_topic.c_str(), "nE");
      }
      break;
#if MQTT_CONFIG > 0
    case 'm':
      switch (payload[1]){
        case 'u':
          if (length-1<=sizeof(config.mqtt_user)){
            strlcpy(config.mqtt_user, payload+2, length-1);
            state.config_changed=true;
            client.publish(tmp_topic.c_str(), "muOK");
          } else {
            client.publish(tmp_topic.c_str(), "muE");
          }
          break;
        case 'p':
          if (length-1<=sizeof(config.mqtt_pass)){
            strlcpy(config.mqtt_pass, payload+2, length-1);
            state.config_changed=true;
            client.publish(tmp_topic.c_str(), "mpOK");
          } else {
            client.publish(tmp_topic.c_str(), "mpE");
          }
          break;
        case 's':
          if (length-1<=sizeof(config.mqtt_host)){
            strlcpy(config.mqtt_host, payload+2, length-1);
            state.config_changed=true;
            client.publish(tmp_topic.c_str(), "msOK");
          } else {
            client.publish(tmp_topic.c_str(), "msE");
          }
          break;
        case 't':
          if (length-1<=10){
            strlcpy(itoa_buf, payload+1, length-1);
            config.mqtt_port=atoi(itoa_buf);
            state.config_changed=true;
            client.publish(tmp_topic.c_str(), "mtOK");
          } else {
            client.publish(tmp_topic.c_str(), "mtE");
          }
          break;
      }
      break;
#endif
    case 't':
      if (payload[1]=='0'){
        config.mqtt_retain=false;
        state.config_changed=true;
        client.publish(tmp_topic.c_str(), "tOK");
      } else if (payload[1]=='1'){
        config.mqtt_retain=true;
        state.config_changed=true;
        client.publish(tmp_topic.c_str(), "tOK");
      } else
        client.publish(tmp_topic.c_str(), "tE");
      break;

  }

  c.save();

  tmp_topic=node_topic+"s";
  client.publish(tmp_topic.c_str(), NULL);
}

void report_sensors(){
  String tmp_topic;
#if ENVIRONMENTMONITOR_SENSOR_DHT22 > 0
  if (mon.hasDHT22()){
    tmp_topic=node_topic+"t/dht22";
    dtostrf(measurements[0].dht22.temperature, 3, 1, itoa_buf);
    client.publish(tmp_topic.c_str(),
                   itoa_buf,
                   config.mqtt_retain);
    tmp_topic=node_topic+"h/dht22";
    dtostrf(measurements[0].dht22.humidity, 3, 1, itoa_buf);
    client.publish(tmp_topic.c_str(),
                   itoa_buf,
                   config.mqtt_retain);
  }
#endif

#if ENVIRONMENTMONITOR_SENSOR_MCP9808 > 0
  if (mon.hasMCP9808()){
    tmp_topic=node_topic+"t/mcp9808";
    dtostrf(measurements[0].mcp9808.temperature, 3, 1, itoa_buf);
    client.publish(tmp_topic.c_str(),
                   itoa_buf,
                   config.mqtt_retain);
  }
#endif

#if ENVIRONMENTMONITOR_SENSOR_BMP085 > 0
  if (mon.hasBMP085()){
    tmp_topic=node_topic+"t/bmp085";
    dtostrf(measurements[0].bmp085.temperature, 3, 1, itoa_buf);
    client.publish(tmp_topic.c_str(),
                   itoa_buf,
                   config.mqtt_retain);
    tmp_topic=node_topic+"p/bmp085";
    dtostrf(measurements[0].bmp085.pressure, 5, 1, itoa_buf);
    client.publish(tmp_topic.c_str(),
                   itoa_buf,
                   config.mqtt_retain);
  }
#endif

#if ENVIRONMENTMONITOR_SENSOR_RAIN > 0
  tmp_topic=node_topic+"r/rain";
  itoa(measurements[0].rain.rain, itoa_buf, 10);
  client.publish(tmp_topic.c_str(),
                 itoa_buf,
                 config.mqtt_retain);
#endif
}

void report_uptime(){
  String tmp_topic;

  tmp_topic=node_topic+"i/mqtt_up";
  ltoa((uptime-mqtt_uptime)/1000, ltoa_buf, 10);
  client.publish(tmp_topic.c_str(),
                 ltoa_buf);
  tmp_topic=node_topic+"i/up";
  ltoa(uptime/1000, ltoa_buf, 10);
  client.publish(tmp_topic.c_str(),
                 ltoa_buf);
}

void report_sensor_availability(){
  String tmp_topic;
#if ENVIRONMENTMONITOR_SENSOR_DHT22 > 0
  tmp_topic=node_topic+"i/dht22";
  client.publish(tmp_topic.c_str(),
                 mon.hasDHT22()?"true":"false",
                 config.mqtt_retain);
#endif


#if ENVIRONMENTMONITOR_SENSOR_MCP9808 > 0
  tmp_topic=node_topic+"i/mcp9808";
  client.publish(tmp_topic.c_str(),
                 mon.hasMCP9808()?"true":"false",
                 config.mqtt_retain);
#endif

#if ENVIRONMENTMONITOR_SENSOR_BMP085 > 0
  tmp_topic=node_topic+"i/bmp085";
  client.publish(tmp_topic.c_str(),
                 mon.hasBMP085()?"true":"false",
                 config.mqtt_retain);
#endif

}

boolean reconnect_mqtt(){
  if (client.connect(
        config.node,
        config.mqtt_user,
        config.mqtt_pass)){
    mqtt_connect_counter=0;
    mqtt_uptime=millis();
    String tmp_topic=node_topic+"i/v";
    client.publish(tmp_topic.c_str(),
                   VERSION_STRING,
                   config.mqtt_retain);
#ifdef BUILD_TAG
    tmp_topic=node_topic+"i/b";
    client.publish(tmp_topic.c_str(),
                   BUILD_TAG,
                   config.mqtt_retain);
#endif
#ifdef GIT_HASH
    tmp_topic=node_topic+"i/c";
    client.publish(tmp_topic.c_str(),
                   GIT_HASH,
                   config.mqtt_retain);
#endif
    report_uptime();
    report_sensor_availability();

    tmp_topic=node_topic+"s";
    client.subscribe(tmp_topic.c_str());
  } else {
    mqtt_connect_counter++;
  }

  // trigger watchdog if connection is not possible for some time
  if (mqtt_connect_counter>=MQTT_MAX_CONNECTION_ATTEMPTS){
    delay(10000);
  }

  return true;
}

void setup(){
#if debug_log > 0
  Serial.begin(9600);
#endif

  debug_println("Init");
  c.load();

  init_MAC();

  if (config.magic != CONFIG_MAGIC){
    memset(config.mqtt_user, 0, sizeof(config.mqtt_user));
    memset(config.mqtt_pass, 0, sizeof(config.mqtt_pass));
    memset(config.mqtt_host, 0, sizeof(config.mqtt_host));
    strlcpy(config.mqtt_user, MQTT_USER, sizeof(config.mqtt_user));
    strlcpy(config.mqtt_pass, MQTT_PASSWORD, sizeof(config.mqtt_pass));
    strlcpy(config.mqtt_host, MQTT_SERVER, sizeof(config.mqtt_host));
    config.mqtt_port=MQTT_PORT;

    memset(config.node, 0, sizeof(config.node));
    strlcpy(config.node, config.mac, sizeof(config.node));

    config.report_interval=6;
    config.mqtt_retain=false;

    state.config_changed=true;
  }

  c.save();

  pinMode(LED, OUTPUT);

  node_topic+="/";
  node_topic+=config.node;
  node_topic+="/";

  // on a device with a bootloader from before ~2018 triggering the watchdog
  // will cause a reboot loop. Update bootloader if that happens.
  wdt_enable(WDTO_8S);

  byte mac[8];
  MAC_to_byte(config.mac, mac);
  if (Ethernet.begin(mac) == 0){
    // go to endless loop, triggering watchdog, if Ethernet can't be
    // initialized
    for (;;);
  }

  mon.begin();

  client.setClient(ethClient);
  client.setServer(config.mqtt_host,
                   config.mqtt_port);
  client.setCallback(mqtt_callback);
}

void loop(){
  wdt_reset();

  if (!client.connected()){
    reconnect_mqtt();
  } else {
    client.loop();
    unsigned long m=millis();
    if (m-uptime >= config.report_interval*1000){
      digitalWrite(LED, HIGH);
      uptime=m;
      report_uptime();

      if (mon.pollSensors()){
        measurements = mon.data(&measurement_p);
        report_sensors();
      }

      digitalWrite(LED, LOW);
    }
  }
}