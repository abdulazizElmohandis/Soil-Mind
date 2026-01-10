#!/usr/bin/env python3
import os
import json
import time
import random
import signal
import sys

import paho.mqtt.client as mqtt

SITE = os.getenv("SITE", "site1")
NODE = os.getenv("NODE", "nodeA")
MQTT_HOST = os.getenv("MQTT_HOST", "127.0.0.1")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
INTERVAL_SEC = float(os.getenv("INTERVAL_SEC", "5"))

TOPIC_BASE = f"farm/{SITE}/{NODE}"
TOPIC_TELEMETRY = f"{TOPIC_BASE}/telemetry"
TOPIC_STATUS = f"{TOPIC_BASE}/status"
TOPIC_HEALTH = f"{TOPIC_BASE}/health"

running = True

def handle_sig(*_):
    global running
    running = False

signal.signal(signal.SIGINT, handle_sig)
signal.signal(signal.SIGTERM, handle_sig)

def on_connect(client, userdata, flags, reason_code, properties=None):
    print(f"[nodeA] connected rc={reason_code} host={MQTT_HOST}:{MQTT_PORT} site={SITE} node={NODE}", flush=True)

def on_disconnect(client, userdata, reason_code, properties=None):
    print(f"[nodeA] disconnected rc={reason_code}", flush=True)

def publish_json(client: mqtt.Client, topic: str, payload: dict, qos: int = 0, retain: bool = False):
    client.publish(topic, json.dumps(payload), qos=qos, retain=retain)

def gen_telemetry():
    # realistic-ish ranges; tweak later
    soil_moisture = round(random.uniform(10, 80), 1)   # %
    temperature = round(random.uniform(15, 40), 1)     # C
    humidity = round(random.uniform(20, 90), 1)        # %
    ph = round(random.uniform(5.5, 7.8), 2)
    n = round(random.uniform(0, 60), 1)
    p = round(random.uniform(0, 60), 1)
    k = round(random.uniform(0, 60), 1)

    return {
        "site": SITE,
        "node": NODE,
        "soil_moisture": soil_moisture,
        "temperature": temperature,
        "humidity": humidity,
        "ph": ph,
        "n": n,
        "p": p,
        "k": k,
        "ts": int(time.time()),
    }

def main():
    client = mqtt.Client(client_id=f"sim-{SITE}-{NODE}", protocol=mqtt.MQTTv311)
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect

    client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
    client.loop_start()

    # Publish online status (retain so dashboards can read last state)
    publish_json(client, TOPIC_STATUS, {"site": SITE, "node": NODE, "online": 1, "ts": int(time.time())}, retain=True)

    # Simple health heartbeat (retain latest)
    publish_json(client, TOPIC_HEALTH, {"site": SITE, "node": NODE, "ok": 1, "ts": int(time.time())}, retain=True)

    print(f"[nodeA] publishing: {TOPIC_TELEMETRY} {TOPIC_STATUS} {TOPIC_HEALTH}", flush=True)

    try:
        while running:
            telem = gen_telemetry()
            publish_json(client, TOPIC_TELEMETRY, telem)
            time.sleep(INTERVAL_SEC)
    finally:
        # offline status
        publish_json(client, TOPIC_STATUS, {"site": SITE, "node": NODE, "online": 0, "ts": int(time.time())}, retain=True)
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"[nodeA] fatal: {e}", file=sys.stderr)
        raise
