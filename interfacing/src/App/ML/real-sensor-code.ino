/*
 * SoilMind - Smart Irrigation with Real Sensors
 * 
 * Hardware:
 *   - Capacitive Soil Moisture Sensor → GPIO34
 *   - DHT22 Temperature/Humidity     → GPIO4
 *   - Relay Module                   → GPIO26
 *   - Status LED                     → GPIO2 (built-in)
 * 
 * ML Model: 8 trend-based features for intelligent irrigation decisions
 */

#include <ArduTFLite.h>
#include <DHT.h>
#include "irrigation_model.h"

// =============================================================================
// PIN CONFIGURATION
// =============================================================================
#define SOIL_MOISTURE_PIN   34
#define DHT_PIN             4
#define RELAY_PIN           26
#define LED_PIN             2

// =============================================================================
// SENSOR CONFIGURATION
// =============================================================================
#define DHT_TYPE            DHT22

// *** CALIBRATION VALUES - UPDATE THESE AFTER CALIBRATION! ***
const int DRY_VALUE = 3400;    // ADC reading in dry air
const int WET_VALUE = 1600;    // ADC reading in water

// Mapping to training data range
const float MOISTURE_MIN = 50.0f;
const float MOISTURE_MAX = 450.0f;

// Temperature offset (if needed after verification)
const float TEMP_OFFSET = 0.0f;

// =============================================================================
// ML MODEL CONFIGURATION
// =============================================================================
constexpr int kTensorArenaSize = 8 * 1024;
alignas(16) uint8_t tensorArena[kTensorArenaSize];
bool modelReady = false;

const float IRRIGATION_THRESHOLD = 0.5f;

// =============================================================================
// TIMING CONFIGURATION
// =============================================================================
const unsigned long READING_INTERVAL = 30000;   // 30 seconds between readings
const unsigned long MIN_PUMP_TIME = 10000;      // Minimum pump ON time (10s)
const unsigned long PUMP_COOLDOWN = 60000;      // Cooldown after pump OFF (60s)

unsigned long lastReadingTime = 0;
unsigned long pumpStartTime = 0;
unsigned long pumpStopTime = 0;

// =============================================================================
// SENSOR OBJECTS
// =============================================================================
DHT dht(DHT_PIN, DHT_TYPE);

// =============================================================================
// HISTORY BUFFER
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
// STATE VARIABLES
// =============================================================================
bool irrigationActive = false;
bool sensorsOK = true;
float lastProbability = 0;
float currentTemp = 0;
float currentHumidity = 0;
float currentMoisture = 0;
int rawMoistureADC = 0;

// =============================================================================
// SENSOR READING FUNCTIONS
// =============================================================================
float readSoilMoisture() {
    // Average multiple readings for stability
    long sum = 0;
    const int samples = 10;
    
    for (int i = 0; i < samples; i++) {
        sum += analogRead(SOIL_MOISTURE_PIN);
        delay(10);
    }
    rawMoistureADC = sum / samples;
    
    // Map to training data range (inverted - lower ADC = wetter)
    float normalized = (float)(DRY_VALUE - rawMoistureADC) / (DRY_VALUE - WET_VALUE);
    normalized = constrain(normalized, 0.0f, 1.0f);
    
    return MOISTURE_MIN + normalized * (MOISTURE_MAX - MOISTURE_MIN);
}

bool readDHT(float &temp, float &humidity) {
    temp = dht.readTemperature() + TEMP_OFFSET;
    humidity = dht.readHumidity();
    
    if (isnan(temp) || isnan(humidity)) {
        return false;
    }
    return true;
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
    
    // Standardize using scaler params
    float scaled[NUM_FEATURES];
    for (int i = 0; i < NUM_FEATURES; i++) {
        scaled[i] = (features[i] - featureMeans[i]) / featureStds[i];
    }
    
    // Set inputs and run
    for (int i = 0; i < NUM_FEATURES; i++) {
        modelSetInput(scaled[i], i);
    }
    
    if (!modelRunInference()) return -1.0f;
    
    return modelGetOutput(0);
}

// =============================================================================
// IRRIGATION CONTROL WITH SAFETY
// =============================================================================
void controlIrrigation(float probability) {
    unsigned long now = millis();
    
    // FAILSAFE: Sensors or model failed
    if (!sensorsOK || !modelReady || probability < 0) {
        if (irrigationActive) {
            digitalWrite(RELAY_PIN, LOW);
            digitalWrite(LED_PIN, LOW);
            irrigationActive = false;
            pumpStopTime = now;
            Serial.println("⚠️  SAFETY: Pump OFF (sensor/model error)");
        }
        return;
    }
    
    bool shouldIrrigate = (probability >= IRRIGATION_THRESHOLD);
    
    // Turn ON pump
    if (shouldIrrigate && !irrigationActive) {
        // Check cooldown period
        if (now - pumpStopTime < PUMP_COOLDOWN && pumpStopTime > 0) {
            Serial.printf("⏳ Cooldown: %lus remaining\n", 
                         (PUMP_COOLDOWN - (now - pumpStopTime)) / 1000);
            return;
        }
        
        irrigationActive = true;
        pumpStartTime = now;
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        Serial.println("\n💧 PUMP ON - Irrigation started\n");
    }
    
    // Turn OFF pump
    if (!shouldIrrigate && irrigationActive) {
        // Ensure minimum pump runtime
        if (now - pumpStartTime < MIN_PUMP_TIME) {
            Serial.printf("⏳ Min runtime: %lus remaining\n",
                         (MIN_PUMP_TIME - (now - pumpStartTime)) / 1000);
            return;
        }
        
        irrigationActive = false;
        pumpStopTime = now;
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        Serial.println("\n✅ PUMP OFF - Irrigation complete\n");
    }
}

