// ============================================================================
// SENSOR CALIBRATION & TEST SKETCH
// Run this FIRST to verify all connections before using the full code
// ============================================================================

#include <DHT.h>

// Pin definitions (same as main code)
#define PIN_SOIL_MOISTURE   32
#define PIN_DHT             4
#define PIN_POT_N           33
#define PIN_POT_P           34
#define PIN_POT_K           35
#define PIN_POT_PH          36

#define DHT_TYPE DHT22

DHT dht(PIN_DHT, DHT_TYPE);

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n====================================");
    Serial.println("SENSOR CALIBRATION & TEST");
    Serial.println("====================================");
    Serial.println("\nThis sketch helps you:");
    Serial.println("1. Verify all connections");
    Serial.println("2. Calibrate soil moisture sensor");
    Serial.println("3. Test potentiometer ranges");
    Serial.println("\n");
    
    // Initialize DHT
    dht.begin();
    delay(1000);
    
    Serial.println("Pin Assignments:");
    Serial.println("  GPIO32 - Soil Moisture");
    Serial.println("  GPIO4  - DHT22 Data");
    Serial.println("  GPIO33 - Potentiometer N");
    Serial.println("  GPIO34 - Potentiometer P");
    Serial.println("  GPIO35 - Potentiometer K");
    Serial.println("  GPIO36 - Potentiometer pH");
    Serial.println("\n====================================\n");
}

void loop() {
    Serial.println("--- SENSOR READINGS ---\n");
    
    // ========== DHT22 ==========
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    Serial.print("DHT22 Temperature: ");
    if (isnan(temp)) {
        Serial.println("FAILED! Check wiring & pull-up resistor");
    } else {
        Serial.print(temp);
        Serial.println(" °C  ✓");
    }
    
    Serial.print("DHT22 Humidity:    ");
    if (isnan(humidity)) {
        Serial.println("FAILED!");
    } else {
        Serial.print(humidity);
        Serial.println(" %   ✓");
    }
    
    // ========== SOIL MOISTURE ==========
    int soilRaw = analogRead(PIN_SOIL_MOISTURE);
    Serial.print("\nSoil Moisture ADC: ");
    Serial.print(soilRaw);
    
    if (soilRaw == 0) {
        Serial.println("  ✗ Check VCC connection!");
    } else if (soilRaw == 4095) {
        Serial.println("  ✗ Check GND or signal wire!");
    } else if (soilRaw > 3000) {
        Serial.println("  (DRY - in air or dry soil)");
    } else if (soilRaw < 1500) {
        Serial.println("  (WET - moist soil or water)");
    } else {
        Serial.println("  (MEDIUM moisture)");
    }
    
    // ========== POTENTIOMETERS ==========
    Serial.println("\nPotentiometers (turn each to verify):");
    
    int potN = analogRead(PIN_POT_N);
    int potP = analogRead(PIN_POT_P);
    int potK = analogRead(PIN_POT_K);
    int potPH = analogRead(PIN_POT_PH);
    
    // Visual bar for each pot
    printPotBar("N  (GPIO33)", potN, 0, 150);
    printPotBar("P  (GPIO34)", potP, 0, 150);
    printPotBar("K  (GPIO35)", potK, 0, 200);
    printPotBar("pH (GPIO36)", potPH, 3.5, 9.0);
    
    // ========== MAPPED VALUES ==========
    Serial.println("\nMapped Sensor Values (for model input):");
    
    float N_val = map(potN, 0, 4095, 0, 15000) / 100.0;
    float P_val = map(potP, 0, 4095, 0, 15000) / 100.0;
    float K_val = map(potK, 0, 4095, 0, 20000) / 100.0;
    float pH_val = map(potPH, 0, 4095, 350, 900) / 100.0;
    float moisture_val = map(soilRaw, 3500, 1200, 0, 100);
    moisture_val = constrain(moisture_val, 0, 100);
    
    Serial.print("  N:        "); Serial.print(N_val, 1); Serial.println(" mg/kg");
    Serial.print("  P:        "); Serial.print(P_val, 1); Serial.println(" mg/kg");
    Serial.print("  K:        "); Serial.print(K_val, 1); Serial.println(" mg/kg");
    Serial.print("  pH:       "); Serial.println(pH_val, 2);
    Serial.print("  Moisture: "); Serial.print(moisture_val, 1); Serial.println(" %");
    Serial.print("  Temp:     "); 
    if (!isnan(temp)) {
        Serial.print(temp, 1); Serial.println(" °C");
    } else {
        Serial.println("-- °C");
    }
    
    // ========== CALIBRATION HINTS ==========
    Serial.println("\n--- CALIBRATION HINTS ---");
    Serial.println("Soil Moisture Sensor:");
    Serial.println("  1. Hold sensor in AIR, note ADC value (DRY)");
    Serial.println("  2. Put sensor in WATER, note ADC value (WET)");
    Serial.println("  3. Update MOISTURE_DRY and MOISTURE_WET in main code");
    Serial.println("\nPotentiometers:");
    Serial.println("  Turn fully LEFT  → should show ~0");
    Serial.println("  Turn fully RIGHT → should show ~4095");
    Serial.println("  If stuck at 0 or 4095, check wiring");
    
    Serial.println("\n====================================");
    Serial.println("Next reading in 3 seconds...");
    Serial.println("====================================\n\n");
    
    delay(3000);
}

// Helper function to print visual bar
void printPotBar(const char* name, int value, float minVal, float maxVal) {
    Serial.print("  ");
    Serial.print(name);
    Serial.print(": ");
    
    // Print raw ADC
    Serial.print(value);
    Serial.print(" [");
    
    // Visual bar (20 chars wide)
    int barLen = map(value, 0, 4095, 0, 20);
    for (int i = 0; i < 20; i++) {
        if (i < barLen) Serial.print("█");
        else Serial.print("░");
    }
    Serial.print("] ");
    
    // Mapped value
    float mapped = map(value, 0, 4095, minVal * 100, maxVal * 100) / 100.0;
    Serial.print(mapped, 1);
    
    // Status
    if (value == 0 || value == 4095) {
        Serial.println(" ✗");
    } else {
        Serial.println(" ✓");
    }
}
