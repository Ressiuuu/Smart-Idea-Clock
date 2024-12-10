#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#define MAX_ALARM 5
#define PIN_POLLUTION 17

/* ===== Constant Definitions ==== */
const String COUNTRY_CODES[13] = {
  "cn",  // China
  "hk",  // Hong Kong
  "id",  // Indonesia
  "jp",  // Japan
  "kr",  // Korea
  "my",  // Malaysia
  "ph",  // Philippines
  "ps",  // Palestinian Territory
  "sa",  // Saudi Arabia
  "sg",  // Singapore
  "th",  // Thailand
  "tw",  // Taiwan
  "vn",  // Vietnam
};

typedef struct SensorData {
  float T;
  float H;
} SensorData;

typedef struct AlarmItem {
  int8_t index;
  char label[100];
  char time[10];
} AlarmItem;
/* ===== Constant Definitions ==== */


/* ===== Function Definitions ==== */
// FreeRTOS
void taskUpdateTime(void* parameters);
void taskAirPollutionSensor(void* parameters);

// ESP-NOW
void OnDataRecv(const esp_now_recv_info_t* info, const uint8_t* incomingData, int len);

// Web Server
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

void webGetAlarm();
void webAddAlarm();
void webDeleteAlarm();
void parseNewAlarm(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total);
void parseDeleteAlarm(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total);
void webDashboard();

void startWebServer();
/* ===== Function Definitions ==== */


/* ===== Variable Declarations ==== */
// FreeRTOS TaskHandle
TaskHandle_t taskHandleUpdateTime;
TaskHandle_t taskHandleAirPollutionSensor;

String currentCC = COUNTRY_CODES[2];  // Indonesia
const char ntpServer[] = "id.pool.ntp.org";

String currentTime;
AlarmItem alarmLists[MAX_ALARM];
int alarmIndex = 0;

SensorData sensorData;

WebSocketsServer webSocketTime = WebSocketsServer(81);
WebSocketsServer webSocketDht = WebSocketsServer(82);
WebSocketsServer webSocketPollution = WebSocketsServer(83);
AsyncWebServer server(80);
/* ===== Variable Declarations ==== */


/* ===== Setup ==== */
void setup() {
  /* Starting Serial Monitor */
  Serial.begin(115200);
  delay(1000);

  for (int i = 0; i < MAX_ALARM; i++) {
    AlarmItem* tmp = (AlarmItem*)malloc(sizeof(AlarmItem));

    tmp->index = -1;
    stpcpy(tmp->label, "");
    stpcpy(tmp->time, "");

    alarmLists[i] = *tmp;

    free(tmp);
  }

  /* ====== Wifi Setup ====== */
  WiFi.mode(WIFI_STA);              // Stationary mode
  WiFi.setChannel(WiFi.channel());  // Channel is important for ESP-NOW

  WiFiManager wifiManager;  // Using WiFiManager for dynamic WiFi
  wifiManager
    .autoConnect("SmartClock ESP Main");  // WiFi AP with name
  /* ====== Wifi Setup ====== */

  /* ====== DNS Setup ====== */
  // Akses ke web lewat http://smartclock18.local
  // Kalo gabisa akses, pastikan menggunakan DNS
  // Cloudflare 1.1.1.1 pada jaringan
  if (!MDNS.begin("smartclock18")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(500);
    }
  }
  Serial.println("Access the dashboard via: http://smartclock18.local");
  /* ====== DNS Setup ====== */


  /* Config NTP Server for clock */
  configTime(25200, 0, ntpServer);
  xTaskCreate(taskUpdateTime, "Task Update Time", 2048, NULL, 1, &taskHandleUpdateTime);

  /* Config MQ-2 */
  pinMode(PIN_POLLUTION, INPUT);
  analogSetAttenuation(ADC_11db);
  Serial.println("Warming up Sensor");
  xTaskCreate(taskAirPollutionSensor, "Task Update Time", 2048, NULL, 1, &taskHandleAirPollutionSensor);

  /* ====== ESP-NOW Setup ====== */
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Register callback function to receive ESP-NOW data
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  Serial.println("ESP NOW Initialized!");
  /* ====== ESP-NOW Setup ====== */


  /* ====== WebSocket Setup ====== */
  webSocketTime.begin();
  webSocketDht.begin();
  webSocketPollution.begin();

  startWebServer();
}

