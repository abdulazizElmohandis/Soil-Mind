/*
 * Plant Health Classification - REAL + SIMULATED SENSORS
 * 
 * Real Sensors:
 *   - DHT22: Temperature (GPIO4)
 *   - Capacitive Soil Moisture (GPIO36/VP)
 * 
 * Simulated (Potentiometers):
 *   - Nitrogen (GPIO32)
 *   - Phosphorus (GPIO33)
 *   - Potassium (GPIO34)
 *   - pH (GPIO35)
 * 
 * Wiring:
 *   DHT22:        VCC→3.3V, GND→GND, DATA→GPIO4 (10K pullup)
 *   Soil Sensor:  VCC→3.3V, GND→GND, AOUT→GPIO36
 *   POT_N:        VCC→3.3V, GND→GND, SIG→GPIO32
 *   POT_P:        VCC→3.3V, GND→GND, SIG→GPIO33
 *   POT_K:        VCC→3.3V, GND→GND, SIG→GPIO34
 *   POT_PH:       VCC→3.3V, GND→GND, SIG→GPIO35
 *   LED:          GPIO2 (built-in)
 */

#include <Chirale_TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <DHT.h>

#include "plant_health_model.h"

// =============================================================================
// PIN CONFIGURATION
// =============================================================================
// Real sensors
#define DHT_PIN             4     // DHT22 data
#define SOIL_MOISTURE_PIN   36    // Capacitive soil moisture (VP)

// Simulated sensors (potentiometers)
#define POT_N_PIN           32    // Nitrogen
#define POT_P_PIN           33    // Phosphorus
#define POT_K_PIN           34    // Potassium
#define POT_PH_PIN          35    // pH

#define LED_PIN             2     // Status LED

// =============================================================================
// SENSOR CONFIGURATION
// =============================================================================
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// Soil moisture calibration (adjust for YOUR sensor)
const int SOIL_DRY = 3200;
const int SOIL_WET = 1400;

// Mapping ranges (based on your training data distributions)
// N: 0-140, P: 0-140, K: 0-200, pH: 3.5-9.0
const float N_MIN = 0.0f,   N_MAX = 140.0f;
const float P_MIN = 0.0f,   P_MAX = 140.0f;
const float K_MIN = 0.0f,   K_MAX = 200.0f;
const float PH_MIN = 3.5f,  PH_MAX = 9.0f;
const float MOISTURE_MIN = 0.0f, MOISTURE_MAX = 100.0f;

// =============================================================================
// ML MODEL CONFIGURATION
// =============================================================================
constexpr int kTensorArenaSize = 16 * 1024;
alignas(16) uint8_t tensorArena[kTensorArenaSize];

const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* inputTensor = nullptr;
TfLiteTensor* outputTensor = nullptr;

bool modelReady = false;

// =============================================================================
// TIMING
// =============================================================================
const unsigned long READING_INTERVAL = 2000;  // 2s for DHT22
unsigned long lastReadingTime = 0;

// =============================================================================
// CURRENT READINGS
// =============================================================================
struct SensorData {
    float nitrogen;
    float phosphorus;
    float potassium;
    float pH;
    float moisture;
    float temperature;
    bool valid;
} currentData;

// =============================================================================
// SENSOR READING FUNCTIONS
// =============================================================================
float readPotAsValue(int pin, float minVal, float maxVal) {
    // Average multiple readings for stability
    long sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += analogRead(pin);
        delay(2);
    }
    int raw = sum / 5;
    return minVal + (raw / 4095.0f) * (maxVal - minVal);
}

float readSoilMoisture() {
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(SOIL_MOISTURE_PIN);
        delay(5);
    }
    int raw = sum / 10;
    
    // Capacitive: higher ADC = drier (invert mapping)
    float moisture = map(raw, SOIL_DRY, SOIL_WET, MOISTURE_MIN, MOISTURE_MAX);
    return constrain(moisture, MOISTURE_MIN, MOISTURE_MAX);
}

