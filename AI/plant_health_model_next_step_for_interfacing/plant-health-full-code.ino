// ============================================================================
// PLANT HEALTH CLASSIFICATION - FULL SENSOR INTEGRATION
// Real: Soil Moisture + DHT22 Temperature
// Simulated: NPK + pH via Potentiometers
// ============================================================================

#include <Chirale_TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <DHT.h>

#include "plant_health_model.h"

// ============================================================================
// PIN DEFINITIONS
// ============================================================================
#define PIN_SOIL_MOISTURE   32    // Analog - Capacitive soil moisture sensor
#define PIN_DHT             4     // Digital - DHT22 data pin
#define PIN_POT_N           33    // Analog - Nitrogen potentiometer
#define PIN_POT_P           34    // Analog - Phosphorus potentiometer
#define PIN_POT_K           35    // Analog - Potassium potentiometer
#define PIN_POT_PH          36    // Analog - pH potentiometer

// ============================================================================
// SENSOR CONFIGURATION
// ============================================================================
#define DHT_TYPE DHT22            // Change to DHT11 if using DHT11

// Soil moisture calibration (adjust based on your sensor)
#define MOISTURE_DRY    3500      // ADC value when sensor is in air
#define MOISTURE_WET    1200      // ADC value when sensor is in water

// Potentiometer ranges (simulating real sensor ranges)
#define N_MIN   0.0f              // Nitrogen min (mg/kg)
#define N_MAX   150.0f            // Nitrogen max (mg/kg)
#define P_MIN   0.0f              // Phosphorus min (mg/kg)
#define P_MAX   150.0f            // Phosphorus max (mg/kg)
#define K_MIN   0.0f              // Potassium min (mg/kg)
#define K_MAX   200.0f            // Potassium max (mg/kg)
#define PH_MIN  3.5f              // pH min
#define PH_MAX  9.0f              // pH max

// Reading configuration
#define NUM_SAMPLES     10        // Samples for averaging
#define READ_INTERVAL   2000      // Interval between readings (ms)

// ============================================================================
// TFLITE SETUP
// ============================================================================
constexpr int kTensorArenaSize = 16 * 1024;
alignas(16) uint8_t tensorArena[kTensorArenaSize];

const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* inputTensor = nullptr;
TfLiteTensor* outputTensor = nullptr;

// ============================================================================
// SENSOR OBJECTS
// ============================================================================
DHT dht(PIN_DHT, DHT_TYPE);

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
unsigned long lastReadTime = 0;
float sensorValues[NUM_FEATURES];  // N, P, K, pH, moisture, temperature

// Feature names for display
const char* FEATURE_NAMES[] = {"N", "P", "K", "pH", "Moisture", "Temp"};
const char* FEATURE_UNITS[] = {"mg/kg", "mg/kg", "mg/kg", "", "%", "°C"};

// ============================================================================
// SENSOR READING FUNCTIONS
// ============================================================================

// Read analog pin with averaging
int readAnalogAvg(int pin, int samples = NUM_SAMPLES) {
    long sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(pin);
        delay(5);
    }
    return sum / samples;
}

// Map potentiometer to sensor range
float mapPotToRange(int pin, float minVal, float maxVal) {
    int adcValue = readAnalogAvg(pin);
    return map(adcValue, 0, 4095, minVal * 100, maxVal * 100) / 100.0f;
}

// Read soil moisture as percentage
float readSoilMoisture() {
    int adcValue = readAnalogAvg(PIN_SOIL_MOISTURE);
    // Invert and map: high ADC = dry, low ADC = wet
    float moisture = map(adcValue, MOISTURE_DRY, MOISTURE_WET, 0, 100);
    return constrain(moisture, 0.0f, 100.0f);
}