void loop() {
  webSocketTime.loop();
  webSocketDht.loop();
  webSocketPollution.loop();
}

void taskUpdateTime(void* parameters) {
  while (1) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }

    char buffer[50];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);

    currentTime = String(buffer);

    webSocketTime.broadcastTXT(currentTime);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}

void taskAirPollutionSensor(void* parameters) {
  while (1) {
    int gasValue = analogRead(PIN_POLLUTION);

    String value = std::to_string(gasValue).c_str();

    webSocketPollution.broadcastTXT(value);
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}

void webDashboard() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html lang="id">

      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">

        <title>SmartClock</title>

        <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
        <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" rel="stylesheet">

        <style>
          :root {
            --color-lightest: #D3F1DF;
            --color-light: #85A98F;
            --color-medium: #5A6C57;
            --color-dark: #525B44;
            --font-family: 'Inter', sans-serif;
          }

          * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: var(--font-family);
            /* border: black solid 1px; */
          }

          *::selection {
            background-color: var(--color-dark);
            color: var(--color-lightest);
          }

          body {
            background-color: var(--color-lightest);
            color: var(--color-dark);
            line-height: 1.6;
          }

          .button {
            padding: 0.5rem 1rem;
            border: none;
            border-radius: 0.5rem;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.3s ease;
            border: var(--color-dark) solid 2px;
          }

          .button-outline {
            background-color: var(--color-lightest);
            color: var(--color-dark);
            border: var(--color-dark) solid 2px;
          }

          .button-outline:hover {
            background-color: var(--color-medium);
            color: white;
          }

          .modal {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background-color: rgba(0, 0, 0, 0.5);
          }

          .modal-content {
            background-color: white;
            padding: 2rem;
            border-radius: 1rem;
            width: 90%;
            max-width: 500px;
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
          }

          .modal-title {
            font-size: 1.5rem;
            font-weight: 600;
            margin-bottom: 1.5rem;
            color: var(--color-dark);
          }

          .form-group {
            margin-bottom: 1rem;
          }

          .form-group label {
            display: block;
            margin-bottom: 0.5rem;
            color: var(--color-medium);
          }

          .form-control {
            width: 100%;
            padding: 0.5rem;
            border: 1px solid var(--color-light);
            border-radius: 0.5rem;
            font-size: 1rem;
          }

          .modal-footer {
            display: flex;
            justify-content: flex-end;
            gap: 1rem;
            margin-top: 1.5rem;
          }

          @media (max-width: 768px) {
            .grid {
              grid-template-columns: 1fr;
            }
          }
        </style>
      </head>

      <body>
        <style id="navbar-style">
          .navbar {
            width: 100%;
            background-color: var(--color-light);
            padding: 1rem 2rem;
            border-bottom: var(--color-dark) solid 2px;
          }

          .navbar-content {
            max-width: 1200px;
            margin: 0 auto;
            display: flex;
            justify-content: space-between;
            align-items: center;
          }

          .navbar-logo {
            font-size: 2rem;
            font-weight: 700;
            letter-spacing: -3.5px;
            color: white;
            background-color: var(--color-medium);
            padding: 2px 10px;
            border-radius: 4px;
          }

          .navbar-buttons {
            display: flex;
            gap: 1rem;
          }

          .navbar-button {
            padding: 0.5rem 1rem;
            border-radius: 0.5rem;
            font-weight: bold;
            font-size: 1rem;
            cursor: pointer;
            transition: all 0.3s ease;
          }
        </style>
        <nav class="navbar">
          <div class="navbar-content">
            <div class="navbar-logo">SmartClock</div>
            <div class="navbar-buttons">
              <!-- <button class="navbar-button button-outline" onclick="modalOpen('timezone-modal')">Timezone</button> -->
              <button class="navbar-button button-outline" onclick="modalOpen('alarm-modal')">New Alarm</button>
            </div>
          </div>
        </nav>

        <style id="container style">
          .container {
            width: 100%;
            max-width: 1200px;
            display: flex;
            flex-direction: column;
            flex-wrap: wrap;
            align-items: center;
            justify-content: center;
            gap: 12px;
            margin: 0 auto;
            overflow: hidden;
          }

          .sub-container {
            display: flex;
            justify-content: center;
            gap: 12px;
            width: 100%;
            overflow: hidden;
          }

          .card {
            background-color: var(--color-light);
            display: flex;
            flex-direction: column;
            flex: 1;
            width: 100%;
            max-height: 600px;
            padding: 12px 24px;
            border-radius: 6px;
            border: var(--color-dark) solid 2px;
          }

          .card-title {
            color: var(--color-dark);
            background-color: var(--color-lightest);
            width: fit-content;
            letter-spacing: -1px;
            margin: 0 0 12px 0;
            border-radius: 4px;
            padding: 0 6px;
            border: var(--color-dark) solid 2px;
          }
        </style>
        <div class="container">
          <div class="sub-container" style="margin-top: 2rem;">
            <div id="Sensor" class="card">
              <h2 class="card-title">Kondisi Ruangan</h2>

              <style id="environment-style">
                .environment-card {
                  display: flex;
                  justify-content: space-between;
                  flex-wrap: wrap;
                  gap: 12px;
                  width: 100%;
                  height: 100%;
                }

                .env-metric {
                  display: flex;
                  flex: 1;
                  flex-direction: column;
                  align-items: center;
                  width: 100%;
                  background: var(--color-lightest);
                  padding: 1.5rem;
                  border-radius: 0.75rem;
                  text-align: center;
                  transition: transform 0.2s;
                  border: var(--color-dark) solid 2px;
                }

                .env-metric:hover {
                  transform: translateY(-4px);
                  box-shadow: 0 0 4px var(--color-dark);
                }

                .metric-header {
                  display: flex;
                  align-items: center;
                  gap: 12px;
                  margin-bottom: 8px;
                }

                .env-icon {
                  color: var(--color-medium);
                  font-size: 2rem;
                }

                .env-label {
                  font-size: 1.5rem;
                  font-weight: bold;
                  color: var(--color-medium);
                }

                .env-value-container {
                  width: 100%;
                  height: 100%;
                  display: flex;
                  align-items: center;
                  justify-content: center;
                }

                .env-value {
                  color: var(--color-dark);
                  background-color: white;
                  font-size: 2rem;
                  font-weight: 900;
                  padding: 42px 32px;
                  border: var(--color-dark) solid 2px;
                  border-radius: 100%;
                }
              </style>
              <div class="environment-card">
                <div class="env-metric">
                  <div class="metric-header">
                    <i class="fas fa-temperature-high env-icon"></i>
                    <div class="env-label">Suhu</div>
                  </div>

                  <div class="env-value-container">
                    <p class="env-value" id="temperature">
                      28°C
                    </p>
                  </div>
                </div>

                <div class="env-metric">
                  <div class="metric-header">
                    <i class="fas fa-tint env-icon"></i>
                    <div class="env-label">Kelembaban</div>
                  </div>

                  <div class="env-value-container">
                    <p class="env-value" id="humidity">
                      65%
                    </p>
                  </div>
                </div>

                <div class="env-metric">
                  <div class="metric-header">
                    <i class="fas fa-cloud env-icon"></i>
                    <div class="env-label">PPM</div>
                  </div>

                  <div class="env-value-container">
                    <p class="env-value" id="pollution" style="min-width: 150px;">
                      234
                    </p>
                  </div>
                </div>
              </div>
            </div>

            <div id="Clock" class="card">
              <style id="clock-style">
                .timezone-container {
                  display: flex;
                  align-items: start;
                  justify-content: space-between;
                  gap: 12px;
                }

                .clock-timezone {
                  font-weight: 300;
                  font-size: 1.5rem;
                  padding: 0 12px;
                }

                .clock {
                  display: flex;
                  align-items: center;
                  justify-content: center;
                  padding: 12px 0;
                  font-weight: bold;
                  font-size: 2.5rem;
                  color: var(--color-medium);
                  background-color: white;
                  border-radius: 12px;
                  border: var(--color-dark) solid 2px;
                }
              </style>

              <div class="timezone-container">
                <h2 class="card-title">Jam</h2>

                <div class="clock-timezone card-title" id="clock-timezone">Indonesia (WIB)</div>
              </div>

              <div class="clock" id="clock">
                13:23:22
              </div>

              <style id="alarm-style">
                .alarms-list {
                  margin-top: 1rem;
                  border-top: 2px solid var(--color-lightest);
                  padding-top: 1rem;
                  overflow: hidden;
                }

                .alarm-item {
                  display: flex;
                  justify-content: space-between;
                  align-items: center;
                  padding: 0.75rem;
                  background: var(--color-lightest);
                  border-radius: 0.5rem;
                  margin-bottom: 0.5rem;
                  border: var(--color-dark) solid 2px;
                }

                .alarm-info {
                  display: flex;
                  align-items: center;
                  gap: 0.75rem;
                }

                .alarm-time {
                  font-weight: 600;
                  color: var(--color-dark);
                }

                .alarm-label {
                  color: var(--color-medium);
                }

                .alarm-actions {
                  display: flex;
                  gap: 0.5rem;
                }

                .alarms-container {
                  height: 100%;
                  overflow-y: auto;
                  padding-bottom: 56px;
                }
              </style>
              <div class="alarms-list">
                <h2 class="card-title">Active Alarms</h2>

                <div id="alarms-container" class="alarms-container">
                </div>
              </div>
            </div>
          </div>
        </div>

        <div id="timezone-modal" class="modal">
          <div class="modal-content">
            <h2 class="modal-title">Select Timezone</h2>
            <div class="form-group">
              <select class="form-control" id="timezone-select">
                <option value="0">China</option>
                <option value="1">Hong Kong</option>
                <option value="2">Indonesia</option>
                <option value="3">Japan</option>
                <option value="4">Korea</option>
                <option value="5">Malaysia</option>
                <option value="6">Philippines</option>
                <option value="7">Palestinian Territory</option>
                <option value="8">Saudi Arabia</option>
                <option value="9">Singapore</option>
                <option value="10">Thailand</option>
                <option value="11">Taiwan</option>
                <option value="12">Vietnam</option>
              </select>
            </div>
            <div class="modal-footer">
              <button class="button button-outline" onclick="modalClose('timezone-modal')">Cancel</button>
              <button class="button button-outline" onclick="saveTimezone()">Save</button>
            </div>
          </div>
        </div>

        <div id="alarm-modal" class="modal">
          <div class="modal-content">
            <h2 class="modal-title">Create New Alarm</h2>
            <div class="form-group">
              <label>Time</label>
              <input type="time" class="form-control" id="alarm-time">
            </div>
            <div class="form-group">
              <label>Label</label>
              <input type="text" class="form-control" id="alarm-label" maxlength="32" placeholder="Alarm label">
            </div>
            <div class="modal-footer">
              <button class="button button-outline" onclick="modalClose('alarm-modal')">Cancel</button>
              <button class="button button-outline" onclick="addAlarm()">Create</button>
            </div>
          </div>
        </div>

        
        <div id="player" style="display: none;"></div>
        
        <script src="https://www.youtube.com/iframe_api"></script>
        <script>
          <!-- Audio -->
          let player;
          let isReady = false;
          
          // Initialize YouTube player
          function onYouTubeIframeAPIReady() {
              player = new YT.Player('player', {
                  height: '0',
                  width: '0',
                  videoId: 'rUkzZTGE6jI', // Replace with your desired YouTube video ID
                  playerVars: {
                      'autoplay': 0,
                      'controls': 0
                  },
                  events: {
                      'onReady': onPlayerReady
                  }
              });
          }
          
          // When player is ready
          function onPlayerReady(event) {
            isReady = true;
          }

          <!-- Main -->
          let alarmLists = [];

          let webSocketTime = new WebSocket('ws://' + window.location.hostname + ':81/');
          let webSocketDht = new WebSocket('ws://' + window.location.hostname + ':82/');
          let webSocketMQ = new WebSocket('ws://' + window.location.hostname + ':83/');
          webSocketTime.onmessage = function (event) {
            document.getElementById("clock").innerHTML = event.data;

            if (!player) return;
            for (let i = 0; i < alarmLists.length; i++) {
              if (alarmLists[i].time == event.data) player.playVideo();
            }
          }

          webSocketDht.onmessage = function (event) {
            let data = JSON.parse(event.data);

            document.getElementById("temperature").innerHTML = data.T + "°C";
            document.getElementById("humidity").innerHTML = data.H + "%";
          }

          webSocketMQ.onmessage = function (event) {
            document.getElementById("pollution").innerHTML = event.data;
          }

          async function getAlarm() {
            const res = await fetch(`http://${window.location.hostname}/alarm`, {
                headers: {
                  'Accept': 'application/json',
                  'Content-Type': 'application/json'
                }
              });
            
            let data = await res.json();

            alarmLists = data.alarms;

            const container = document.getElementById('alarms-container');
            container.innerHTML = data.alarms.map((alarm, index) => alarm.id == -1 ? "" : `
                  <div class="alarm-item">
                      <div class="alarm-info">
                          <i class="fas fa-clock"></i>
                          <span class="alarm-time">${alarm.time}</span>
                          <span class="alarm-label">${alarm.label}</span>
                      </div>
                      <div class="alarm-actions">
                          <i class="fas fa-trash" style="cursor: pointer; color: var(--color-medium);"
                            onclick="deleteAlarm(${alarm.id})"></i>
                      </div>
                  </div>
              `).join('');
          }

          async function addAlarm() {
            const time = document.getElementById("alarm-time").value + ":00";
            const label = document.getElementById("alarm-label").value;

            if (!time || !label) return;

            await fetch(`http://${window.location.hostname}/alarm`, {
              method: 'POST',
              headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json'
              },
              body: JSON.stringify({ time, label })
            });

            await getAlarm();

            document.getElementById("alarm-time").value = "";
            document.getElementById("alarm-label").value = "";
            modalClose('alarm-modal');
          }

          const deleteAlarm = async (index) => {
            await fetch(`http://${window.location.hostname}/delete`, {
              method: 'POST',
              headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json'
              },
              body: JSON.stringify({ index })
            });

            await getAlarm();
          }

          getAlarm();

          const modalOpen = (modalId) => {
            document.getElementById(modalId).style.display = 'block';
          }

          const modalClose = (modalId) => {
            document.getElementById(modalId).style.display = 'none';
          }

          window.onclick = function (event) {
            if (event.target.classList.contains('modal')) {
              event.target.style.display = 'none';
            }
          }
        </script>
      </body>

      </html>
    )rawliteral");
    });
}

