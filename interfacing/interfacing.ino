#include <Arduino.h>

// ============================================================================
// TEST MODULES
// ============================================================================
// Uncomment the line below to enable MQTT testing
#include "src/App/MQTTTest/MQTTTest.h"

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // TODO: Add your application initialization here

  // Uncomment to test MQTT RTOS (requires MQTTTest.h to be included above)
  MQTTTest_Setup();
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  // TODO: Add your application main loop here

  // MQTT RTOS Test runs automatically in background task
  // No need to call MQTTTest_Loop() anymore

  // Add a small delay to prevent busy waiting
  delay(1000);

  // Optional: Add periodic status output or other main loop tasks
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime >= 10000) { // Every 10 seconds
    Serial.println("Main loop: MQTT RTOS test running in background...");
    lastStatusTime = millis();
  }
}

// ============================================================================
// CLEANUP (Optional - for testing)
// ============================================================================
// Uncomment the function below if you want to test deinitialization
/*
void cleanup() {
  MQTTTest_Deinit();
}
*/
