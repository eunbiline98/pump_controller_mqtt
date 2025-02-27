#include <Arduino.h>
#include <LoRa.h>

// Pin LoRa
#define SS_PIN 5
#define RST_PIN 14
#define DIO0_PIN 2
#define RF95_FREQ 915E6 // Sesuaikan dengan wilayah

// Pin relay dan sensor
#define RELAY_PIN 27
#define PRESURE_PIN 32
#define UP_POT_PIN 34
#define DOWN_POT_PIN 35
#define BUTTON_PIN 15

// Indicator
#define LED_LORA 25
#define LED_RELAY 26

// Konstanta untuk perhitungan rata-rata
#define NUM_READINGS 10
int sensorReadings[NUM_READINGS] = {0};
int upReadings[NUM_READINGS] = {0};
int downReadings[NUM_READINGS] = {0};
int readIndex = 0;
long totalSensor = 0;
long totalUp = 0;
long totalDown = 0;
int averageSensor = 0;
int averageUp = 0;
int averageDown = 0;

// Kontrol relay dari tombol push on
static bool lastButtonState = HIGH;
static unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // 50ms debounce

bool isRelayOn = false;
unsigned long relayStartTime = 0;
const unsigned long relayDuration = 10 * 60 * 1000; // 10 menit

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000; // 1 detik

void setup()
{
  Serial.begin(115200);

  pinMode(LED_RELAY, OUTPUT);
  pinMode(LED_LORA, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_RELAY, LOW);
  digitalWrite(LED_LORA, LOW);

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
  unsigned long currentMillis = millis();

  // *** Kirim data setiap 1 detik ***
  if (currentMillis - lastSendTime >= sendInterval)
  {
    lastSendTime = currentMillis;

    int sensorValue = analogRead(PRESURE_PIN);
    int upValue = analogRead(UP_POT_PIN);
    int downValue = analogRead(DOWN_POT_PIN);

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

    // Tambahkan status relay ke data yang dikirim
    String relayStatus = isRelayOn ? "ON" : "OFF";
    String data = "P:" + String(sensorPressure) + " UP:" + String(upPressure) + " DOWN:" + String(downPressure) + " RELAY:" + relayStatus;

    // LED_LORA menyala sebelum kirim data
    digitalWrite(LED_LORA, HIGH);

    // Kirim data ke Gateway
    LoRa.beginPacket();
    LoRa.print(data);
    LoRa.endPacket();

    // Matikan LED_LORA setelah 100ms
    delay(100);
    digitalWrite(LED_LORA, LOW);

    Serial.println("Data terkirim ke Gateway: " + data);
  }

  // Cek data dari gateway
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0)
  {
    String receivedMessage = "";
    while (LoRa.available())
    {
      receivedMessage += (char)LoRa.read();
    }

    Serial.println("Perintah dari gateway: " + receivedMessage);

    if (receivedMessage.equalsIgnoreCase("ON"))
    {
      digitalWrite(RELAY_PIN, HIGH); // Nyalakan Relay
      digitalWrite(LED_RELAY, HIGH);
      isRelayOn = true;
      relayStartTime = currentMillis;
      Serial.println("Relay ON dari gateway");
    }
    else if (receivedMessage.equalsIgnoreCase("OFF"))
    {
      digitalWrite(RELAY_PIN, LOW); // Matikan Relay
      digitalWrite(LED_RELAY, LOW);
      isRelayOn = false;
      Serial.println("Relay OFF dari gateway");
    }
  }

  // Matikan relay setelah 10 menit
  if (isRelayOn && currentMillis - relayStartTime >= relayDuration)
  {
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_RELAY, LOW); // Matikan LED relay
    isRelayOn = false;
    Serial.println("Relay OFF (Timeout 10 menit)");
  }

  // Kontrol relay dari tombol push on
  // bool buttonState = digitalRead(BUTTON_PIN);

  // if (buttonState != lastButtonState)
  // {
  //   lastDebounceTime = currentMillis; // Reset debounce timer
  // }

  // if ((currentMillis - lastDebounceTime) > debounceDelay)
  // {
  //   if (buttonState == LOW && lastButtonState == HIGH)
  //   {                         // Tombol ditekan
  //     isRelayOn = !isRelayOn; // Toggle relay
  //     digitalWrite(RELAY_PIN, isRelayOn ? HIGH : LOW);
  //     digitalWrite(LED_RELAY, isRelayOn ? HIGH : LOW);
  //     Serial.println(isRelayOn ? "Relay ON dari tombol!" : "Relay OFF dari tombol!");
  //     relayStartTime = isRelayOn ? currentMillis : 0; // Reset timer jika OFF
  //   }
  // }

  // lastButtonState = buttonState; // Simpan status tombol sebelumnya
}
