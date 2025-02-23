#include <Arduino.h>
#include <WiFi.h>
#include <MQTT.h>
#include <LoRa.h>

//========= Konfigurasi WiFi dan MQTT ============
const char *SSID = "BUSOL";
const char *PASS = "$ut0h0m312";
const char *MQTT_SERVER = "docker-busol.paditech.id";
const char *MQTT_RELAY = "relay/cmnd";
const char *MQTT_PRESSURE_SENSOR = "sensor/pressure";
const char *MQTT_STATUS = "gateway/status/lwt";
const char *MQTT_CLIENT_ID = "Gateway_LoRa-Quinzel";
const char *MQTT_USERNAME = "busol-labs";
const char *MQTT_PASSWORD = "$ut0h0m312";

WiFiClient espClient;
MQTTClient client;

// Pin LoRa
#define SS_PIN 5
#define RST_PIN 14
#define DIO0_PIN 2
#define RF95_FREQ 915E6 // Sesuaikan dengan wilayah

// Pin Button
#define BUTTON_PIN 15

// Indicator
#define LED_LORA 25
#define LED_BUTTON 26

unsigned long lastReceivedTime = 0;
const unsigned long timeoutLoRa = 30000;
bool loraConnected = false;
bool loraDisconnectedSent = false;

// LED Blinking Control
unsigned long lastBlinkTime = 0;
bool ledState = false;
unsigned long blinkInterval = 1000; // Default 1 detik (untuk LoRa terkoneksi)

// Status Relay & Button
bool isRelayOn = false;
bool lastRelayState = false; // Menyimpan status terakhir relay
bool lastButtonState = HIGH;
unsigned long lastButtonPress = 0;

int sensorPressure;
int limitUp;
int limitDown;
String relayStatus;

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

void checkWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi terputus! Menghubungkan kembali...");
    WiFi.disconnect();
    WiFi.reconnect();
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
    {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("\nWiFi Reconnected!");
    }
    else
    {
      Serial.println("\nGagal reconnect ke WiFi.");
    }
  }
}

void connectToMqtt()
{
  Serial.print("Menghubungkan ke MQTT broker...");
  unsigned long startAttemptTime = millis();

  while (!client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD) && millis() - startAttemptTime < 10000)
  {
    Serial.print(".");
    delay(1000);
  }

  if (client.connected())
  {
    Serial.println("\nTerhubung ke MQTT broker!");
    client.subscribe(MQTT_RELAY);
    client.publish(MQTT_STATUS, "Online", true, 1);
  }
  else
  {
    Serial.println("\nGagal terhubung ke MQTT broker.");
  }
}

void messageReceived(String &topic, String &payload)
{
  if (topic.equals(MQTT_RELAY))
  {
    Serial.println("Data dari MQTT: " + payload);

    if (payload.equalsIgnoreCase("ON"))
    {
      isRelayOn = true;
    }
    else if (payload.equalsIgnoreCase("OFF"))
    {
      isRelayOn = false;
    }

    lastRelayState = isRelayOn;

    // Kirim ke LoRa
    LoRa.beginPacket();
    LoRa.print(isRelayOn ? "ON" : "OFF");
    LoRa.endPacket();
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_LORA, OUTPUT);
  pinMode(LED_BUTTON, OUTPUT);
  digitalWrite(LED_LORA, LOW); // Matikan LED awal

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
  checkWiFi();

  if (!client.connected())
  {
    Serial.println("MQTT Disconnected! Reconnecting...");
    connectToMqtt();
  }
  client.loop();

  // Tombol dengan debounce & edge detection
  bool buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH && millis() - lastButtonPress > 300)
  {
    lastButtonPress = millis();
    isRelayOn = !isRelayOn; // Toggle status relay

    // Kirim ke LoRa
    LoRa.beginPacket();
    LoRa.print(isRelayOn ? "ON" : "OFF");
    LoRa.endPacket();

    Serial.println(isRelayOn ? "Relay ON dari tombol!" : "Relay OFF dari tombol!");

    // Jika WiFi terhubung, kirim ke MQTT
    if (WiFi.status() == WL_CONNECTED)
    {
      client.publish(MQTT_RELAY, isRelayOn ? "ON" : "OFF");
    }
  }
  lastButtonState = buttonState; // Simpan state tombol terakhir

  // LED Button mengikuti status relay
  digitalWrite(LED_BUTTON, isRelayOn ? HIGH : LOW);

  int packetSize = LoRa.parsePacket();
  if (packetSize > 0)
  {
    String receivedMessage = "";
    while (LoRa.available())
    {
      receivedMessage += (char)LoRa.read();
    }

    Serial.println("Data dari Node: " + receivedMessage);
    lastReceivedTime = millis();
    loraConnected = true;
    loraDisconnectedSent = false;
    blinkInterval = 1000; // LoRa terkoneksi, berkedip setiap 1 detik

    String jsonPayload = "{\"lora_status\":\"Connected\"}";
    client.publish(MQTT_PRESSURE_SENSOR, jsonPayload.c_str());
  }

  // Cek apakah LoRa masih aktif
  if (millis() - lastReceivedTime > timeoutLoRa)
  {
    if (loraConnected)
    {
      Serial.println("LoRa Disconnected!");
      loraConnected = false;
    }

    if (!loraDisconnectedSent)
    {
      String jsonPayload = "{\"lora_status\":\"Disconnected\"}";
      client.publish(MQTT_PRESSURE_SENSOR, jsonPayload.c_str());
      loraDisconnectedSent = true;
    }

    blinkInterval = 100; // LoRa terputus, berkedip cepat (100 ms)
  }

  // Kontrol LED Indikator LoRa
  if (millis() - lastBlinkTime >= blinkInterval)
  {
    ledState = !ledState;
    digitalWrite(LED_LORA, ledState);
    lastBlinkTime = millis();
  }
}
