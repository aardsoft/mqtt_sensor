# default settings for the Arduino installation
# you can override those by creating either system.mk or ../arduino-system.mk
ARDUINO_DIR  = /usr/share/arduino

-include ../arduino-system.mk
-include conf/system.mk

# default settings for the board targetted.
# you can override those by creating board.mk
MCU          = atmega328p
F_CPU        = 16000000L
ARDUINO_PORT = /dev/ttyUSB1
#BOARD_TAG    = nano
BOARD_TAG    = pro
AVRDUDE_ARD_BAUDRATE = 115200
AVRDUDE_ARD_PROGRAMMER = arduino
HEX_MAXIMUM_SIZE = 32256

-include conf/board.mk

ARDUINO_LIBS = PersistentConfiguration MACTool EEPROM PubSubClient

# somewhat reduces binary size by preferring RCALL over CALL
LDFLAGS += -Wl,--relax

# make sure other libraries use the correct config struct
CXXFLAGS += -DCONFIG_STRUCT=ConfigData -include config.h

GIT_TAG = $(shell git describe --abbrev=0 --tags)
GIT_TAG_DIRTY = $(shell git describe --abbrev=0 --tags --dirty=".d")
GIT_HASH = $(shell git rev-parse --short HEAD)
GIT_COMMITS = $(shell git rev-list $(GIT_TAG).. --count)

SENSORS =
CONNECTIVITY = Ethernet
VERSION_STRING = $(GIT_TAG_DIRTY).$(GIT_COMMITS)

ifdef CONFIG
include conf/$(CONFIG).mk
ifndef BUILD_TAG
BUILD_TAG = $(CONFIG)
endif
else
-include conf/default-config.mk
endif

#CXXFLAGS += -DCONFIC_MAGIC=0000
CXXFLAGS += -DVERSION_STRING='"$(VERSION_STRING)"'
CXXFLAGS += -DGIT_HASH='"$(GIT_HASH)"'

ifdef BUILD_TAG
TARGET = mqtt_sensor-$(BUILD_TAG)-$(VERSION_STRING)
CXXFLAGS += -DBUILD_TAG='"$(BUILD_TAG)"'
else
TARGET = mqtt_sensor-$(VERSION_STRING)
endif

ifneq "$(findstring Ethernet,$(CONNECTIVITY))" ""
ARDUINO_LIBS += Ethernet SPI
CXXFLAGS += -DETHERNET
endif

ifneq "$(findstring UIP,$(CONNECTIVITY))" ""
ARDUINO_LIBS += UIPEthernet SPI
CXXFLAGS += -DUIP
endif

ifneq "$(findstring DHT,$(SENSORS))" ""
CXXFLAGS += -DENVIRONMENTMONITOR_SENSOR_DHT22=1
ARDUINO_LIBS += DHT EnvironmentMonitor
endif

ifneq "$(findstring MCP9808,$(SENSORS))" ""
CXXFLAGS += -DENVIRONMENTMONITOR_SENSOR_MCP9808=1
ARDUINO_LIBS += Adafruit_MCP9808 Wire SPI EnvironmentMonitor
#Adafruit_Sensor
endif

ifneq "$(findstring BMP085,$(SENSORS))" ""
CXXFLAGS += -DENVIRONMENTMONITOR_SENSOR_BMP085=1
ARDUINO_LIBS += Adafruit_BMP085 Wire EnvironmentMonitor
endif

ifneq "$(findstring BME280,$(SENSORS))" ""
CXXFLAGS += -DENVIRONMENTMONITOR_SENSOR_BME280=1
ARDUINO_LIBS += EnvironmentMonitor
 ifndef BME_LIBRARY
BME_LIBRARY = ADAFRUIT
 endif
 ifneq "$(findstring ADAFRUIT,$(BME_LIBRARY))" ""
CXXFLAGS += -DENVIRONMENTMONITOR_BME_LIBRARY=L_Adafruit
ARDUINO_LIBS += Adafruit_BME280 Wire Adafruit_Sensor
 endif
 ifneq "$(findstring BOSCH,$(BME_LIBRARY))" ""
CXXFLAGS += -DENVIRONMENTMONITOR_BME_LIBRARY=L_Bosch
ARDUINO_LIBS += Bme280BoschWrapper Wire
 endif
 ifneq "$(findstring GLENN,$(BME_LIBRARY))" ""
CXXFLAGS += -DENVIRONMENTMONITOR_BME_LIBRARY=L_Glenn
ARDUINO_LIBS += BME280 Wire
 endif
 ifneq "$(findstring SPARKFUN,$(BME_LIBRARY))" ""
CXXFLAGS += -DENVIRONMENTMONITOR_BME_LIBRARY=L_SparkFun
ARDUINO_LIBS += SparkFunBME280 Wire
 endif
 ifneq "$(findstring FORCED,$(BME_LIBRARY))" ""
CXXFLAGS += -DENVIRONMENTMONITOR_BME_LIBRARY=L_Forced
ARDUINO_LIBS += forcedClimate Wire
 endif
endif

ifneq "$(findstring RAIN,$(SENSORS))" ""
CXXFLAGS += -DENVIRONMENTMONITOR_SENSOR_RAIN=1
ARDUINO_LIBS += EnvironmentMonitor
endif

ifneq "$(findstring EnvironmentMonitor,$(ARDUINO_LIBS))" ""
CXXFLAGS += -DENVIRONMENTMONITOR=1
endif

include /usr/share/arduino/Arduino.mk