// Read all sensors
void readAllSensors() {
    Serial.println("\n--- Reading Sensors ---");
    
    // Read simulated NPK (potentiometers)
    sensorValues[0] = mapPotToRange(PIN_POT_N, N_MIN, N_MAX);   // N
    sensorValues[1] = mapPotToRange(PIN_POT_P, P_MIN, P_MAX);   // P
    sensorValues[2] = mapPotToRange(PIN_POT_K, K_MIN, K_MAX);   // K
    
    // Read simulated pH (potentiometer)
    sensorValues[3] = mapPotToRange(PIN_POT_PH, PH_MIN, PH_MAX); // pH
    
    // Read real soil moisture
    sensorValues[4] = readSoilMoisture();                        // Moisture
    
    // Read real temperature from DHT22
    float temp = dht.readTemperature();
    if (isnan(temp)) {
        Serial.println("WARNING: DHT read failed, using last value");
    } else {
        sensorValues[5] = temp;                                   // Temperature
    }
    
    // Print all readings
    Serial.println("\nSensor Readings:");
    for (int i = 0; i < NUM_FEATURES; i++) {
        Serial.print("  ");
        Serial.print(FEATURE_NAMES[i]);
        Serial.print(": ");
        Serial.print(sensorValues[i], 1);
        Serial.print(" ");
        Serial.println(FEATURE_UNITS[i]);
    }
}

// ============================================================================
// INFERENCE FUNCTION
// ============================================================================
int runInference(float* features) {
    // 1. Normalize and quantize inputs
    for (int i = 0; i < NUM_FEATURES; i++) {
        float normalized = (features[i] - FEATURE_MEANS[i]) / FEATURE_STDS[i];
        int32_t quantized = (int32_t)(normalized / INPUT_SCALE) + INPUT_ZERO_POINT;
        inputTensor->data.int8[i] = (int8_t)constrain(quantized, -128, 127);
    }
    
    // 2. Run inference
    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("Inference failed!");
        return -1;
    }
    
    // 3. Read and dequantize outputs
    Serial.println("\nPrediction Probabilities:");
    int maxIdx = 0;
    float maxProb = -999.0;
    
    for (int i = 0; i < NUM_CLASSES; i++) {
        int8_t q = outputTensor->data.int8[i];
        float prob = (q - OUTPUT_ZERO_POINT) * OUTPUT_SCALE;
        prob = constrain(prob, 0.0f, 1.0f);
        
        if (prob > maxProb) {
            maxProb = prob;
            maxIdx = i;
        }
        
        // Only print significant probabilities
        if (prob > 0.01f) {
            Serial.print("  ");
            Serial.print(CLASS_LABELS[i]);
            Serial.print(": ");
            Serial.print(prob * 100, 1);
            Serial.print("%");
            if (i == maxIdx) Serial.print(" ***");
            Serial.println();
        }
    }
    
    return maxIdx;
}

