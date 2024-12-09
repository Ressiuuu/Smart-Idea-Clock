  #include <Arduino.h>
  #include <Wire.h>
  #include <LiquidCrystal_I2C.h>
  #include <DHT.h>
  #include <painlessMesh.h>

  // Konfigurasi LCD
  #define LCD_ADDR 0x27
  #define LCD_COLS 16
  #define LCD_ROWS 2
  LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

  // Konfigurasi DHT11
  #define DHTPIN 4
  #define DHTTYPE DHT11
  DHT dht(DHTPIN, DHTTYPE);

  // Konfigurasi painlessMesh
  #define MESH_PREFIX "SmartClockMesh"
  #define MESH_PASSWORD "password123"
  #define MESH_PORT 5555

  Scheduler userScheduler;
  painlessMesh mesh;

  // Variabel data sensor
  float temperature = 0;
  float humidity = 0;
  int airQuality = 0;

  void displayData() {

  .LO  lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperature);
    lcd.print("C H:");
    lcd.print(humidity);
    lcd.print("%");
    
    lcd.setCursor(0, 1);
    lcd.print("AirQ:");
    lcd.print(airQuality);
    delay(2000);
  }

  void sendDataToMaster() {
    DynamicJsonDocument jsonDoc(1024);
    jsonDoc["temperature"] = temperature;
    jsonDoc["humidity"] = humidity;
    jsonDoc["airQuality"] = airQuality;
    
    String message;
    serializeJson(jsonDoc, message);
    mesh.sendBroadcast(message);
  }

  void meshReceived(uint32_t from, String &msg) {
    Serial.printf("Message from %u: %s\n", from, msg.c_str());
  }

  void setup() {
    Serial.begin(115200);

    // Inisialisasi LCD
    lcd.init();
    lcd.backlight();
    lcd.print("Starting...");

    // Inisialisasi DHT
    dht.begin();

    // Inisialisasi mesh
    mesh.setDebugMsgTypes(ERROR | DEBUG); 
    mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
    mesh.onReceive(&meshReceived);
    
    // Tampilkan pesan awal
    lcd.clear();
    lcd.print("Smart Clock");
    delay(2000);
  }

  void loop() {
    mesh.update();
    
    // Baca data sensor
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    airQuality = random(0, 500); // Simulasi data polusi udara

    // Tampilkan di LCD
    displayData();

    // Kirim data ke root node
    sendDataToMaster();
  }
