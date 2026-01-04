#ifndef APP_CFG_H
#define APP_CFG_H 

//General Definitions
#define STD_ON 1
#define STD_OFF 0
#define WIFI_MODULE    1
#define BLE     2
#define _4G    3
#define Zero 0
#define ADC_MAX 4095


//Module definitions
#define GPIO_ENABLED               STD_OFF
#define COMMUNICATION_MODULE       WIFI_MODULE
#define SENSORH_ENABLED            STD_OFF
#define ADC_ENABLED                STD_ON
#define POT_ENABLED                STD_ON
#define SOILMOISTURE_ENABLED       STD_ON
#define LM35_ENABLED               STD_OFF
#define PWM_ENABLED                STD_OFF
#define PUMP_ENABLED               STD_ON
#define WIFI_ENABLED               STD_ON
#define MQTT_ENABLED               STD_ON
#define DIMALARM_ENABLED           STD_OFF
#define UART_ENABLED               STD_ON
#define ChatApp_ENABLED            STD_ON
#define DHT11_ENABLED              STD_ON
#define Nitrogen_ENABLED           STD_ON
#define Phosphorus_ENABLED         STD_ON
#define Potassium_ENABLED          STD_ON
//Debug Definitions
#define GPIO_DEBUG                 STD_OFF
#define SENSORH_DEBUG              STD_OFF
#define ADC_DEBUG                  STD_OFF
#define POT_DEBUG                  STD_OFF
#define SOILMOISTURE_DEBUG         STD_OFF
#define LM35_DEBUG                 STD_OFF
#define PWM_DEBUG                  STD_OFF
#define PUMP_DEBUG                 STD_OFF
#define WIFI_DEBUG                 STD_ON
#define MQTT_DEBUG                 STD_ON
#define DIMALARM_DEBUG             STD_OFF
#define UART_DEBUG                 STD_ON
#define ChatApp_DEBUG              STD_ON
#define DHT11_DEBUG                STD_ON
#define Nitrogen_DEBUG             STD_ON
#define Phosphorus_DEBUG           STD_ON
#define Potassium_DEBUG            STD_ON

//Pin Configuration
#define POT_PIN             34
#define SOILMOISTURE_PIN    35
#define LM35_PIN            0
#define POT_RESOLUTION      12
#define SOILMOISTURE_RESOLUTION 12
#define LM35_RESOLUTION     10
#define ALARM_LOW_LED       16
#define ALARM_HIGH_LED      17
#define DIMER_LED           9
#define PUMP_PIN            26
#define Nitrogen_SENSOR_PIN 36
#define Nitrogen_RESOLUTION 12
#define Phosphorus_SENSOR_PIN 39
#define Phosphorus_RESOLUTION 12
#define Potassium_SENSOR_PIN 33
#define Potassium_RESOLUTION 12

//General Configurations
#define MAX_TEMP_RANGE                   150.0// Maximum temperature range for LM35 sensor in Celsius
#define ALARM_LOW_THRESHOLD_PERCENTAGE   20.0 // Low Voltage threshold for DimAlarm in Celsius
#define ALARM_HIGH_THRESHOLD_PERCENTAGE  80.0 // High Voltage threshold for DimAlarm in Celsius
#define ADC_MAX_VALUE                    4095 // 12-bit ADC
#define PUMP_PWM_FREQUENCY                20000 // 20 kHz PWM frequency for pump

//UART1 Configuration
#define UART1_BAUD_RATE 115200
#define UART1_TX_PIN    17
#define UART1_RX_PIN    16
#define UART1_FRAME_CFG SERIAL_8N1

//WiFi Configuration
#define WIFI_SSID                  "MES"
#define WIFI_PASSWORD              "@MES12345@"
#define WIFI_RECONNECT_INTERVAL_MS 5000
#define WIFI_CONNECT_TIMEOUT_MS    15000

//MQTT Configuration
#define MQTT_BROKER                "10.17.84.102"
#define MQTT_PORT                   1883
#define MQTT_USERNAME               ""  // Leave empty "" if no authentication needed
#define MQTT_PASSWORD               ""  // Leave empty "" if no authentication needed
#define MQTT_TOPIC_TELEMETRY        "farm/site1/nodeA/telemetry"
#define MQTT_TOPIC_IRRIGATION_DECISION "farm/site1/nodeA/decision"
#define MQTT_TOPIC_PUMP_CONTROL     "farm/site1/nodeB/status"


// QUEUE Configuration
#define Moisture_QUEUE_SIZE                 10
// DHT 11 Configuration
#define DHT11_1_PIN   5
#define DHT22_1_PIN   5
#define MAX_SENSORS_DHT 1  
#define Temperature_QUEUE_SIZE              10
#define Humidity_QUEUE_SIZE                 10


// Nitrogen Sensor Configuration

#define Nitrogen_QUEUE_SIZE                10

#define NITROGEN_MAX  200

// phosphorus Sensor Configuration

#define Phosphorus_QUEUE_SIZE                10

#define PHOSPHORUS_MAX  200

// Potassium Sensor Configuration

#define Potassium_QUEUE_SIZE                10
#define POTASSIUM_MAX  200
#endif