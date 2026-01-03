#ifndef MQTT_TEST_H
#define MQTT_TEST_H

/**
 * @brief Initialize MQTT Test module
 * 
 * Sets up WiFi and MQTT connections for testing
 */
void MQTTTest_Setup(void);

/**
 * @brief Main loop for MQTT Test module
 * 
 * Processes WiFi/MQTT state machines and publishes test messages periodically
 */
void MQTTTest_Loop(void);

#endif // MQTT_TEST_H
