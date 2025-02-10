#include <Arduino.h>
#include <WiFi.h>
#include <MQTT.h>
#include <LoRa.h>

//========= Konfigurasi WiFi dan MQTT ============
const char *SSID = "BUSOL";
const char *PASS = "$ut0h0m312";
const char *MQTT_SERVER = "192.168.8.220";
const char *MQTT_RELAY = "relay/cmnd";
const char *MQTT_PRESSURE_SENSOR = "sensor/pressure/stat";
const char *MQTT_PRESSURE_UP = "sensor/pressure/up/stat";
const char *MQTT_PRESSURE_DOWN = "sensor/pressure/down/stat";
const char *MQTT_STATUS = "gateway/status/lwt";
const char *MQTT_CLIENT_ID = "Gateway_LoRa";
const char *MQTT_USERNAME = "paditech";
const char *MQTT_PASSWORD = "p4d1n3tbusol";

WiFiClient espClient;
MQTTClient client;

// Pin LoRa
#define SS_PIN 5
#define RST_PIN 14
#define DIO0_PIN 2
#define RF95_FREQ 915E6 // Sesuaikan dengan wilayah

unsigned long lastReceivedTime = 0;      // Waktu terakhir LoRa menerima data
const unsigned long timeoutLoRa = 10000; // 10 detik timeout

void setup_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
  }
  Serial.println("WiFi Connected.");
}

void connectToMqtt()
{
  while (!client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
  {
    delay(1000);
  }
  client.subscribe(MQTT_RELAY);
  client.publish(MQTT_STATUS, "Online", true, 1);
}

void messageReceived(String &topic, String &payload)
{
  if (topic.equals(MQTT_RELAY))
  {
    Serial.println("Kirim ke Node: " + payload);
    LoRa.beginPacket();
    LoRa.print(payload);
    LoRa.endPacket();
  }
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();

  client.begin(MQTT_SERVER, 1883, espClient);
  connectToMqtt();
  client.onMessage(messageReceived);

  LoRa.setPins(SS_PIN, RST_PIN, DIO0_PIN);
  if (!LoRa.begin(RF95_FREQ))
  {
    Serial.println("Gagal memulai LoRa!");
    while (1)
      ;
  }
  Serial.println("LoRa Initialized!");
  LoRa.setTxPower(23);
}

void loop()
{
  if (!client.connected())
  {
    connectToMqtt();
  }
  client.loop();

  int packetSize = LoRa.parsePacket();
  if (packetSize > 0)
  {
    String receivedMessage = "";
    while (LoRa.available())
    {
      receivedMessage += (char)LoRa.read();
    }

    Serial.println("Data dari Node: " + receivedMessage);
    lastReceivedTime = millis(); // Reset waktu terakhir menerima data

    // format pesan : "P:xx UP:xx DOWN:xx"
    int sensorP = receivedMessage.substring(receivedMessage.indexOf("P:") + 2, receivedMessage.indexOf("UP:")).toInt();
    int upP = receivedMessage.substring(receivedMessage.indexOf("UP:") + 3, receivedMessage.indexOf("DOWN:")).toInt();
    int downP = receivedMessage.substring(receivedMessage.indexOf("DOWN:") + 5).toInt();

    // publish ke MQTT
    client.publish(MQTT_PRESSURE_SENSOR, String(sensorP).c_str());
    client.publish(MQTT_PRESSURE_UP, String(upP).c_str());
    client.publish(MQTT_PRESSURE_DOWN, String(downP).c_str());
  }

  // **Monitor jika LoRa tidak menerima data dalam 10 detik**
  if (millis() - lastReceivedTime > timeoutLoRa)
  {
    Serial.println("LoRa Timeout! Tidak menerima data dalam 10 detik.");
    client.publish(MQTT_STATUS, "LoRa Disconnected", true, 1);
    lastReceivedTime = millis(); // Hindari spam, reset waktu terakhir
  }
}
