# Pressure Sensor MQTT Relay Controller

Proyek ini menggunakan ESP32 untuk menghubungkan sensor tekanan potensiometer (3 potensiometer) ke broker MQTT. Relay dikendalikan melalui MQTT dan tekanan dari sensor dibaca serta dikirim ke broker MQTT.

## Fitur

- **WiFi dan MQTT**: Koneksi ke jaringan WiFi dan broker MQTT.
- **Relay Control**: Menghidupkan dan mematikan relay melalui pesan MQTT.
- **Pressure Sensor**: Membaca nilai dari tiga potensiometer yang digunakan untuk mengukur tekanan, dengan data dikirim melalui MQTT.
- **Moving Average**: Menggunakan rata-rata bergerak untuk mendapatkan nilai tekanan yang lebih stabil.
- **Timeout Relay**: Relay akan dimatikan otomatis setelah 10 menit.
- **Threshold Monitoring**: Mengirimkan status "HIGH" atau "LOW" jika tekanan terlalu tinggi atau rendah.

## Persyaratan

- **ESP32**: Mikrokontroler yang digunakan untuk proyek ini.
- **Arduino IDE**: Untuk memprogram ESP32.
- **Broker MQTT**: Seperti Mosquitto atau broker MQTT lainnya.
- **WiFi**: Koneksi WiFi untuk menghubungkan ESP32 ke jaringan.
- **MQTT Client**: Menggunakan pustaka `MQTT.h` untuk koneksi MQTT.

## Instalasi

### 1. Siapkan ESP32 di Arduino IDE
   - Pastikan Anda sudah menambahkan board ESP32 di Arduino IDE.
   - Pilih board **ESP32 Dev Module** di **Tools > Board**.

### 2. Instalasi Pustaka
   - Instal pustaka berikut di Arduino IDE:
     - `WiFi.h` untuk koneksi WiFi.
     - `MQTT.h` untuk koneksi ke broker MQTT.

### 3. Koneksi WiFi dan MQTT
   - Masukkan detail SSID, password WiFi, dan informasi broker MQTT seperti nama server, client ID, username, dan password pada variabel yang telah disediakan di bagian konfigurasi awal.

### 4. Upload Program ke ESP32
   - Hubungkan ESP32 ke komputer dan pilih port yang sesuai di **Tools > Port**.
   - Klik **Upload** untuk mengunggah program ke ESP32.

## Konfigurasi MQTT

- **MQTT_TOPIC_RELAY**: Topik untuk kontrol relay, mengirimkan `ON` atau `OFF` untuk menghidupkan atau mematikan relay.
- **MQTT_TOPIC_PRESSURE_SENSOR**: Topik untuk mengirim data tekanan sensor (nilai sensor yang sudah dihitung).
- **MQTT_TOPIC_PRESSURE_UP**: Topik untuk tekanan "UP" (nilai potensiometer untuk batas atas).
- **MQTT_TOPIC_PRESSURE_DOWN**: Topik untuk tekanan "DOWN" (nilai potensiometer untuk batas bawah).

## Fungsi Utama

- **Relay Control**: Relay akan dikendalikan melalui pesan MQTT, menerima perintah `ON` atau `OFF` pada topik yang sesuai.
- **Sensor Pressure**: Nilai tekanan dihitung berdasarkan pembacaan potensiometer dan dikirimkan ke broker MQTT dalam bentuk nilai (dalam satuan PSI).
- **Threshold Monitoring**: Mengirimkan status "HIGH" atau "LOW" jika tekanan sensor lebih besar dari atau lebih kecil dari batas atas dan bawah yang ditentukan.

## Pembacaan Tekanan

- Pembacaan dari sensor tekanan dilakukan menggunakan tiga potensiometer, satu untuk nilai sensor utama dan dua lainnya untuk batas atas dan bawah.
- Pembacaan ini dilakukan setiap detik dan hasilnya dikirimkan ke broker MQTT.
- Data tekanan diproses dengan menggunakan rata-rata bergerak untuk memperhalus hasil pembacaan.

## Pengaturan Relay

- Relay akan tetap hidup selama 10 menit setelah diaktifkan. Setelah itu, relay akan mati secara otomatis.
- Perintah untuk menghidupkan atau mematikan relay dikirimkan melalui pesan MQTT.

## Penggunaan

1. **Aktifkan Relay**: Mengirimkan pesan `ON` ke topik `MQTT_RELAY`.
2. **Nonaktifkan Relay**: Mengirimkan pesan `OFF` ke topik `MQTT_RELAY`.
3. **Pemantauan Tekanan**: Tekanan sensor akan dikirimkan secara berkala ke topik `MQTT_PRESSURE_SENSOR`, dan status tekanan (`HIGH` atau `LOW`) akan dikirimkan ke topik `MQTT_PRESSURE_UP` atau `MQTT_PRESSURE_DOWN` sesuai kondisi.

## Troubleshooting

- Pastikan ESP32 terhubung ke jaringan WiFi dengan benar.
- Periksa koneksi ke broker MQTT dan pastikan kredensial yang dimasukkan sudah benar.
- Pastikan relay dan sensor potensiometer terhubung dengan benar ke pin yang sesuai pada ESP32.

## Lisensi

Proyek ini menggunakan lisensi MIT. Silakan lihat file `LICENSE` untuk detail lebih lanjut.
