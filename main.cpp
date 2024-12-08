#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

// ----------- Konfigurasi DHT Sensor -----------
#define DHTPIN 4        // Pin DHT11
#define DHTTYPE DHT11   // Tipe DHT11

DHT dht(DHTPIN, DHTTYPE);

// ----------- Struktur Data untuk ESP-NOW -----------
typedef struct SensorData {
  float T; // Suhu
  float H; // Kelembaban
} SensorData;

SensorData sensorData;

// ----------- MAC Address ESP1 -----------
uint8_t esp1Address[] = {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF}; // Ganti dengan MAC Address ESP1

// ----------- Callback untuk ESP-NOW -----------
void onSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

// ----------- Fungsi untuk Membaca Sensor -----------
void readSensors() {
  // Membaca suhu dan kelembaban dari DHT11
  sensorData.T = dht.readTemperature();
  sensorData.H = dht.readHumidity();

  // Log data sensor ke Serial Monitor
  Serial.printf("Temperature: %.1f °C, Humidity: %.1f %%\n", 
                sensorData.T, sensorData.H);
}

// ----------- Setup Program -----------
void setup() {
  Serial.begin(115200);

  // Inisialisasi DHT11
  dht.begin();

  // Inisialisasi WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Inisialisasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  esp_now_register_send_cb(onSent);

  // Tambahkan peer (ESP1)
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, esp1Address, 6);
  peerInfo.channel = 0; // Gunakan channel default
  peerInfo.encrypt = false; // Non-enkripsi
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESP2 Initialized and Ready to Send Data");
}

// ----------- Loop Program -----------
void loop() {
  // Membaca data dari sensor
  readSensors();

  // Mengirimkan data ke ESP1
  esp_err_t result = esp_now_send(esp1Address, (uint8_t *)&sensorData, sizeof(sensorData));
  if (result == ESP_OK) {
    Serial.println("Data sent successfully");
  } else {
    Serial.println("Error sending data");
  }

  delay(2000); // Interval pengiriman data
}
