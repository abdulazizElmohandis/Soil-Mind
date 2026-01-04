#include "ML.h"
#include "../SoilMoisture/SoilMoisture.h"
#include "../DHT/DHT11.h"

// TensorFlow Lite Micro includes
#include <Chirale_TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

// TensorFlow Lite globals
uint8_t tensorArena[kTensorArenaSize];
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
bool modelReady = false;

// Sensor history
SensorHistory history;

// ML inference implementation
bool ML_Init() {
    Serial.println("[ML] Initializing TensorFlow Lite Micro model...");

    // Map the data into a model
    model = tflite::GetModel(irrigation_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("[ML ERROR] Model schema version mismatch!");
        return false;
    }

    // Create an AllOpsResolver to load all available operations
    static tflite::AllOpsResolver resolver;

    // Build an interpreter to run the model
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensorArena, kTensorArenaSize);
    interpreter = &static_interpreter;

    // Allocate memory from the tensorArena for the model's tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        Serial.println("[ML ERROR] Failed to allocate tensors!");
        return false;
    }

    // Get pointers to the model's input and output tensors
    input = interpreter->input(0);
    output = interpreter->output(0);

    modelReady = true;

    // Initialize history
    history.init();

    Serial.printf("[ML] Model loaded successfully (%d bytes)\n", irrigation_model_len);
    return true;
}

float ML_RunInference() {
    if (!modelReady || !interpreter) {
        Serial.println("[ML ERROR] Model not ready!");
        return -1.0f;
    }

    if (!history.isReady()) {
        Serial.println("[ML] Not enough history data for inference");
        return -1.0f;
    }

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

    // Copy the scaled features to the input tensor
    for (int i = 0; i < NUM_FEATURES; i++) {
        input->data.f[i] = scaled[i];
    }

    // Run inference
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        Serial.println("[ML ERROR] Inference failed!");
        return -1.0f;
    }

    // Get the output probability
    float probability = output->data.f[0];

    Serial.printf("[ML] Inference result: %.4f\n", probability);
    return probability;
}

Decision_t ML_GetDecision(float probability) {
    if (probability < 0) {
        return DECISION_CHECK_SYSTEM;  // Error case
    }

    if (probability >= IRRIGATION_THRESHOLD) {
        return DECISION_IRRIGATE;
    } else {
        return DECISION_NO_IRRIGATION;
    }
}

bool ML_GetSensorData(float *temperature, float *humidity, uint8_t *soilMoisture) {
    // Get data from queues
    SoilMoisture_getMoisture(soilMoisture);
    DHT11_GetTemperature(temperature);
    DHT11_GetHumidity(humidity);

    // Check if data is valid (not default 0)
    if (*temperature == 0.0f && *humidity == 0.0f && *soilMoisture == 0) {
        return false;  // Queues empty
    }

    return true;
}

void ML_UpdateHistory() {
    float temperature, humidity;
    uint8_t soilMoisture;

    if (ML_GetSensorData(&temperature, &humidity, &soilMoisture)) {
        // Scale soil moisture to training range
        const float MOISTURE_MIN = 50.0f;
        const float MOISTURE_MAX = 450.0f;
        float scaledMoisture = MOISTURE_MIN + (float)soilMoisture * (MOISTURE_MAX - MOISTURE_MIN) / 100.0f;

        history.addReading(temperature, scaledMoisture);

        Serial.printf("[ML] History updated - Temp: %.1f, Moisture: %.0f (scaled: %.1f)\n",
                     temperature, soilMoisture, scaledMoisture);
    } else {
        Serial.println("[ML] No sensor data available for history update");
    }
}

void ML_ProcessDecision() {
    // Update history with latest sensor data
    ML_UpdateHistory();

    // Run inference
    float probability = ML_RunInference();

    // Get decision
    Decision_t decision = ML_GetDecision(probability);

    // Publish decision
    MQTT_APP_PublishDecision(decision);

    Serial.printf("[ML] Decision published: %d\n", (int)decision);
}

// SensorHistory method implementations
void SensorHistory::init() {
    index = 0;
    count = 0;
    for (int i = 0; i < HISTORY_SIZE; i++) {
        temperature[i] = 0;
        soilmoisture[i] = 0;
    }
}

void SensorHistory::addReading(float temp, float moisture) {
    temperature[index] = temp;
    soilmoisture[index] = moisture;
    index = (index + 1) % HISTORY_SIZE;
    if (count < HISTORY_SIZE) count++;
}

float SensorHistory::getTemp(uint8_t stepsAgo) {
    int idx = (index - 1 - stepsAgo + HISTORY_SIZE) % HISTORY_SIZE;
    return temperature[idx];
}

float SensorHistory::getMoisture(uint8_t stepsAgo) {
    int idx = (index - 1 - stepsAgo + HISTORY_SIZE) % HISTORY_SIZE;
    return soilmoisture[idx];
}

float SensorHistory::tempMean() {
    float sum = 0;
    for (int i = 0; i < count; i++) sum += temperature[i];
    return count > 0 ? sum / count : 0;
}

float SensorHistory::moistureMean() {
    float sum = 0;
    for (int i = 0; i < count; i++) sum += soilmoisture[i];
    return count > 0 ? sum / count : 0;
}

float SensorHistory::tempTrend() {
    if (count < 2) return 0;
    return getTemp(0) - getTemp(count - 1);
}

float SensorHistory::moistureTrend() {
    if (count < 2) return 0;
    return getMoisture(0) - getMoisture(count - 1);
}

bool SensorHistory::isReady() {
    return count >= HISTORY_SIZE;
}
