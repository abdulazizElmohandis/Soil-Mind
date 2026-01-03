# Soil-Mind 🌱
### Smart Irrigation & Soil Monitoring Platform (AIoT)

Soil-Mind is a cloud-connected smart agriculture platform designed to monitor soil conditions and control irrigation systems using real-time telemetry, rule-based decision logic, and MQTT-based control loops.

This project demonstrates a **production-oriented AIoT architecture** combining edge logic, message brokers, time-series databases, and cloud-native visualization tools.

---

## 🚀 Key Features
- Real-time soil telemetry (moisture, temperature, humidity, pH)
- Automated irrigation decision engine (AUTO mode)
- Manual irrigation override via MQTT (MANUAL mode)
- Clear decision reasoning (ON / OFF / HOLD)
- Multi-site & multi-node scalable topic hierarchy
- Cloud-native monitoring with Grafana & InfluxDB
- Docker-based, reproducible deployment

---

## 🧱 System Architecture

**Edge / Simulator**
- ESP32-ready control logic (simulated via Python)
- Publishes telemetry and receives irrigation commands

**Messaging**
- MQTT broker (Mosquitto)
- Topic structure: `farm/<site>/<node>/...`

**Ingestion**
- Telegraf MQTT consumers
- Structured measurements: telemetry / status / control

**Storage**
- InfluxDB v2 (time-series database)

**Visualization & Control**
- Grafana dashboards
- Manual control & decision transparency

---


