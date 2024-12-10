#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define MQ135_PIN 36

TaskHandle_t TaskReadSensorHandle;
TaskHandle_t TaskProcessDataHandle;

volatile int sensorValue = 0;
volatile int airQuality = 0;
int sensitivity = 200;

void TaskReadSensor(void *pvParameters) {
    while (true)
    {
        sensorValue = analogRead(MQ135_PIN);
        vTaskDelay(500 / portTICK_RATE_MS);
    }
}

void TaskProcessData(void *pvParameters) {
    while (true) {
        airQuality = sensor_value * sensitivity / 1023;

        Serial.print("Nilai sensor: ");
        Serial.println(sensorValue);
        Serial.print("Nilai polusi udara: ");
        Serial.println(airQuality);

        if (sensorValue < 50) {
            Serial.println("Kualitas Udara: Sangat Baik");
        }
        else if (sensorValue < 100) {
            Serial.println("Kualitas Udara: Baik");
        }
        else if (sensorValue < 150) {
            Serial.println("Kualitas Udara: Sedang");
        }
        else if (sensorValue < 200) {
            Serial.println("Kualitas Udara: Buruk")
        }
        else {
            Serial.println("Kualitas Udara: Sangat Buruk");
        }
        vTaskDelay(500 / portTICK_RATE_MS);
    }
}

void setup() {
    Serial.begin(9600);
    pinMode(MQ135_PIN, INPUT);

    xTaskCreate(
        TaskReadSensor,
        "TaskReadSensor",
        1000,
        NULL,
        1,
        &TaskReadSensorHandle
    );

    xTaskCreate(
        TaskProcessData,
        "TaskProcessData",
        1000,
        NULL,
        1,
        &TaskProcessDataHandle
    );
}

void loop() {
}
