import paho.mqtt.client as mqtt
import json
import random
import time

broker = "172.20.10.5"  # หรือ IP ของเครื่องที่รัน Mosquitto
port = 1883
topic = "sensor/data"

client = mqtt.Client()
client.connect(broker, port, 60)

while True:
    data = {
        "temperature": random.randint(20, 35),
        "humidity": random.randint(40, 80)
    }
    payload = json.dumps(data)
    print(f"Publish: {payload}")
    client.publish(topic, payload)
    time.sleep(5)
