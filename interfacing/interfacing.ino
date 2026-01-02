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
  
  // Uncomment to test MQTT (requires MQTTTest.h to be included above)
  MQTTTest_Setup();
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  // TODO: Add your application main loop here
  
  // Uncomment to test MQTT (requires MQTTTest.h to be included above)
  MQTTTest_Loop();
}
