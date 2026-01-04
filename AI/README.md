# Soil-Mind AI & TinyML Models

This directory contains the machine learning models for the Soil-Mind intelligent irrigation and plant health monitoring system. All models are optimized for deployment on ESP32 microcontrollers using TensorFlow Lite.

## 📋 Table of Contents

- [Overview](#overview)
- [Models](#models)
  - [Irrigation Scheduling Model](#irrigation-scheduling-model)
  - [Plant Health Classification Model](#plant-health-classification-model)
- [Model Architecture](#model-architecture)
- [Deployment](#deployment)
- [Performance Metrics](#performance-metrics)
- [File Structure](#file-structure)
- [Usage](#usage)
- [Development](#development)

---

## Overview

The Soil-Mind AI system consists of two main TinyML models:

1. **Irrigation Scheduling Model** - Predicts optimal irrigation timing using trend-based forecasting
2. **Plant Health Classification Model** - Classifies plant health conditions based on soil nutrients and environmental factors

Both models are:
- ✅ Optimized for ESP32 microcontrollers
- ✅ Quantized to INT8 for minimal memory footprint
- ✅ Deployed as TensorFlow Lite models
- ✅ Run inference locally on edge devices (no cloud required)

---

## Models

### Irrigation Scheduling Model

**Purpose:** Intelligent irrigation scheduling using trend-based forecasting

**Key Innovation:** Unlike simple threshold-based systems, this model predicts future irrigation needs by analyzing temporal patterns and trends in sensor data.

#### Model

##### v2 
- **Location:** `irrigation_model_v2/`
- **Type:** Binary Classification (Irrigation Needed / Not Needed)
- **Features:** 8 temporal features
  - Current temperature
  - Current soil moisture
  - Temperature mean (rolling average)
  - Soil moisture mean (rolling average)
  - Temperature trend (rate of change)
  - Soil moisture trend (rate of change)
  - Soil moisture lag-1 (previous reading)
  - Soil moisture lag-2 (2 readings ago)
- **Forecast Horizon:** Predicts 2 steps ahead
- **Test Accuracy:** 93.16%
- **Model Size:** ~23 KB (INT8 quantized)
- **Inference Time:** < 50ms on ESP32


#### Training Data

- **Dataset:** Real-world sensor data from agricultural fields
- **Samples:** 4,688+ readings
- **Labeling:** Based on agronomic rules:
  - Soil moisture < 25% → Always irrigate
  - Soil moisture < 40% AND temp > 28°C → Irrigate
  - Soil moisture < 35% AND humidity < 45% → Irrigate
  - Soil moisture >= 70% → Never irrigate

#### Feature Engineering

The v2 model uses advanced time-series feature engineering:

```python
# Rolling statistics
temperature_mean = rolling_mean(temperature, window=4)
soilmoisture_mean = rolling_mean(soilmoisture, window=4)

# Trend features
temperature_trend = diff(temperature, periods=4)
soilmoisture_trend = diff(soilmoisture, periods=4)

# Lag features
soilmoisture_lag_1 = shift(soilmoisture, periods=1)
soilmoisture_lag_2 = shift(soilmoisture, periods=2)
```

---

### Plant Health Classification Model

**Purpose:** Multi-class classification of plant health conditions

**Location:** `plant_health_model/`

#### Model Details

- **Type:** Multi-class Classification (7 classes)
- **Features:** 6 features
  1. N (Nitrogen) 
  2. P (Phosphorus) 
  3. K (Potassium) 
  4. pH - soil pH level
  5. Soil Moisture - percentage
  6. Temperature - °C
- **Classes:**
  1. Healthy
  2. Nitrogen Deficiency
  3. pH Stress (Acidic)
  4. pH Stress (Alkaline)
  5. Phosphorus Deficiency
  6. Potassium Deficiency
  7. Water Stress
- **Model Size:** ~36 KB (INT8 quantized)
- **Dataset:** Plant health dataset with labeled nutrient and environmental conditions

#### Data Preprocessing

- **Class Balancing:** SMOTE (Synthetic Minority Oversampling Technique)
- **Normalization:** StandardScaler (Z-score normalization)
- **Feature Scaling:** Mean and standard deviation stored in model header file

---

## Model Architecture

### Irrigation Model v2

```
Input Layer: 8 features
    ↓
Hidden Layer 1: Dense(16) + ReLU
    ↓
Hidden Layer 2: Dense(8) + ReLU
    ↓
Output Layer: Dense(1) + Sigmoid
    ↓
Output: Irrigation probability (0-1)
```

**Architecture Details:**
- **Activation:** ReLU for hidden layers, Sigmoid for output
- **Loss Function:** Binary Cross-Entropy
- **Optimizer:** Adam
- **Regularization:** Early stopping, dropout

### Plant Health Model

```
Input Layer: 6 features
    ↓
Hidden Layer 1: Dense(64) + ReLU
    ↓
Dropout(0.3)
    ↓
Hidden Layer 2: Dense(32) + ReLU
    ↓
Dropout(0.3)
    ↓
Output Layer: Dense(7) + Softmax
    ↓
Output: Class probabilities (7 classes)
```

**Architecture Details:**
- **Activation:** ReLU for hidden layers, Softmax for output
- **Loss Function:** Categorical Cross-Entropy
- **Optimizer:** Adam with learning rate scheduling
- **Regularization:** Dropout, early stopping

---

### Model Files

Each model directory contains:

1. **Model Header File** (`*.h`)
   - TensorFlow Lite model as C array
   - Normalization parameters
   - Quantization parameters
   - Feature definitions

2. **TensorFlow Lite Model** (`*.tflite`)
   - Quantized INT8 model
   - Ready for deployment

3. **Training Notebook** (`*.ipynb`)
   - Complete training pipeline
   - Data preprocessing
   - Model evaluation
   - Visualization

4. **Test Code** (`Arduino_test/`)
   - Arduino/ESP32 test code
   - Example inference implementation

### Integration Steps

1. **Include Model Header File**
   ```cpp
   // Copy irrigation_model.h to your ESP32 project
   #include "irrigation_model.h"
   ```

2. **Include TensorFlow Lite Library**
   ```cpp
   #include <Arduino_TensorFlowLite.h>
   // or
   #include "tensorflow/lite/micro/all_ops_resolver.h"
   ```

3. **Initialize Model**
   ```cpp
   const tflite::Model* model = tflite::GetModel(irrigation_model);
   static tflite::MicroInterpreter* interpreter = nullptr;
   // ... initialize interpreter
   ```

4. **Run Inference**
   ```cpp
   // Prepare features
   float features[8] = {temp, moisture, temp_mean, ...};
   
   // Run inference
   TfLiteTensor* input = interpreter->input(0);
   // ... copy features to input tensor
   interpreter->Invoke();
   
   // Get output
   TfLiteTensor* output = interpreter->output(0);
   float probability = output->data.f[0];
   ```

See individual model directories for complete Arduino/ESP32 example code.

---

## Performance Metrics

### Irrigation Model v2

| Metric | Value |
|--------|-------|
| Accuracy | 93.16% |
| Precision (Irrigation) | 0.95 |
| Recall (Irrigation) | 0.97 |
| F1-Score (Irrigation) | 0.96 |
| Model Size | ~23 KB |


### Plant Health Model

| Metric | Value |
|--------|-------|
| Accuracy | ~95% |
| Classes | 7 |
| Model Size | ~36 KB |

*Note: Performance may vary based on hardware and sensor calibration*

---

## File Structure

```
AI/
├── README.md                                    # This file
│
├── irrigation_model_v2/                         # Recommended irrigation model ⭐
│   ├── Irrigation_scheduling.ipynb             # Training notebook
│   ├── irrigation_model.h                      # Model header file
│   ├── irrigation_model_int8.tflite            # TFLite model
│   ├── scaler_params.csv                       # Normalization parameters
│   ├── Arduino_test_with_pot/                  # Test code with potentiometers
│   ├── Irrigation Scheduling.csv               # Training dataset
│   └── *.png                                   # Visualization outputs
│
├── irrigation_schduling_next_step_for_interfacing/
│   ├── real-sensor-code/                       # Real sensor integration code
│   │   ├── real-sensor-code.ino
│   │   └── irrigation_model.h
│   └── how to interface with real data (irrifation scheduling).pdf
│
├── plant_health_model/                          # Plant health classification model
│   ├── plant_health_model.ipynb                # Training notebook
│   ├── plant_health_model.h                    # Model header file
│   ├── plant_health_model.tflite               # TFLite model
│   ├── plant_health.csv                        # Training dataset
│   ├── Arduino_test/                           # Test code
│   └── *.png                                   # Visualization outputs
│
└── plant_health_model_next_step_for_interfacing/
    ├── plant-health-full-code.ino              # Full integration code
    ├── sensor-test-code.ino                    # Sensor test code
    └── next_step_for_plant_health_model.pdf
```

---

## Usage

### Training New Models

1. **Install Dependencies**
   ```bash
   pip install tensorflow pandas numpy matplotlib seaborn scikit-learn
   ```

2. **Open Training Notebook**
   ```bash
   jupyter notebook irrigation_model_v2/Irrigation_scheduling.ipynb
   ```

3. **Follow Notebook Steps**
   - Load and preprocess data
   - Feature engineering
   - Train model
   - Evaluate performance
   - Convert to TFLite
   - Generate header file

### Using Pre-trained Models

1. **For Irrigation Scheduling:**
   - Use `irrigation_model_v2/irrigation_model.h`
   - Follow example in `irrigation_model_v2/Arduino_test_with_pot/`
   - Or use production code in `irrigation_schduling_next_step_for_interfacing/real-sensor-code/`

2. **For Plant Health Classification:**
   - Use `plant_health_model/plant_health_model.h`
   - Follow example in `plant_health_model/Arduino_test/`
   - Or use production code in `plant_health_model_next_step_for_interfacing/`

### Model Selection Guide

| Use Case | Recommended Model |
|----------|-------------------|
| **Production Irrigation** | `irrigation_model_v2` (trend-based, more accurate) |
| **Plant Health Monitoring** | `plant_health_model` (requires NPK + pH sensors) |

---

## Development

### Model Optimization Techniques

1. **Quantization**
   - INT8 quantization reduces model size by ~4x
   - Minimal accuracy loss (< 1%)
   - Faster inference on ESP32

2. **Feature Engineering**
   - Time-series features (trends, lags, rolling stats)
   - Domain knowledge integration (agronomic rules)
   - Feature selection for model size

3. **Architecture Optimization**
   - Minimal hidden layers
   - Appropriate activation functions
   - Dropout for regularization

### Future Improvements

- [ ] Retrain models with larger datasets
- [ ] Ensemble models for better accuracy
- [ ] Transfer learning for new crops
- [ ] Edge learning (online adaptation)
- [ ] Multi-field optimization models
- [ ] Weather integration for predictions
