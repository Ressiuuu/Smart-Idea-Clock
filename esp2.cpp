#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
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
uint8_t broadcastAddress[] = { 0x3c, 0x84, 0x27, 0xc9, 0x55, 0x64 };

// ----------- Variabel WiFi Channel -----------
uint8_t WIFI_CHANNEL = 1; // Default channel
bool channelConnected = false;

// ----------- Callback untuk ESP-NOW -----------
void OnDataSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");

  if (status == ESP_NOW_SEND_FAIL && !channelConnected) {
    Serial.println("Trying other WiFi Channel...");
    Serial.printf("Current Channel: %d\n", WIFI_CHANNEL);

    // Perbarui channel WiFi
    WIFI_CHANNEL = (WIFI_CHANNEL == 14) ? 1 : WIFI_CHANNEL + 1;

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    Serial.printf("Trying Channel: %d\n", WIFI_CHANNEL);
  } else {
    channelConnected = true;
  }
}

// ----------- Fungsi untuk Membaca Sensor -----------
void readSensors() {
  // Membaca suhu dan kelembaban dari DHT11
  sensorData.T = dht.readTemperature();
  sensorData.H = dht.readHumidity();

  // Log data sensor ke Serial Monitor
  Serial.printf("Temperature: %.1f, Humidity: %.1f%\n", 
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

  // Atur channel WiFi
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Log MAC Address ESP2
  Serial.println("ESP2 MAC Address: " + WiFi.macAddress());

  // Inisialisasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  // Tambahkan peer (ESP1)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo)); // Pastikan peerInfo diinisialisasi
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0; // Default channel
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
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&sensorData, sizeof(sensorData));
  if (result == ESP_OK) {
    Serial.println("Data sent successfully");
  } else {
    Serial.println("Error sending data");
  }

  delay(2000); // Interval pengiriman data
}