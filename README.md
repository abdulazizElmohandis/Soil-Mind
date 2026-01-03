## MQTT Topics

### Telemetry Topic
farm/<site>/<node>/telemetry

Example payload:
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

Control Topic

farm/<site>/<node>/control

Example payload:

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

Control Logic Summary

Soil Moisture State | Decision | Reason
Below minimum | ON | Moisture below minimum
Above maximum | OFF | Moisture above maximum
Within range | HOLD | Moisture in range
Manual override | ON / OFF | User decision

Deployment (Cloud / Local)
cd cloud/docker
docker compose up -d --build

Services:

MQTT Broker (Mosquitto): 1883

InfluxDB: 8086

Grafana: 3000

Control API: 8088

Grafana URL:
http://<SERVER_IP>:3000

Simulator (Testing Without Hardware)
cd cloud/simulator
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python simulator.py


The simulator behaves exactly like a real field node and was used to validate the full system behavior.

Grafana Dashboards — Proof of Work

Manual Mode:

Manual irrigation ON

Manual irrigation OFF

Automatic Mode Scenarios:

Soil moisture below minimum → Irrigation ON

Soil moisture within range → HOLD

Soil moisture above maximum → Irrigation OFF

Screenshots for all scenarios are provided in the screenshots/ directory and represent real system behavior.

Verification Summary

MQTT messages verified on broker topics

Telemetry data confirmed in InfluxDB bucket

Grafana dashboards showing live data

Control decisions observed and validated

Manual override tested successfully

Project Conclusion

This project successfully demonstrates a complete smart irrigation system with monitoring,
decision-making, and control capabilities. The system is stable, fully tested, and ready
to be extended to real agricultural deployments.

Author

Marwa Salama
Smart Irrigation • IoT • Cloud • MQTT • InfluxDB • Grafana
