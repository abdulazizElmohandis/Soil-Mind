/*
 * Smart Irrigation Controller - POTENTIOMETER SIMULATION
 * 
 * Simulates soil moisture and temperature using 2 potentiometers.
 * ML model makes real-time irrigation decisions based on trends.
 * 
 * Wiring:
 *   POT 1 (Temperature):  VCC→3.3V, GND→GND, SIG→GPIO34
 *   POT 2 (Moisture):     VCC→3.3V, GND→GND, SIG→GPIO35
 *   RELAY:                Signal→GPIO26
 *   LED:                  Anode→GPIO2 (built-in)
 */

#include <ArduTFLite.h>
#include "irrigation_model.h"

// =============================================================================
// PIN CONFIGURATION
// =============================================================================
#define POT_TEMP_PIN        34    // Potentiometer for temperature
#define POT_MOISTURE_PIN    35    // Potentiometer for soil moisture
#define LED_PIN             2     // Built-in LED
#define RELAY_PIN           26    // Relay/Pump control

// =============================================================================
// SENSOR MAPPING (adjust to match your training data)
// =============================================================================
// Training data: Temp mean=29.6°C, Moisture mean=244
const float TEMP_MIN = 20.0f;      // Pot at 0 = 20°C
const float TEMP_MAX = 45.0f;      // Pot at max = 45°C
const float MOISTURE_MIN = 50.0f;  // Pot at 0 = very dry
const float MOISTURE_MAX = 450.0f; // Pot at max = very wet

// =============================================================================
// ML MODEL CONFIGURATION
// =============================================================================
constexpr int kTensorArenaSize = 8 * 1024;
alignas(16) uint8_t tensorArena[kTensorArenaSize];
bool modelReady = false;

const float IRRIGATION_THRESHOLD = 0.5f;

// =============================================================================
// TIMING
// =============================================================================
const unsigned long READING_INTERVAL = 1000;  // Read every 1 second
const unsigned long DISPLAY_INTERVAL = 500;   // Update display every 500ms
unsigned long lastReadingTime = 0;
unsigned long lastDisplayTime = 0;

// =============================================================================
// SENSOR HISTORY BUFFER
// =============================================================================
#define HISTORY_SIZE 4

struct SensorHistory {
    float temperature[HISTORY_SIZE];
    float soilmoisture[HISTORY_SIZE];
    int index;
    int count;
    
    void init() {
        index = 0;
        count = 0;
        for (int i = 0; i < HISTORY_SIZE; i++) {
            temperature[i] = 0;
            soilmoisture[i] = 0;
        }
    }
    
    void addReading(float temp, float moisture) {
        temperature[index] = temp;
        soilmoisture[index] = moisture;
        index = (index + 1) % HISTORY_SIZE;
        if (count < HISTORY_SIZE) count++;
    }
    
    float getTemp(int stepsAgo) {
        int idx = (index - 1 - stepsAgo + HISTORY_SIZE) % HISTORY_SIZE;
        return temperature[idx];
    }
    
    float getMoisture(int stepsAgo) {
        int idx = (index - 1 - stepsAgo + HISTORY_SIZE) % HISTORY_SIZE;
        return soilmoisture[idx];
    }
    
    float tempMean() {
        float sum = 0;
        for (int i = 0; i < count; i++) sum += temperature[i];
        return count > 0 ? sum / count : 0;
    }
    
    float moistureMean() {
        float sum = 0;
        for (int i = 0; i < count; i++) sum += soilmoisture[i];
        return count > 0 ? sum / count : 0;
    }
    
    float tempTrend() {
        if (count < 2) return 0;
        return getTemp(0) - getTemp(count - 1);
    }
    
    float moistureTrend() {
        if (count < 2) return 0;
        return getMoisture(0) - getMoisture(count - 1);
    }
    
    bool isReady() { return count >= HISTORY_SIZE; }
};

SensorHistory history;

// =============================================================================
// STATE
// =============================================================================
bool irrigationActive = false;
float lastProbability = 0;
float currentTemp = 0;
float currentMoisture = 0;

// =============================================================================
// SENSOR READING
// =============================================================================
float readTemperature() {
    int raw = analogRead(POT_TEMP_PIN);
    // Map 0-4095 to TEMP_MIN-TEMP_MAX
    return TEMP_MIN + (raw / 4095.0f) * (TEMP_MAX - TEMP_MIN);
}

float readMoisture() {
    int raw = analogRead(POT_MOISTURE_PIN);
    // Map 0-4095 to MOISTURE_MIN-MOISTURE_MAX
    return MOISTURE_MIN + (raw / 4095.0f) * (MOISTURE_MAX - MOISTURE_MIN);
}