// =============================================================================
// DISPLAY FUNCTIONS
// =============================================================================
void printHeader() {
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║           SOILMIND - SMART IRRIGATION SYSTEM               ║");
    Serial.println("║                   Real Sensor Mode                         ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.printf("ML Model: %s | Size: %d bytes\n", 
                  modelReady ? "✓ READY" : "✗ FAILED", irrigation_model_len);
    Serial.printf("Reading Interval: %lus | History Size: %d\n", 
                  READING_INTERVAL/1000, HISTORY_SIZE);
    Serial.printf("Calibration: DRY=%d, WET=%d\n", DRY_VALUE, WET_VALUE);
    Serial.println();
}

void printTableHeader() {
    Serial.println("┌────────┬────────┬──────────┬──────────┬───────────┬────────┬──────────┐");
    Serial.println("│ Temp°C │ Humid% │ Moisture │ Raw ADC  │ M_Trend   │ Prob   │ Decision │");
    Serial.println("├────────┼────────┼──────────┼──────────┼───────────┼────────┼──────────┤");
}

void printReading() {
    float mTrend = history.moistureTrend();
    
    char trendStr[12];
    if (history.count >= 2) {
        sprintf(trendStr, "%+.0f", mTrend);
    } else {
        strcpy(trendStr, "N/A");
    }
    
    const char* decision;
    if (!sensorsOK) {
        decision = "ERROR";
    } else if (!history.isReady()) {
        decision = "WAIT";
    } else if (lastProbability >= IRRIGATION_THRESHOLD) {
        decision = irrigationActive ? "PUMPING" : "IRRIGATE";
    } else {
        decision = "OK";
    }
    
    Serial.printf("│ %5.1f  │ %5.1f  │  %5.0f   │  %5d   │ %9s │ ",
                  currentTemp, currentHumidity, currentMoisture, 
                  rawMoistureADC, trendStr);
    
    if (history.isReady() && sensorsOK) {
        Serial.printf("%5.2f  │ %-8s │\n", lastProbability, decision);
    } else {
        Serial.printf("  --   │ %-8s │\n", decision);
    }
}

void printCalibrationMode() {
    Serial.println("\n📊 CALIBRATION MODE");
    Serial.println("════════════════════════════════════════");
    Serial.printf("Raw ADC Value: %d\n", rawMoistureADC);
    Serial.println("1. Note this value in DRY AIR → DRY_VALUE");
    Serial.println("2. Submerge in water, note value → WET_VALUE");
    Serial.println("════════════════════════════════════════\n");
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize pins
    pinMode(SOIL_MOISTURE_PIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize DHT22
    dht.begin();
    Serial.println("[INIT] DHT22 sensor initialized");
    
    // Initialize ML model
    Serial.println("[INIT] Loading TensorFlow Lite model...");
    modelReady = modelInit(irrigation_model, tensorArena, kTensorArenaSize);
    
    if (!modelReady) {
        Serial.println("[ERROR] Model failed to load!");
    }
    
    // Initialize history
    history.init();
    
    // Wait for DHT to stabilize
    Serial.println("[INIT] Waiting for sensors to stabilize...");
    delay(2000);
    
    printHeader();
    printTableHeader();
}

// =============================================================================
// MAIN LOOP
// =============================================================================
void loop() {
    unsigned long now = millis();
    
    if (now - lastReadingTime >= READING_INTERVAL) {
        lastReadingTime = now;
        
        // Read sensors
        currentMoisture = readSoilMoisture();
        sensorsOK = readDHT(currentTemp, currentHumidity);
        
        if (!sensorsOK) {
            Serial.println("│  ERR   │  ERR   │         │          │           │   --   │  ERROR   │");
            controlIrrigation(-1);  // Safety: turn off pump
            return;
        }
        
        // Add to history
        history.addReading(currentTemp, currentMoisture);
        
        // Run ML inference if ready
        if (history.isReady() && modelReady) {
            lastProbability = runMLInference();
            controlIrrigation(lastProbability);
        }
        
        // Print current reading
        printReading();
        
        // Print table separator every 10 readings
        static int readingCount = 0;
        if (++readingCount % 10 == 0) {
            Serial.println("├────────┼────────┼──────────┼──────────┼───────────┼────────┼──────────┤");
        }
    }
    
    delay(100);
}

// =============================================================================
// OPTIONAL: Serial Commands for Calibration
// =============================================================================
void serialEvent() {
    while (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case 'c':  // Calibration mode
            case 'C':
                printCalibrationMode();
                break;
                
            case 'r':  // Reset history
            case 'R':
                history.init();
                Serial.println("🔄 History reset");
                break;
                
            case 'p':  // Manual pump toggle
            case 'P':
                irrigationActive = !irrigationActive;
                digitalWrite(RELAY_PIN, irrigationActive ? HIGH : LOW);
                digitalWrite(LED_PIN, irrigationActive ? HIGH : LOW);
                Serial.printf("🔧 Manual pump: %s\n", irrigationActive ? "ON" : "OFF");
                break;
                
            case 'h':  // Help
            case 'H':
                Serial.println("\n📖 Commands:");
                Serial.println("  c - Calibration mode (show raw ADC)");
                Serial.println("  r - Reset sensor history");
                Serial.println("  p - Toggle pump manually");
                Serial.println("  h - Show this help\n");
                break;
        }
    }
}