bool readAllSensors(SensorData &data) {
    // Read real sensors
    float humidity = dht.readHumidity();
    data.temperature = dht.readTemperature();
    
    if (isnan(humidity) || isnan(data.temperature)) {
        Serial.println("[ERROR] DHT22 read failed!");
        data.valid = false;
        return false;
    }
    
    data.moisture = readSoilMoisture();
    
    // Read simulated sensors (potentiometers)
    data.nitrogen = readPotAsValue(POT_N_PIN, N_MIN, N_MAX);
    data.phosphorus = readPotAsValue(POT_P_PIN, P_MIN, P_MAX);
    data.potassium = readPotAsValue(POT_K_PIN, K_MIN, K_MAX);
    data.pH = readPotAsValue(POT_PH_PIN, PH_MIN, PH_MAX);
    
    data.valid = true;
    return true;
}

// =============================================================================
// ML INFERENCE
// =============================================================================
int runInference(SensorData &data) {
    if (!modelReady || !data.valid) return -1;
    
    // Prepare features array: N, P, K, pH, Moisture, Temp
    float features[NUM_FEATURES] = {
        data.nitrogen,
        data.phosphorus,
        data.potassium,
        data.pH,
        data.moisture,
        data.temperature
    };
    
    // Normalize and quantize inputs
    for (int i = 0; i < NUM_FEATURES; i++) {
        float normalized = (features[i] - FEATURE_MEANS[i]) / FEATURE_STDS[i];
        int32_t quantized = (int32_t)(normalized / INPUT_SCALE) + INPUT_ZERO_POINT;
        inputTensor->data.int8[i] = (int8_t)constrain(quantized, -128, 127);
    }
    
    // Run inference
    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("[ERROR] Inference failed!");
        return -1;
    }
    
    // Find class with highest probability
    int maxIdx = 0;
    float maxProb = -999.0f;
    
    for (int i = 0; i < NUM_CLASSES; i++) {
        int8_t q = outputTensor->data.int8[i];
        float prob = (q - OUTPUT_ZERO_POINT) * OUTPUT_SCALE;
        prob = constrain(prob, 0.0f, 1.0f);
        
        if (prob > maxProb) {
            maxProb = prob;
            maxIdx = i;
        }
    }
    
    return maxIdx;
}

void printProbabilities() {
    Serial.println("\nClass Probabilities:");
    for (int i = 0; i < NUM_CLASSES; i++) {
        int8_t q = outputTensor->data.int8[i];
        float prob = (q - OUTPUT_ZERO_POINT) * OUTPUT_SCALE;
        prob = constrain(prob, 0.0f, 1.0f);
        
        Serial.print("  ");
        Serial.print(CLASS_LABELS[i]);
        Serial.print(": ");
        
        // Visual bar
        int bars = (int)(prob * 20);
        for (int j = 0; j < bars; j++) Serial.print("█");
        for (int j = bars; j < 20; j++) Serial.print("░");
        
        Serial.print(" ");
        Serial.print(prob * 100, 1);
        Serial.println("%");
    }
}

// =============================================================================
// DISPLAY
// =============================================================================
void printHeader() {
    Serial.println("\n");
    Serial.println("╔═══════════════════════════════════════════════════════════════╗");
    Serial.println("║           PLANT HEALTH MONITOR - REAL + SIMULATED             ║");
    Serial.println("╠═══════════════════════════════════════════════════════════════╣");
    Serial.println("║  REAL:      DHT22 (Temp) + Capacitive (Moisture)              ║");
    Serial.println("║  SIMULATED: POT_N(32) POT_P(33) POT_K(34) POT_pH(35)          ║");
    Serial.println("╚═══════════════════════════════════════════════════════════════╝");
    Serial.println();
}

