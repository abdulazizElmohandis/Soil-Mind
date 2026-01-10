#!/usr/bin/env python3
import os
import json
import time
import signal
import sys

import paho.mqtt.client as mqtt

SITE = os.getenv("SITE", "site1")
NODE = os.getenv("NODE", "nodeB")
MQTT_HOST = os.getenv("MQTT_HOST", "127.0.0.1")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))

TOPIC_BASE = f"farm/{SITE}/{NODE}"
TOPIC_CONTROL = f"{TOPIC_BASE}/control"
TOPIC_STATUS = f"{TOPIC_BASE}/status"
TOPIC_HEALTH = f"{TOPIC_BASE}/health"

running = True
pump_on = 0  # simulated state

def handle_sig(*_):
    global running
    running = False

signal.signal(signal.SIGINT, handle_sig)
signal.signal(signal.SIGTERM, handle_sig)

def on_connect(client, userdata, flags, reason_code, properties=None):
    print(f"[nodeB] connected rc={reason_code} host={MQTT_HOST}:{MQTT_PORT} site={SITE} node={NODE}", flush=True)
    client.subscribe(TOPIC_CONTROL, qos=0)
    print(f"[nodeB] subscribed: {TOPIC_CONTROL}", flush=True)

def on_disconnect(client, userdata, reason_code, properties=None):
    print(f"[nodeB] disconnected rc={reason_code}", flush=True)

def publish_json(client: mqtt.Client, topic: str, payload: dict, qos: int = 0, retain: bool = False):
    client.publish(topic, json.dumps(payload), qos=qos, retain=retain)

def set_pump(client: mqtt.Client, on: int, reason: str = "control"):
    global pump_on
    pump_on = 1 if on else 0
    publish_json(
        client,
        TOPIC_STATUS,
        {"site": SITE, "node": NODE, "online": 1, "pump": pump_on, "reason": reason, "ts": int(time.time())},
        retain=True,
    )
    print(f"[nodeB] pump={pump_on} reason={reason}", flush=True)

def on_message(client, userdata, msg):
    payload_raw = msg.payload.decode("utf-8", errors="replace")
    try:
        data = json.loads(payload_raw)
    except Exception:
        print(f"[nodeB] bad json on {msg.topic}: {payload_raw}", flush=True)
        return

    # Accept common control formats:
    # {"pump":1} or {"pump_on":true} or {"action":"on"/"off"} or {"mode":"manual","value":1}
    on = None

    if "pump" in data:
        on = int(bool(data["pump"]))
    elif "pump_on" in data:
        on = int(bool(data["pump_on"]))
    elif "action" in data and str(data["action"]).lower() in ("on", "off"):
        on = 1 if str(data["action"]).lower() == "on" else 0
    elif "value" in data and "mode" in data:
        # if mode present, treat value as state
        on = int(bool(data["value"]))

    if on is None:
        print(f"[nodeB] control received but no recognized field: {data}", flush=True)
        return

    set_pump(client, on, reason="control")

def main():
    client = mqtt.Client(client_id=f"sim-{SITE}-{NODE}", protocol=mqtt.MQTTv311)
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message

    client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
    client.loop_start()

    # Online + initial state
    publish_json(client, TOPIC_STATUS, {"site": SITE, "node": NODE, "online": 1, "pump": pump_on, "ts": int(time.time())}, retain=True)
    publish_json(client, TOPIC_HEALTH, {"site": SITE, "node": NODE, "ok": 1, "ts": int(time.time())}, retain=True)

    try:
        while running:
            time.sleep(1)
    finally:
        # offline
        publish_json(client, TOPIC_STATUS, {"site": SITE, "node": NODE, "online": 0, "pump": pump_on, "ts": int(time.time())}, retain=True)
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"[nodeB] fatal: {e}", file=sys.stderr)
        raise
