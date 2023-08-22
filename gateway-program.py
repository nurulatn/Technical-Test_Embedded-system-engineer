import paho.mqtt.client as mqtt
import time

# MQTT settings
mqtt_broker = "2001:41d0:1:925e::1"
mqtt_port = 1883
mqtt_topic = "voltage/DATA/LOCAL/SENSOR/PANEL_1"

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe(mqtt_topic)

def on_message(client, userdata, msg):
    print("Received message: " + msg.payload.decode())

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(mqtt_broker, mqtt_port, 60)

client.loop_start()

try:
    while True:
        time.sleep(1)  # Tunggu selama 1 detik
except KeyboardInterrupt:
    client.loop_stop()