// =============================================================================
// ML INFERENCE
// =============================================================================
float runMLInference() {
    if (!modelReady) return -1.0f;
    
    // Prepare 8 features
    float features[NUM_FEATURES];
    features[0] = history.getTemp(0);           // temperature
    features[1] = history.getMoisture(0);       // soilmoisture
    features[2] = history.tempMean();           // temperature_mean
    features[3] = history.moistureMean();       // soilmoisture_mean
    features[4] = history.tempTrend();          // temperature_trend
    features[5] = history.moistureTrend();      // soilmoisture_trend
    features[6] = history.getMoisture(1);       // soilmoisture_lag_1
    features[7] = history.getMoisture(2);       // soilmoisture_lag_2
    
    // Standardize using scaler params from irrigation_model.h
    float scaled[NUM_FEATURES];
    for (int i = 0; i < NUM_FEATURES; i++) {
        scaled[i] = (features[i] - featureMeans[i]) / featureStds[i];
    }
    
    // Set inputs
    for (int i = 0; i < NUM_FEATURES; i++) {
        modelSetInput(scaled[i], i);
    }
    
    // Run inference
    if (!modelRunInference()) return -1.0f;
    
    return modelGetOutput(0);
}

// =============================================================================
// IRRIGATION CONTROL
// =============================================================================
void controlIrrigation(bool shouldIrrigate) {
    if (shouldIrrigate && !irrigationActive) {
        irrigationActive = true;
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        Serial.println("\n*** PUMP ON ***\n");
    } else if (!shouldIrrigate && irrigationActive) {
        irrigationActive = false;
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        Serial.println("\n*** PUMP OFF ***\n");
    }
}

// =============================================================================
// DISPLAY
// =============================================================================
void printHeader() {
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║      SMART IRRIGATION - POTENTIOMETER SIMULATION           ║");
    Serial.println("║                                                            ║");
    Serial.println("║  Twist the pots to simulate different conditions!          ║");
    Serial.println("║  POT1 (GPIO34) = Temperature    POT2 (GPIO35) = Moisture   ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.printf("Model: %s | Arena: %d bytes\n", 
                  modelReady ? "READY" : "FAILED", kTensorArenaSize);
    Serial.printf("Temp Range: %.0f-%.0f°C | Moisture Range: %.0f-%.0f\n",
                  TEMP_MIN, TEMP_MAX, MOISTURE_MIN, MOISTURE_MAX);
    Serial.println();
    Serial.println("Temp    Moist   T_Trend  M_Trend  Prob   Decision   Pump");
    Serial.println("------  ------  -------  -------  -----  ---------  ----");
}

void printStatus() {
    float tTrend = history.tempTrend();
    float mTrend = history.moistureTrend();
    
    char tTrendStr[10], mTrendStr[10];
    if (history.count >= 2) {
        sprintf(tTrendStr, "%+.1f", tTrend);
        sprintf(mTrendStr, "%+.0f", mTrend);
    } else {
        strcpy(tTrendStr, " N/A");
        strcpy(mTrendStr, " N/A");
    }
    
    const char* decision;
    if (!history.isReady()) {
        decision = "WAIT";
    } else if (lastProbability >= IRRIGATION_THRESHOLD) {
        decision = "IRRIGATE";
    } else {
        decision = "NO";
    }
    
    Serial.printf("%.1f°C  %.0f    %s    %s   ",
                  currentTemp, currentMoisture, tTrendStr, mTrendStr);
    
    if (history.isReady()) {
        Serial.printf("%.2f   %-9s  %s\n", 
                      lastProbability, decision, 
                      irrigationActive ? "ON" : "OFF");
    } else {
        Serial.printf(" --    %-9s  %s\n", decision, 
                      irrigationActive ? "ON" : "OFF");
    }
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Configure pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(POT_TEMP_PIN, INPUT);
    pinMode(POT_MOISTURE_PIN, INPUT);
    
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize model
    Serial.println("\n[INIT] Loading TensorFlow Lite model...");
    modelReady = modelInit(irrigation_model, tensorArena, kTensorArenaSize);
    
    if (modelReady) {
        Serial.printf("[INIT] Model loaded! Size: %d bytes\n", irrigation_model_len);
    } else {
        Serial.println("[ERROR] Model failed to load!");
    }
    
    history.init();
    printHeader();
}

// =============================================================================
// MAIN LOOP
// =============================================================================
void loop() {
    unsigned long now = millis();
    
    // Take new reading every READING_INTERVAL
    if (now - lastReadingTime >= READING_INTERVAL) {
        lastReadingTime = now;
        
        // Read potentiometers
        currentTemp = readTemperature();
        currentMoisture = readMoisture();
        
        // Add to history
        history.addReading(currentTemp, currentMoisture);
        
        // Run ML inference if history is ready
        if (history.isReady() && modelReady) {
            lastProbability = runMLInference();
            
            // Control irrigation
            bool shouldIrrigate = (lastProbability >= IRRIGATION_THRESHOLD);
            controlIrrigation(shouldIrrigate);
        }
        
        // Print status
        printStatus();
    }
    
    delay(10);
}