#!/usr/bin/env python3

import os
import sys
import json
import time
import random
import signal
import logging
import paho.mqtt.client as mqtt

# =========================
# Logging (nohup-safe)
# =========================
logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] %(message)s',
    handlers=[logging.StreamHandler(sys.stdout)]
)
log = logging.getLogger()

# =========================
# Config
# =========================
BROKER_HOST = os.getenv("MQTT_HOST", "127.0.0.1")
BROKER_PORT = int(os.getenv("MQTT_PORT", "1883"))

SITE = os.getenv("SITE", "site1")
NODE = os.getenv("NODE", "nodeA")

PUBLISH_EVERY_SEC = float(os.getenv("PUBLISH_EVERY", "5"))

MIN_TH = float(os.getenv("MIN_TH", "30"))
MAX_TH = float(os.getenv("MAX_TH", "45"))

TELEMETRY_TOPIC = f"farm/{SITE}/{NODE}/telemetry"
STATUS_TOPIC    = f"farm/{SITE}/{NODE}/status"
CONTROL_TOPIC   = f"farm/{SITE}/{NODE}/control"
CMD_TOPIC       = f"farm/{SITE}/{NODE}/cmd"

# None = AUTO, True = MANUAL ON, False = MANUAL OFF
manual_override = None
running = True

# =========================
# MQTT callbacks
# =========================
def on_connect(client, userdata, flags, rc):
    log.info("[MQTT] Connected")
    client.subscribe(CMD_TOPIC)
    publish_status(True)

def on_message(client, userdata, msg):
    global manual_override
    try:
        payload = json.loads(msg.payload.decode())
        cmd = str(payload.get("cmd", "")).upper()

        if cmd == "ON":
            manual_override = True
            log.info("[CMD] Manual ON")
        elif cmd == "OFF":
            manual_override = False
            log.info("[CMD] Manual OFF")
        elif cmd == "AUTO":
            manual_override = None
            log.info("[CMD] Back to AUTO")
        else:
            log.info(f"[CMD] Unknown cmd={cmd} payload={payload}")

    except Exception as e:
        log.error(f"[MQTT] Bad command payload: {e}")

# =========================
# Publishers
# =========================
def publish_status(online: bool):
    payload = {"site": SITE, "node": NODE, "online": online}
    client.publish(STATUS_TOPIC, json.dumps(payload), qos=1)
    log.info(f"[STATUS] online={online}")

def publish_telemetry():
    telemetry = {
        "site": SITE,
        "node": NODE,
        "soil_moisture": round(random.uniform(20, 70), 1),
        "temperature": round(random.uniform(18, 35), 1),
        "humidity": round(random.uniform(40, 90), 1),
        "ph": round(random.uniform(5.5, 8.0), 2),
        "n": round(random.uniform(5, 20), 1),
        "p": round(random.uniform(5, 20), 1),
        "k": round(random.uniform(5, 20), 1),
    }

    client.publish(TELEMETRY_TOPIC, json.dumps(telemetry), qos=1)
    log.info(f"[TELEMETRY] {telemetry}")
    return telemetry

def decide_and_publish_control(telemetry):
    global manual_override

    soil = float(telemetry["soil_moisture"])

    # ✅ Manual mode holds decision ثابت (مش بيتدهس)
    if manual_override is True:
        irrigation = True
        decision = "ON"
        reason = "manual override"

    elif manual_override is False:
        irrigation = False
        decision = "OFF"
        reason = "manual override"

    else:
        # ✅ AUTO logic
        if soil < MIN_TH:
            irrigation = True
            decision = "ON"
            reason = "moisture below min"
        elif soil > MAX_TH:
            irrigation = False
            decision = "OFF"
            reason = "moisture above max"
        else:
            irrigation = False
            decision = "HOLD"
            reason = "moisture in range"

    control = {
        "site": SITE,
        "node": NODE,
        "irrigation": irrigation,
        "decision": decision,
        "reason": reason,
        "soil_moisture": soil,
        "min_th": MIN_TH,
        "max_th": MAX_TH
    }

    client.publish(CONTROL_TOPIC, json.dumps(control), qos=1)
    log.info(f"[CONTROL] {control}")

# =========================
# Graceful shutdown
# =========================
def shutdown(sig, frame):
    global running
    log.info("[SYSTEM] Shutting down...")
    publish_status(False)
    running = False
    time.sleep(1)
    sys.exit(0)

signal.signal(signal.SIGINT, shutdown)
signal.signal(signal.SIGTERM, shutdown)

# =========================
# Main
# =========================
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

log.info("[SIM] Connecting to MQTT broker...")
client.connect(BROKER_HOST, BROKER_PORT, 60)
client.loop_start()

try:
    while running:
        telemetry = publish_telemetry()
        decide_and_publish_control(telemetry)
        time.sleep(PUBLISH_EVERY_SEC)

except Exception as e:
    log.error(f"[SIM] Runtime error: {e}")
    shutdown(None, None)
