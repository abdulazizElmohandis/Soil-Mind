/*
 * Smart Irrigation Controller - REAL SENSORS
 * 
 * Uses DHT22 for temperature/humidity and capacitive soil moisture sensor.
 * ML model makes real-time irrigation decisions based on trends.
 * 
 * Wiring:
 *   DHT22:           VCC→3.3V, GND→GND, DATA→GPIO4 (with 10K pullup)
 *   Soil Moisture:   VCC→3.3V, GND→GND, AOUT→GPIO35
 *   RELAY:           Signal→GPIO26
 *   LED:             Anode→GPIO2 (built-in)
 */

#include <ArduTFLite.h>
#include <DHT.h>
#include "irrigation_model.h"

// =============================================================================
// PIN CONFIGURATION
// =============================================================================
#define DHT_PIN             4     // DHT22 data pin
#define SOIL_MOISTURE_PIN   35    // Capacitive soil moisture sensor (ADC)
#define LED_PIN             2     // Built-in LED
#define RELAY_PIN           26    // Relay/Pump control

// =============================================================================
// SENSOR CONFIGURATION
// =============================================================================
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// Soil moisture calibration (measure these for YOUR sensor!)
// Capacitive sensor: higher value = drier soil (inverse of resistive)
const int SOIL_DRY = 3200;      // ADC reading in dry air
const int SOIL_WET = 1400;      // ADC reading in water
// Map to training data range (your model expects ~50-450)
const float MOISTURE_MIN = 50.0f;
const float MOISTURE_MAX = 450.0f;

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
const unsigned long READING_INTERVAL = 2000;  // DHT22 needs 2s between reads
const unsigned long DISPLAY_INTERVAL = 500;
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
float currentHumidity = 0;  // Bonus: DHT22 gives humidity too!
float currentMoisture = 0;
bool sensorError = false;

// =============================================================================
// SENSOR READING
// =============================================================================
bool readDHT22(float &temp, float &humidity) {
    humidity = dht.readHumidity();
    temp = dht.readTemperature();
    
    // Check for read failures
    if (isnan(humidity) || isnan(temp)) {
        Serial.println("[ERROR] DHT22 read failed!");
        return false;
    }
    return true;
}

float readSoilMoisture() {
    // Take multiple readings and average for stability
    long sum = 0;
    const int samples = 10;
    
    for (int i = 0; i < samples; i++) {
        sum += analogRead(SOIL_MOISTURE_PIN);
        delay(10);
    }
    int raw = sum / samples;
    
    // Map ADC reading to moisture value (matching training data range)
    // Capacitive: higher ADC = drier, so we invert
    float moisture = map(raw, SOIL_DRY, SOIL_WET, MOISTURE_MIN, MOISTURE_MAX);
    
    // Constrain to valid range
    moisture = constrain(moisture, MOISTURE_MIN, MOISTURE_MAX);
    
    return moisture;
}

// =============================================================================
// ML INFERENCE
// =============================================================================
float runMLInference() {
    if (!modelReady) return -1.0f;
    
    float features[NUM_FEATURES];
    features[0] = history.getTemp(0);
    features[1] = history.getMoisture(0);
    features[2] = history.tempMean();
    features[3] = history.moistureMean();
    features[4] = history.tempTrend();
    features[5] = history.moistureTrend();
    features[6] = history.getMoisture(1);
    features[7] = history.getMoisture(2);
    
    float scaled[NUM_FEATURES];
    for (int i = 0; i < NUM_FEATURES; i++) {
        scaled[i] = (features[i] - featureMeans[i]) / featureStds[i];
    }
    
    for (int i = 0; i < NUM_FEATURES; i++) {
        modelSetInput(scaled[i], i);
    }
    
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
    Serial.println("║         SMART IRRIGATION - REAL SENSORS (DHT22)            ║");
    Serial.println("║                                                            ║");
    Serial.println("║  DHT22 (GPIO4) = Temp/Humidity                             ║");
    Serial.println("║  Capacitive Soil Sensor (GPIO35) = Soil Moisture           ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.printf("Model: %s | Arena: %d bytes\n", 
                  modelReady ? "READY" : "FAILED", kTensorArenaSize);
    Serial.printf("Soil Calibration: DRY=%d, WET=%d\n", SOIL_DRY, SOIL_WET);
    Serial.println();
    Serial.println("Temp    Humid   Soil    T_Trend  M_Trend  Prob   Decision   Pump");
    Serial.println("------  ------  ------  -------  -------  -----  ---------  ----");
}

void printStatus() {
    if (sensorError) {
        Serial.println("SENSOR ERROR - Check wiring!");
        return;
    }
    
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
    
    Serial.printf("%.1f°C  %.0f%%   %.0f    %s    %s   ",
                  currentTemp, currentHumidity, currentMoisture, 
                  tTrendStr, mTrendStr);
    
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
    pinMode(SOIL_MOISTURE_PIN, INPUT);
    
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize DHT22
    Serial.println("\n[INIT] Starting DHT22 sensor...");
    dht.begin();
    delay(2000);  // DHT22 needs time to stabilize
    
    // Initialize model
    Serial.println("[INIT] Loading TensorFlow Lite model...");
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
    
    if (now - lastReadingTime >= READING_INTERVAL) {
        lastReadingTime = now;
        
        // Read real sensors
        sensorError = !readDHT22(currentTemp, currentHumidity);
        
        if (!sensorError) {
            currentMoisture = readSoilMoisture();
            
            // Add to history
            history.addReading(currentTemp, currentMoisture);
            
            // Run ML inference if history is ready
            if (history.isReady() && modelReady) {
                lastProbability = runMLInference();
                bool shouldIrrigate = (lastProbability >= IRRIGATION_THRESHOLD);
                controlIrrigation(shouldIrrigate);
            }
        }
        
        printStatus();
    }
    
    delay(10);
}