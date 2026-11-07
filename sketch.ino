
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Khai bao chan ket noi
#define MQ2_PIN 34
#define DHT_PIN 4
#define FAN_PIN 18
#define PUMP_PIN 19
#define DHT_TYPE DHT22

// WiFi Wokwi
const char *SSID = "Wokwi-GUEST";
const char *PASS = "";

// MQTT Broker
const char *BROKER = "broker.emqx.io";
const int PORT = 1883;

const char *TOPIC_DATA = "vn/demo/fire_7f29/data";
const char *TOPIC_CMD  = "vn/demo/fire_7f29/cmd";

// Nguong canh bao mo phong
const float GAS_LIMIT = 45.0f;
const float TEMP_LIMIT = 50.0f;

// Cau truc du lieu cam bien
struct Reading {
  float gas;
  float temp;
};

// Che do dieu khien
enum Mode : uint8_t {
  AUTO,
  FORCE_ON,
  FORCE_OFF
};

enum Device : uint8_t {
  FAN,
  PUMP
};

struct Command {
  Device device;
  Mode mode;
};

// Khoi tao Queue
QueueHandle_t dataQueue;
QueueHandle_t dataAlert;
QueueHandle_t cmdQueue;

// Khoi tao cam bien va MQTT
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient net;
PubSubClient mqtt(net);

volatile bool fanState = false;
volatile bool pumpState = false;

// Nhan lenh dieu khien tu Web qua MQTT
void onMqtt(char *topic, byte *payload,
            unsigned int length) {

  if (strcmp(topic, TOPIC_CMD) != 0) return;

  char msg[40];
  if (length >= sizeof(msg)) return;

  memcpy(msg, payload, length);
  msg[length] = '\0';

  Command c;

  if (strncmp(msg, "FAN_", 4) == 0) {
    c.device = FAN;
  }
  else if (strncmp(msg, "PUMP_", 5) == 0) {
    c.device = PUMP;
  }
  else return;

  if (strcmp(msg, "FAN_ON") == 0 ||
      strcmp(msg, "PUMP_ON") == 0) {
    c.mode = FORCE_ON;
  }
  else if (strcmp(msg, "FAN_OFF") == 0 ||
           strcmp(msg, "PUMP_OFF") == 0) {
    c.mode = FORCE_OFF;
  }
  else if (strcmp(msg, "FAN_AUTO") == 0 ||
           strcmp(msg, "PUMP_AUTO") == 0) {
    c.mode = AUTO;
  }
  else return;

  xQueueSend(cmdQueue, &c, 0);
}

// TASK 1: DOC DU LIEU - CORE 1
void taskRead(void *pvParameters) {

  Reading r;

  while (1) {

    int raw = analogRead(MQ2_PIN);

    // Phan tram tin hieu ADC, khong phai ppm
    r.gas = (raw / 4095.0f) * 100.0f;

    r.temp = dht.readTemperature();

    if (!isnan(r.temp)) {

      Serial.printf(
        "Gas: %.1f%% | Temperature: %.1f C\n",
        r.gas, r.temp
      );

      // Gui du lieu vao hai Queue
      xQueueOverwrite(dataQueue, &r);
      xQueueOverwrite(dataAlert, &r);

    } else {
      Serial.println("Loi doc DHT22!");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// TASK 2: CANH BAO - CORE 1
void taskAlert(void *pvParameters) {

  Reading r = {0, 0};

  Mode fanMode = AUTO;
  Mode pumpMode = AUTO;

  Command cmd;

  while (1) {

    // Xu ly lenh manual tu Web
    while (xQueueReceive(
        cmdQueue, &cmd, 0) == pdTRUE) {

      if (cmd.device == FAN) {
        fanMode = cmd.mode;
      } else {
        pumpMode = cmd.mode;
      }
    }

    // Doc Queue canh bao
    xQueueReceive(
      dataAlert, &r,
      pdMS_TO_TICKS(100)
    );

    // Dieu khien quat
    fanState =
      (fanMode == FORCE_ON) ||
      (fanMode == AUTO && r.gas > GAS_LIMIT);

    // Dieu khien may bom
    pumpState =
      (pumpMode == FORCE_ON) ||
      (pumpMode == AUTO && r.temp > TEMP_LIMIT);

    digitalWrite(
      FAN_PIN, fanState ? HIGH : LOW
    );

    digitalWrite(
      PUMP_PIN, pumpState ? HIGH : LOW
    );
  }
}

// TASK 3: TRUYEN DATA - CORE 0
void taskTransmit(void *pvParameters) {

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS, 6);

  mqtt.setServer(BROKER, PORT);
  mqtt.setCallback(onMqtt);
  mqtt.setBufferSize(256);

  uint32_t lastWifiTry = 0;
  uint32_t lastMqttTry = 0;

  Reading r;

  while (1) {

    // Kiem tra WiFi
    if (WiFi.status() != WL_CONNECTED) {

      if (millis() - lastWifiTry > 5000) {

        lastWifiTry = millis();

        WiFi.disconnect();
        WiFi.begin(SSID, PASS, 6);

        Serial.println("Connecting WiFi...");
      }

      vTaskDelay(pdMS_TO_TICKS(200));
      continue;
    }

    // Ket noi MQTT
    if (!mqtt.connected()) {

      if (millis() - lastMqttTry > 3000) {

        lastMqttTry = millis();

        String id = "wokwi-fire-" +
          String((uint32_t)ESP.getEfuseMac(), HEX);

        if (mqtt.connect(id.c_str())) {

          mqtt.subscribe(TOPIC_CMD);

          Serial.println("MQTT connected");

        } else {

          Serial.printf(
            "MQTT error: %d\n", mqtt.state()
          );
        }
      }

      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    mqtt.loop();

    // Doc du lieu tu dataQueue
    if (xQueueReceive(
        dataQueue, &r,
        pdMS_TO_TICKS(50)) == pdTRUE) {

      char out[160];

      snprintf(
        out, sizeof(out),
        "{\"gas\":%.1f,\"temperature\":%.1f,"
        "\"fan\":%s,\"pump\":%s}",
        r.gas,
        r.temp,
        fanState ? "true" : "false",
        pumpState ? "true" : "false"
      );

      // Gui len MQTT Broker
      mqtt.publish(TOPIC_DATA, out);

      Serial.printf(
        "Published: %s\n", out
      );
    }
  }
}

// HAM SETUP
void setup() {

  Serial.begin(115200);

  pinMode(FAN_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);

  digitalWrite(FAN_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);

  analogReadResolution(12);

  dht.begin();

  // Tao cac Queue
  dataQueue = xQueueCreate(
    1, sizeof(Reading)
  );

  dataAlert = xQueueCreate(
    1, sizeof(Reading)
  );

  cmdQueue = xQueueCreate(
    10, sizeof(Command)
  );

  if (!dataQueue || !dataAlert || !cmdQueue) {
    Serial.println("Queue allocation failed");
    while (1) delay(1000);
  }

  // Task truyen data - Core 0
  xTaskCreatePinnedToCore(
    taskTransmit,
    "Task truyen data",
    8192,
    NULL,
    1,
    NULL,
    0
  );

  // Task doc data - Core 1
  xTaskCreatePinnedToCore(
    taskRead,
    "Task doc data",
    4096,
    NULL,
    2,
    NULL,
    1
  );

  // Task canh bao - Core 1
  xTaskCreatePinnedToCore(
    taskAlert,
    "Task canh bao",
    4096,
    NULL,
    2,
    NULL,
    1
  );
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
