from fastapi import FastAPI, Query
import os
import json
import paho.mqtt.client as mqtt

app = FastAPI(title="Soil-Mind Control API", version="1.0")

MQTT_HOST = os.getenv("MQTT_HOST", "mosquitto")   # داخل docker compose
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))

def publish_cmd(site: str, node: str, cmd: str):
    topic = f"farm/{site}/{node}/cmd"
    payload = {"cmd": cmd}

    client = mqtt.Client()
    client.connect(MQTT_HOST, MQTT_PORT, 60)
    client.publish(topic, json.dumps(payload), qos=1, retain=False)
    client.disconnect()

    return {"topic": topic, "payload": payload}

@app.get("/health")
def health():
    return {"ok": True}

@app.get("/irrigation/on")
def irrigation_on(site: str = Query("site1"), node: str = Query("nodeA")):
    return publish_cmd(site, node, "ON")

@app.get("/irrigation/off")
def irrigation_off(site: str = Query("site1"), node: str = Query("nodeA")):
    return publish_cmd(site, node, "OFF")

@app.get("/irrigation/auto")
def irrigation_auto(site: str = Query("site1"), node: str = Query("nodeA")):
    return publish_cmd(site, node, "AUTO")
