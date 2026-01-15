# Smart Farm IoT System

An IoT-based plant health monitoring and automated irrigation system using TinyML, ESP32 microcontroller, and cloud-deployed data pipeline.

![M](https://github.com/user-attachments/assets/64eb0611-8741-499e-9a67-7166ec35b8ef)


## Overview

This project combines edge computing with cloud-based analytics to monitor plant health in real-time and automate irrigation decisions. The system uses TinyML models running on FreeRTOS for on-device inference, while sensor data flows through an MQTT-based pipeline to a cloud-hosted monitoring dashboard accessible from anywhere in the world.

## System Architecture

![System Architecture](docs/flowchart.drawio.png "Architecture Diagram")


### Hardware Components

**ESP32 Microcontroller (FreeRTOS)**
- Runs FreeRTOS for real-time task management
- Executes two TinyML models for edge inference
- Collects and processes data from multiple sensors
- Publishes sensor data via MQTT protocol

**Sensors**
| Sensor | Measurements |
|--------|-------------|
| Soil Moisture Sensor | Soil moisture percentage |
| DHT Sensor | Temperature (°C) and Humidity (%) |
| NPK Sensor | Nitrogen, Phosphorus, Potassium levels |
| pH Sensor | Soil pH level |

### Software Stack

**Edge Layer**
- **Operating System:** FreeRTOS
- **ML Framework:** TensorFlow Lite Micro (TinyML)
- **Communication Protocol:** MQTT

**Cloud Infrastructure (Docker Containerized)**

The entire data pipeline is containerized using Docker and deployed on cloud platforms (AWS / Azure / Hostinger) enabling remote access to the dashboard from anywhere in the world.

| Component | Function |
|-----------|----------|
| **Mosquitto** | MQTT broker for receiving sensor data from ESP32 |
| **Telegraf** | Data collection agent and metrics aggregation |
| **InfluxDB** | Time-series database for storing sensor data |
| **Grafana** | Real-time visualization and monitoring dashboards |

### Machine Learning Models

**1. Plant Health Prediction Model**
- Analyzes sensor data to determine plant health status
- Classifies plant condition as Healthy/Unhealthy
- Runs on-device using TinyML

**2. Irrigation Control Model**
- Predicts optimal irrigation timing based on soil conditions
- Controls irrigation system (ON/OFF/STANDBY)
- Enables automated watering decisions

## Grafana Dashboard

The Smart Farm IoT Dashboard displays real-time monitoring data:

**Sensor Readings**
- Soil Moisture (%) with threshold indicators
- Temperature (°C)
- Humidity (%)
- NPK Levels (Nitrogen, Phosphorus, Potassium)
- pH Level with optimal range gauge

**Visualizations**
- Soil Moisture & Temperature real-time graphs
- NPK Levels bar charts
- NPK Trends time-series graphs
- pH Level gauge with optimal zone indicator

**System Status**
- Irrigation System status (ON/OFF/STANDBY)
- Plant Health status (HEALTHY/UNHEALTHY)
- Active Alerts panel
- Recommendations section

## Features

- ✅ Real-time plant health monitoring
- ✅ On-device ML inference using TinyML
- ✅ Automated irrigation control based on ML predictions
- ✅ Cloud-deployed pipeline for global dashboard access
- ✅ Historical data visualization and trends
- ✅ Configurable thresholds and alerts
- ✅ Recommendations based on sensor readings

## Deployment

The Docker containerized stack (Mosquitto → Telegraf → InfluxDB → Grafana) can be deployed on:
- **AWS** (EC2 / ECS)
- **Azure** (Container Instances / AKS)
- **Hostinger** (VPS)

This enables accessing the Grafana dashboard from any location worldwide via web browser.

## Getting Started

Setup instructions will be added as development progresses, including:
- Docker Compose configuration
- ESP32 firmware flashing guide
- Cloud deployment steps
- Grafana dashboard import

## Project Structure

```
Soil-Mind/
├── AI/                          # Machine Learning models and training
│   ├── irrigation_model_v1/     # First version of irrigation model
│   ├── irrigation_model_v2/     # Second version with improvements
│   ├── plant_health_model/      # Plant health classification model
│   └── irrigation_schduling_next_step_for_interfacing/
│       └── integrate_with_real_sensors/
├── cloud/                       # Cloud infrastructure and deployment
│   ├── docker/                  # Docker Compose setup
│   │   ├── docker-compose.yml   # Multi-service container configuration
│   │   ├── mosquitto/          # MQTT broker configuration
│   ├── grafana/                 # Grafana dashboard provisioning
│   ├── simulator/               # Python simulator for testing
│   └── telegraf/                # Telegraf configuration for data collection
├── interfacing/                 # ESP32 firmware and hardware interface
│   ├── interfacing.ino         # Main Arduino sketch with FreeRTOS tasks
│   └── src/
│       ├── APP_Cfg.h           # Application configuration and pin assignments
│       ├── App/                # Application layer modules
│       │   ├── DHT/            # Temperature/Humidity sensor
│       │   ├── ML/             # Machine learning inference
│       │   ├── MQTT_APP/       # MQTT application layer
│       │   ├── SoilMoisture/   # Soil moisture sensor
│       │   ├── NitrogenSensor/ # NPK sensors
│       │   ├── PhosphorusSensor/
│       │   ├── PotassiumSensor/
│       │   └── PHSensor/
│       └── Hal/                # Hardware abstraction layer
│           ├── ADC/            # Analog-to-digital conversion
│           ├── GPIO/           # General purpose I/O
│           ├── MQTT/           # MQTT protocol implementation
│           ├── PWM/            # Pulse width modulation
│           ├── UART/           # Serial communication
│           └── WIFI/           # WiFi connectivity
├── docs/                        # Documentation and diagrams
├── screenshots/                 # Dashboard screenshots and proofs
└── README.md                    # This file
```

## Prerequisites & Dependencies

### Hardware Requirements
- ESP32 microcontroller (with WiFi and Bluetooth)
- DHT11 or DHT22 temperature/humidity sensor
- Soil moisture sensor (capacitive or resistive)
- NPK sensor (Nitrogen, Phosphorus, Potassium)
- pH sensor
- 12V DC water pump with relay module
- Power supply (5V for ESP32, appropriate voltage for pump)
- Jumper wires and breadboard for prototyping

### Software Requirements

**ESP32 Firmware Development:**
- Arduino IDE 1.8.19+
- ESP32 board support (via Board Manager)
- FreeRTOS (included with ESP32 Arduino core)
- TensorFlow Lite Micro library
- ArduinoJson library
- PubSubClient (MQTT) library

**Cloud Infrastructure:**
- Docker Engine 20.10+
- Docker Compose 2.0+

**AI Model Development:**
- Python 3.8+
- TensorFlow 2.10+
- Jupyter Notebook
- Required Python packages: pandas, numpy, scikit-learn, matplotlib, seaborn

### Cloud Services (for deployment)
- Mosquitto MQTT broker
- InfluxDB time-series database
- Grafana visualization platform
- Telegraf data collection agent

## Installation & Setup

### ESP32 Firmware Setup

1. **Install Arduino IDE and ESP32 Support:**
   ```bash
   # Add ESP32 board support via Board Manager
   # URL: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

2. **Install Required Libraries:**
   - TensorFlowLite_ESP32
   - ArduinoJson
   - PubSubClient
   - DHT sensor library

3. **Clone and Open Project:**
   ```bash
   git clone https://github.com/abdulazizElmohandis/Soil-Mind.git
   cd Soil-Mind/interfacing
   ```

4. **Configure Settings:**
   - Edit `src/APP_Cfg.h` for your WiFi credentials, MQTT broker IP, and pin assignments
   - Adjust sensor calibration values if needed

5. **Upload Firmware:**
   - Select ESP32 Dev Module board
   - Set upload speed to 115200
   - Upload the `interfacing.ino` sketch

### Hardware Wiring

Connect sensors according to pin assignments in `APP_Cfg.h`:

- Soil Moisture: GPIO 35 (ADC1)
- DHT11: GPIO 5
- Nitrogen Sensor: GPIO 36 (ADC1)
- Phosphorus Sensor: GPIO 39 (ADC1)
- Potassium Sensor: GPIO 33 (ADC1)
- pH Sensor: GPIO 32 (ADC1)
- Pump Relay: GPIO 26

### Cloud Infrastructure Setup

1. **Navigate to cloud directory:**
   ```bash
   cd cloud/docker
   ```

2. **Deploy services:**
   ```bash
   docker compose up -d --build
   ```

3. **Access services:**
   - Grafana: http://localhost:3000 (admin/admin)
   - InfluxDB: http://localhost:8086
   - MQTT Broker: localhost:1883

### AI Model Training Setup

1. **Navigate to AI directory:**
   ```bash
   cd AI/irrigation_model_v2
   ```

2. **Install Python dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

3. **Run Jupyter notebook:**
   ```bash
   jupyter notebook "Irrigation_scheduling.ipynb"
   ```

## Configuration

### Application Configuration (`interfacing/src/APP_Cfg.h`)

**Pin Assignments:**
```c
#define SOILMOISTURE_PIN    35
#define DHT11_1_PIN         5
#define Nitrogen_SENSOR_PIN 36
#define Phosphorus_SENSOR_PIN 39
#define Potassium_SENSOR_PIN 33
#define PH_SENSOR_PIN       32
#define PUMP_PIN            26
```

**Network Configuration:**
```c
#define WIFI_SSID          "Your_WiFi_SSID"
#define WIFI_PASSWORD      "Your_WiFi_Password"
#define MQTT_BROKER        "192.168.1.100"  // Your MQTT broker IP
#define MQTT_PORT          1883
```

**Sensor Thresholds:**
```c
#define DRY_VALUE          3800  // ADC value for dry soil
#define WET_VALUE          1250  // ADC value for wet soil
#define NITROGEN_MAX       200   // Max nitrogen level (mg/kg)
#define PHOSPHORUS_MAX     200   // Max phosphorus level (mg/kg)
#define POTASSIUM_MAX      200   // Max potassium level (mg/kg)
#define PH_MAX             14    // Max pH value
```

**RTOS Task Configuration:**
- WiFi monitoring task: 100ms interval, Core 0
- Sensor/MQTT task: 400ms interval, Core 1
- ML inference task: 30s interval, Core 1

## Usage & Operation

### Starting the System

1. **Power on ESP32** with sensors connected
2. **Start cloud services:**
   ```bash
   cd cloud/docker
   docker compose up -d
   ```

3. **Monitor logs:**
   ```bash
   docker compose logs -f
   ```

### Accessing Grafana Dashboard

1. Open http://localhost:3000
2. Login with admin/admin
3. Navigate to the Smart Farm dashboard

### Manual vs Automatic Mode

**Manual Mode:**
- Irrigation controlled via MQTT commands
- Override automatic decisions

**Automatic Mode:**
- ML model makes irrigation decisions every 30 seconds
- Based on soil moisture, temperature, and NPK levels

## API Reference

### MQTT Topics

#### Telemetry Topic: `farm/site1/nodeA/telemetry`
```json
{
  "site": "site1",
  "node": "nodeA",
  "soil_moisture": 45.2,
  "temperature": 29.6,
  "humidity": 57.6,
  "ph": 6.9,
  "n": 7.3,
  "p": 18.3,
  "k": 16.8
}
```

#### Control Topic: `farm/site1/nodeA/control`
```json
{
  "site": "site1",
  "node": "nodeA",
  "irrigation": true,
  "decision": "ON",
  "reason": "moisture below min",
  "soil_moisture": 25.0,
  "min_th": 30.0,
  "max_th": 45.0
}
```

#### Decision Topic: `farm/site1/nodeA/decision`
```json
{
  "site": "site1",
  "node": "nodeA",
  "timestamp": 1640995200,
  "irrigation_decision": "ON",
  "probability": 0.85,
  "sensor_data": {
    "soil_moisture": 25.0,
    "temperature": 29.6,
    "humidity": 57.6
  }
}
```

## Development & Testing

### Using the Simulator

The Python simulator mimics ESP32 behavior for testing without hardware:

```bash
cd cloud/simulator
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python simulator.py
```

### Debugging

Enable debug flags in `APP_Cfg.h`:
```c
#define WIFI_DEBUG                 STD_ON
#define MQTT_DEBUG                 STD_ON
#define DHT11_DEBUG                STD_ON
// ... other debug flags
```

🚧 **Currently in Development**