void webGetAlarm() {
  server.on("/alarm", HTTP_GET, [](AsyncWebServerRequest* request) {
    String json = "{";
    json += "\"alarms\": [";
    for (int i = 0; i < MAX_ALARM; i++) {
      json += "{\"id\":\"" + String(alarmLists[i].index) + "\"";
      json += ",\"time\":\"" + String(alarmLists[i].time) + "\"";
      json += ",\"label\":\"" + String(alarmLists[i].label) + "\"}";
      if (i < MAX_ALARM - 1) json += ",";
    }
    json += "]}";

    request->send(200, "application/json", json);
    });
}

void webAddAlarm() {
  server.on("/alarm", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "Success!");
    },
    nullptr, parseNewAlarm);
}

void webDeleteAlarm() {
  server.on("/delete", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", "Success!");
    },
    nullptr, parseDeleteAlarm);
}

void parseNewAlarm(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
  if (alarmIndex >= MAX_ALARM) {
    req->send_P(403, "application/json", "Max Alarm Reached!");
  }

  for (int i = 0; i < MAX_ALARM; i++) {
    if (alarmLists[i].index == -1) {
      alarmIndex = i;
      break;
    }
  }

  AlarmItem* newAlarm = NULL;
  DynamicJsonDocument body(1024);

  deserializeJson(body, data, len);

  newAlarm = (AlarmItem*)pvPortMalloc(sizeof(AlarmItem));

  strcpy(newAlarm->label, body["label"]);
  strcpy(newAlarm->time, body["time"]);
  newAlarm->index = alarmIndex;

  alarmLists[alarmIndex] = (*newAlarm);
}

void parseDeleteAlarm(AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
  DynamicJsonDocument body(1024);

  deserializeJson(body, data, len);

  int idx = body["index"];

  alarmLists[idx].index = -1;
  // alarmLists[idx].label = "";
  // alarmLists[idx].time = "";
}

void startWebServer() {
  // DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  webDashboard();
  webGetAlarm();
  webAddAlarm();
  webDeleteAlarm();

  // Start the server
  Serial.println("Starting the web server...");
  server.begin();
  Serial.println("Web server started.");
}

void OnDataRecv(const esp_now_recv_info_t* info, const uint8_t* incomingData, int len) {
  memcpy(&sensorData, incomingData, sizeof(sensorData));

  // Serial.printf("T: %.2f C\n", sensorData.T);
  // Serial.printf("H: %.2f %\n", sensorData.H);

  String json = "{";
  json += "\"T\":\"" + String((int)sensorData.T) + "\"";
  json += ",\"H\":\"" + String((int)sensorData.H) + "\"}";

  webSocketDht.broadcastTXT(json);
}
