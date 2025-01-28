#include <Arduino.h>
#include <WiFi.h>
#include <MQTT.h>

//========= Konfigurasi WiFi dan MQTT ============
const char *SSID = "xxxxxx";
const char *PASS = "xxxxxx";
const char *MQTT_SERVER = "xxxxxx";
const char *MQTT_RELAY = "xxxxxx";
const char *MQTT_NODES = "xxxxxx";
const char *MQTT_STATUS = "xxxxxx";
const char *MQTT_PRESSURE_SENSOR = "sensor/pressure";
const char *MQTT_PRESSURE_UP = "sensor/pressure/up";
const char *MQTT_PRESSURE_DOWN = "sensor/pressure/down";
const char *WILL_MESSAGE = "Offline";
const char *MQTT_CLIENT_ID = "xxxxxx";
const char *MQTT_USERNAME = "xxxxxx";
const char *MQTT_PASSWORD = "xxxxxx";

WiFiClient espClient;
MQTTClient client;

// Relay dan Potensiometer
#define relayPin 5
#define sensorPotPin 34 // Potensiometer untuk tekanan sensor
#define upPotPin 35     // Potensiometer untuk tekanan UP
#define downPotPin 32   // Potensiometer untuk tekanan DOWN

unsigned long relayStartTime = 0;
const unsigned long relayDuration = 10 * 60 * 1000; // 10 menit
bool isRelayOn = false;

// Moving Average
#define NUM_READINGS 10                                  // Jumlah pembacaan yang akan digunakan untuk rata-rata
int sensorReadings[NUM_READINGS];                        // Array untuk menyimpan pembacaan
int upReadings[NUM_READINGS];                            // Array untuk menyimpan pembacaan upPotPin
int downReadings[NUM_READINGS];                          // Array untuk menyimpan pembacaan downPotPin
int readIndex = 0;                                       // Indeks pembacaan saat ini
long totalSensor = 0, totalUp = 0, totalDown = 0;        // Total pembacaan untuk rata-rata
float averageSensor = 0, averageUp = 0, averageDown = 0; // Rata-rata nilai sensor

// Fungsi koneksi WiFi
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

// Fungsi koneksi MQTT
void connectToMqtt()
{
  while (!client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
  {
    delay(1000);
  }
  client.subscribe(MQTT_RELAY);
  client.publish(MQTT_STATUS, "Online", true, 1);
}

// Callback untuk pesan MQTT
void messageReceived(String &topic, String &payload)
{
  if (topic.equals(MQTT_RELAY))
  {
    if (payload.equalsIgnoreCase("ON"))
    {
      digitalWrite(relayPin, HIGH);
      isRelayOn = true;
      relayStartTime = millis();
      Serial.println("Relay ON");
    }
    else if (payload.equalsIgnoreCase("OFF"))
    {
      digitalWrite(relayPin, LOW);
      isRelayOn = false;
      Serial.println("Relay OFF");
    }
  }
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Inisialisasi array pembacaan dengan nilai 0
  for (int i = 0; i < NUM_READINGS; i++)
  {
    sensorReadings[i] = 0;
    upReadings[i] = 0;
    downReadings[i] = 0;
  }

  client.begin(MQTT_SERVER, 1883, espClient);
  client.setWill(MQTT_STATUS, WILL_MESSAGE, true, 1);
  connectToMqtt();
  client.onMessage(messageReceived);
}

// Variabel untuk interval waktu membaca tekanan
unsigned long previousMillis = 0;
const unsigned long pressureInterval = 1000; // Interval 1 detik

void loop()
{
  if (!client.connected())
  {
    connectToMqtt();
  }
  client.loop();

  // Logic untuk mematikan relay setelah 10 menit
  if (isRelayOn && millis() - relayStartTime >= relayDuration)
  {
    digitalWrite(relayPin, LOW);
    isRelayOn = false;
    client.publish(MQTT_RELAY, "OFF", true, 1);
    Serial.println("Relay OFF (Timeout)");
  }

  // Non-blocking delay menggunakan millis()
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= pressureInterval)
  {
    previousMillis = currentMillis;

    // Baca nilai potensiometer
    int sensorValue = analogRead(sensorPotPin);
    int upValue = analogRead(upPotPin);
    int downValue = analogRead(downPotPin);

    // Hapus pembacaan lama dari total
    totalSensor -= sensorReadings[readIndex];
    totalUp -= upReadings[readIndex];
    totalDown -= downReadings[readIndex];

    // Simpan pembacaan baru di array
    sensorReadings[readIndex] = sensorValue;
    upReadings[readIndex] = upValue;
    downReadings[readIndex] = downValue;

    // Tambahkan pembacaan baru ke total
    totalSensor += sensorReadings[readIndex];
    totalUp += upReadings[readIndex];
    totalDown += downReadings[readIndex];

    // Pindahkan ke indeks berikutnya
    readIndex = (readIndex + 1) % NUM_READINGS;

    // Hitung rata-rata
    averageSensor = totalSensor / NUM_READINGS;
    averageUp = totalUp / NUM_READINGS;
    averageDown = totalDown / NUM_READINGS;

    // Konversi nilai menjadi tekanan (contoh 0-50 PSI)
    float sensorPressure = map(averageSensor, 0, 4095, 0, 50);
    float upPressure = map(averageUp, 0, 4095, 0, 50);
    float downPressure = map(averageDown, 0, 4095, 0, 50);

    // Kirim data tekanan ke MQTT
    client.publish(MQTT_PRESSURE_SENSOR, String(sensorPressure).c_str());
    client.publish(MQTT_PRESSURE_UP, String(upPressure).c_str());
    client.publish(MQTT_PRESSURE_DOWN, String(downPressure).c_str());

    // Logika tambahan berdasarkan tekanan
    if (sensorPressure >= upPressure) // Tekanan terlalu tinggi
    {
      client.publish(MQTT_PRESSURE_UP, "HIGH");
    }
    else if (sensorPressure <= downPressure) // Tekanan terlalu rendah
    {
      client.publish(MQTT_PRESSURE_DOWN, "LOW");
    }

    Serial.print("Sensor Pressure: ");
    Serial.println(sensorPressure);
    Serial.print("UP Threshold: ");
    Serial.println(upPressure);
    Serial.print("DOWN Threshold: ");
    Serial.println(downPressure);
  }
}
