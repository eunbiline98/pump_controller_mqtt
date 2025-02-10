#include <Arduino.h>
#include <LoRa.h>

// Pin LoRa
#define SS_PIN 5
#define RST_PIN 14
#define DIO0_PIN 2
#define RF95_FREQ 915E6 // Sesuaikan dengan wilayah

// Pin relay dan sensor
#define relayPin 27
#define sensorPotPin 32
#define upPotPin 34
#define downPotPin 35

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

bool isRelayOn = false;
unsigned long relayStartTime = 0;
const unsigned long relayDuration = 10 * 60 * 1000; // 10 menit

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000; // 1 detik

void setup()
{
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

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

  // Kirim data setiap 1 detik
  if (currentMillis - lastSendTime >= sendInterval)
  {
    lastSendTime = currentMillis;

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

    // Kirim data tekanan ke Gateway
    String data = "P:" + String(sensorPressure) + " UP:" + String(upPressure) + " DOWN:" + String(downPressure);
    LoRa.beginPacket();
    LoRa.print(data);
    LoRa.endPacket();

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
      digitalWrite(relayPin, LOW);
      // isRelayOn = true;
      // relayStartTime = currentMillis;
      Serial.println("Relay ON");
    }
    else if (receivedMessage.equalsIgnoreCase("OFF"))
    {
      digitalWrite(relayPin, HIGH);
      // isRelayOn = false;
      Serial.println("Relay OFF");
    }
  }

  // Matikan relay setelah 10 menit
  // if (isRelayOn && currentMillis - relayStartTime >= relayDuration)
  // {
  //   digitalWrite(relayPin, LOW);
  //   isRelayOn = false;
  //   Serial.println("Relay OFF (Timeout)");
  // }
}
