#ifndef MQTT_TEST_H
#define MQTT_TEST_H

/**
 * @brief Initialize MQTT Test module
 *
 * Sets up WiFi and creates RTOS task for MQTT testing
 */
void MQTTTest_Setup(void);

/**
 * @brief Deinitialize MQTT Test module
 *
 * Cleans up RTOS task and resources
 */
void MQTTTest_Deinit(void);

#endif // MQTT_TEST_H