// ============================================================================
// DISPLAY RESULT
// ============================================================================
void displayResult(int prediction, float confidence) {
    Serial.println("\n╔════════════════════════════════════╗");
    Serial.println("║       PLANT HEALTH STATUS          ║");
    Serial.println("╠════════════════════════════════════╣");
    Serial.print("║  Status: ");
    
    // Pad the class label for alignment
    String status = CLASS_LABELS[prediction];
    Serial.print(status);
    for (int i = status.length(); i < 24; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.print("║  Confidence: ");
    Serial.print(confidence * 100, 1);
    Serial.println("%                 ║");
    Serial.println("╚════════════════════════════════════╝");
    
    // Print recommendation
    printRecommendation(prediction);
}

// ============================================================================
// RECOMMENDATIONS
// ============================================================================
void printRecommendation(int prediction) {
    Serial.println("\nRecommendation:");
    
    switch (prediction) {
        case 0:  // healthy
            Serial.println("  ✓ Plant is healthy! Continue current care.");
            break;
        case 1:  // nitrogen_deficiency
            Serial.println("  ! Low Nitrogen detected.");
            Serial.println("    → Add nitrogen-rich fertilizer");
            Serial.println("    → Consider compost or manure");
            break;
        case 2:  // ph_stress_acidic
            Serial.println("  ! Soil is too acidic (pH < 5.5)");
            Serial.println("    → Add lime to raise pH");
            Serial.println("    → Monitor pH regularly");
            break;
        case 3:  // ph_stress_alkaline
            Serial.println("  ! Soil is too alkaline (pH > 7.5)");
            Serial.println("    → Add sulfur to lower pH");
            Serial.println("    → Use acidic mulch");
            break;
        case 4:  // phosphorus_deficiency
            Serial.println("  ! Low Phosphorus detected.");
            Serial.println("    → Add phosphorus fertilizer");
            Serial.println("    → Bone meal is a good source");
            break;
        case 5:  // potassium_deficiency
            Serial.println("  ! Low Potassium detected.");
            Serial.println("    → Add potassium fertilizer");
            Serial.println("    → Wood ash can help");
            break;
        case 6:  // water_stress
            Serial.println("  ! Water stress detected.");
            Serial.println("    → Irrigate immediately");
            Serial.println("    → Check drainage system");
            break;
        default:
            Serial.println("  ? Unknown condition");
    }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════════╗");
    Serial.println("║     SOILMIND - Plant Health Monitor        ║");
    Serial.println("║     TinyML on ESP32                        ║");
    Serial.println("╚════════════════════════════════════════════╝");
    
    // Initialize DHT sensor
    Serial.println("\nInitializing DHT22...");
    dht.begin();
    delay(1000);
    
    // Test DHT
    float testTemp = dht.readTemperature();
    if (isnan(testTemp)) {
        Serial.println("WARNING: DHT22 not responding!");
        Serial.println("Check wiring and pull-up resistor.");
    } else {
        Serial.print("DHT22 OK. Current temp: ");
        Serial.print(testTemp);
        Serial.println("°C");
    }
    
    // Initialize default temperature value
    sensorValues[5] = 25.0;  // Default temperature
    
    Serial.print("\nFree heap: ");
    Serial.println(ESP.getFreeHeap());
    
    // Load TFLite model
    Serial.println("\nLoading TinyML model...");
    tflModel = tflite::GetModel(MODEL_DATA);
    
    if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("Model schema mismatch!");
        while (1) delay(1000);
    }
    
    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter staticInterpreter(
        tflModel, resolver, tensorArena, kTensorArenaSize);
    interpreter = &staticInterpreter;
    
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("AllocateTensors failed!");
        while (1) delay(1000);
    }
    
    inputTensor = interpreter->input(0);
    outputTensor = interpreter->output(0);
    
    Serial.println("Model loaded successfully!");
    Serial.print("Free heap after init: ");
    Serial.println(ESP.getFreeHeap());
    
    // Print sensor pin assignments
    Serial.println("\n--- Pin Assignments ---");
    Serial.println("Soil Moisture: GPIO32");
    Serial.println("DHT22 Temp:    GPIO4");
    Serial.println("Pot N:         GPIO33");
    Serial.println("Pot P:         GPIO34");
    Serial.println("Pot K:         GPIO35");
    Serial.println("Pot pH:        GPIO36");
    
    Serial.println("\n========================================");
    Serial.println("System ready! Reading every 2 seconds...");
    Serial.println("========================================\n");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastReadTime >= READ_INTERVAL) {
        lastReadTime = currentTime;
        
        // Read all sensors
        readAllSensors();
        
        // Run inference
        int prediction = runInference(sensorValues);
        
        if (prediction >= 0) {
            // Calculate confidence (max probability)
            int8_t maxQ = outputTensor->data.int8[prediction];
            float confidence = (maxQ - OUTPUT_ZERO_POINT) * OUTPUT_SCALE;
            confidence = constrain(confidence, 0.0f, 1.0f);
            
            // Display result
            displayResult(prediction, confidence);
        }
        
        Serial.println("\n────────────────────────────────────────");
    }
    
    // Handle serial commands for manual testing
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "read") {
            lastReadTime = 0;  // Force immediate reading
        } else if (cmd == "help") {
            Serial.println("\nCommands:");
            Serial.println("  read  - Force immediate sensor reading");
            Serial.println("  help  - Show this help");
        }
    }
}