void printSensorTable(SensorData &data) {
    Serial.println("┌─────────────────────────────────────────────────────────────┐");
    Serial.println("│                    SENSOR READINGS                          │");
    Serial.println("├──────────────┬──────────────┬───────────────────────────────┤");
    Serial.printf("│ Nitrogen     │ %6.1f mg/kg │ %-29s │\n", data.nitrogen, 
                  data.nitrogen < 20 ? "LOW (Deficient)" : 
                  data.nitrogen > 100 ? "HIGH" : "NORMAL");
    Serial.printf("│ Phosphorus   │ %6.1f mg/kg │ %-29s │\n", data.phosphorus,
                  data.phosphorus < 15 ? "LOW (Deficient)" : 
                  data.phosphorus > 80 ? "HIGH" : "NORMAL");
    Serial.printf("│ Potassium    │ %6.1f mg/kg │ %-29s │\n", data.potassium,
                  data.potassium < 15 ? "LOW (Deficient)" : 
                  data.potassium > 120 ? "HIGH" : "NORMAL");
    Serial.println("├──────────────┼──────────────┼───────────────────────────────┤");
    Serial.printf("│ pH Level     │ %6.2f       │ %-29s │\n", data.pH,
                  data.pH < 5.5 ? "ACIDIC (Too low)" : 
                  data.pH > 7.5 ? "ALKALINE (Too high)" : "OPTIMAL");
    Serial.println("├──────────────┼──────────────┼───────────────────────────────┤");
    Serial.printf("│ Moisture     │ %6.1f %%     │ %-29s │\n", data.moisture,
                  data.moisture < 20 ? "DRY (Water stress)" : 
                  data.moisture > 80 ? "WET (Over-watered)" : "GOOD");
    Serial.printf("│ Temperature  │ %6.1f °C    │ %-29s │\n", data.temperature,
                  data.temperature < 15 ? "COLD" : 
                  data.temperature > 35 ? "HOT" : "OPTIMAL");
    Serial.println("└──────────────┴──────────────┴───────────────────────────────┘");
}

void printDiagnosis(int prediction) {
    Serial.println();
    Serial.println("╔═══════════════════════════════════════════════════════════════╗");
    Serial.print("║  DIAGNOSIS: ");
    
    // Color-code status (works in some terminals)
    if (prediction == 0) {  // Assuming 0 = Healthy
        Serial.print("✓ ");
    } else {
        Serial.print("⚠ ");
    }
    
    // Pad to align
    char buf[50];
    sprintf(buf, "%-46s", CLASS_LABELS[prediction]);
    Serial.print(buf);
    Serial.println("║");
    Serial.println("╚═══════════════════════════════════════════════════════════════╝");
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    printHeader();
    
    // Configure pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(SOIL_MOISTURE_PIN, INPUT);
    pinMode(POT_N_PIN, INPUT);
    pinMode(POT_P_PIN, INPUT);
    pinMode(POT_K_PIN, INPUT);
    pinMode(POT_PH_PIN, INPUT);
    
    // Initialize DHT22
    Serial.println("[INIT] Starting DHT22...");
    dht.begin();
    delay(2000);
    
    // Initialize TFLite model
    Serial.println("[INIT] Loading TensorFlow Lite model...");
    
    tflModel = tflite::GetModel(MODEL_DATA);
    if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("[ERROR] Model schema mismatch!");
        while (1) delay(1000);
    }
    
    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter staticInterpreter(
        tflModel, resolver, tensorArena, kTensorArenaSize);
    interpreter = &staticInterpreter;
    
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("[ERROR] AllocateTensors failed!");
        while (1) delay(1000);
    }
    
    inputTensor = interpreter->input(0);
    outputTensor = interpreter->output(0);
    
    modelReady = true;
    Serial.printf("[INIT] Model ready! Arena: %d bytes\n", kTensorArenaSize);
    Serial.printf("[INIT] Free heap: %d bytes\n", ESP.getFreeHeap());
    
    // Print class labels
    Serial.println("\nHealth Classes:");
    for (int i = 0; i < NUM_CLASSES; i++) {
        Serial.printf("  %d: %s\n", i, CLASS_LABELS[i]);
    }
    Serial.println();
    Serial.println("Monitoring started... Adjust pots to simulate conditions!");
    Serial.println("════════════════════════════════════════════════════════════════\n");
}

// =============================================================================
// MAIN LOOP
// =============================================================================
void loop() {
    unsigned long now = millis();
    
    if (now - lastReadingTime >= READING_INTERVAL) {
        lastReadingTime = now;
        
        // Read all sensors
        if (readAllSensors(currentData)) {
            // Display readings
            printSensorTable(currentData);
            
            // Run inference
            int prediction = runInference(currentData);
            
            if (prediction >= 0) {
                printProbabilities();
                printDiagnosis(prediction);
                
                // LED feedback: ON if unhealthy
                digitalWrite(LED_PIN, prediction != 0 ? HIGH : LOW);
            }
        } else {
            Serial.println("[ERROR] Sensor read failed - check wiring!");
        }
        
        Serial.println("\n────────────────────────────────────────────────────────────────\n");
    }
    
    delay(10);
}