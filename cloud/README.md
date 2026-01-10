# Soil-Mind — Cloud-Native Smart Irrigation Platform

## Overview
**Soil-Mind** is a cloud-native smart irrigation platform designed for real-time monitoring,
automated decision-making, and remote control of irrigation systems.

The platform is built with **scalability, observability, and hardware abstraction** as core
principles, enabling seamless operation with real IoT devices (ESP32) or software simulators.

---

## Key Design Goals
- Hardware-agnostic architecture (real devices or simulators)
- Event-driven communication using MQTT
- Time-series data storage optimized for analytics
- Cloud-first deployment using Docker
- Deterministic control logic with manual override
- Production-ready observability via Grafana

---

## System Architecture

```
Sensor Node (Node A)
  └── Telemetry → MQTT
          ↓
     Mosquitto Broker
          ↓
        Telegraf
          ↓
        InfluxDB
          ↓
        Grafana
          ↑
     Control API (FastAPI)
          ↑
Actuator Node (Node B)
```

---

## MQTT Topic Specification

### Telemetry
```
farm/<site>/<node>/telemetry
```

Publishes sensor readings from field nodes.

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

---

### Status
```
farm/<site>/<node>/status
```

Represents node availability and actuator state.

```json
{
  "site": "site1",
  "node": "nodeB",
  "online": 1,
  "pump": 0
}
```

---

### Control
```
farm/<site>/<node>/control
```

Issued by the Control API to drive irrigation decisions.

```json
{
  "decision": "ON",
  "irrigation": true,
  "soil_moisture": 25.0,
  "min_th": 30.0,
  "max_th": 45.0,
  "reason": "moisture below minimum"
}
```

---

## Control Logic

| Condition | Action | Description |
|---------|--------|-------------|
| Moisture < min | ON | Start irrigation |
| Moisture > max | OFF | Stop irrigation |
| Within range | HOLD | Maintain current state |
| Manual override | ON / OFF | User-initiated control |

---

## Deployment (Cloud)

### Prerequisites
- Docker
- Docker Compose

### Environment Setup
```bash
cp env.example .env
```

Populate the required environment variables inside `.env`.

### Start the Stack
```bash
cd cloud/docker
docker compose up -d
```

### Exposed Services

| Service | Port |
|-------|------|
| MQTT (Mosquitto) | 1883 |
| InfluxDB | 8086 |
| Grafana | 3000 |
| Control API | 8088 |

Grafana UI:
```
http://<SERVER_IP>:3000
```

---

## Simulator (Hardware-Free Validation)

To enable full system validation without physical devices, Soil-Mind includes Python-based simulators.

### Sensor Node (Node A)
```bash
python3 cloud/simulator/sim_nodeA.py
```

### Actuator Node (Node B)
```bash
python3 cloud/simulator/sim_nodeB.py
```

The simulators:
- Publish real MQTT telemetry
- Consume control commands
- Emulate real ESP32 device behavior

---

## Validation & Observability

The system was validated end-to-end using simulators.

- MQTT traffic verified at the broker
- Telemetry persisted in InfluxDB
- Grafana dashboards display real-time data
- Control decisions observed and validated

Screenshots for validation scenarios are available under the `screenshots/` directory.

---

## Project Status
- Stable
- Fully tested using simulators
- Ready for real hardware integration
- Designed with scalability in mind

---

## Author
**Marwa Salama**  
Cloud • IoT • Distributed Systems • Observability
