#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

const char* ssid     = "Was";
const char* password = "SO120305";

const char* mqtt_server = "broker.emqx.io";
const int   mqtt_port   = 1883;
const char* client_id   = "bendungan_pintar_esp32_v3";

const char* TOPIC_NILAI  = "bendungan/iot/nilai";
const char* TOPIC_STATUS = "bendungan/iot/statusair";
const char* TOPIC_BUZZER = "bendungan/iot/buzzer";
const char* TOPIC_SERVO  = "bendungan/iot/servo";
const char* TOPIC_MODE   = "bendungan/iot/mode";

const char* TOPIC_CONTROL    = "bendungan/iot/control";
const char* TOPIC_CMD_SERVO  = "bendungan/iot/cmd/servo";
const char* TOPIC_CMD_BUZZER = "bendungan/iot/cmd/buzzer";

#define WATER_SENSOR_PIN  34
#define SERVO_PIN         13
#define BUZZER_PIN        14

#define LEVEL_AMAN_MAX     800
#define LEVEL_WASPADA_MAX  1500

#define PUBLISH_INTERVAL  2000
#define BLINK_INTERVAL    300
#define SENSOR_CHANGE_THR 15

enum Mode { MODE_OTOMATIS, MODE_MANUAL };

Mode          currentMode      = MODE_OTOMATIS;
int           currentAngle     = 0;
bool          buzzerActive     = false;
bool          buzzerBlinkState = false;
unsigned long lastPublishTime  = 0;
unsigned long lastBlinkTime    = 0;

int    lastPublishedSensor = -999;
String lastPublishedStatus = "";
int    lastPublishedAngle  = -1;
bool   lastPublishedBuzzer = false;
Mode   lastPublishedMode   = MODE_OTOMATIS;
bool   firstPublish        = true;

unsigned long lastReconnectAttempt = 0;
int           reconnectRetryCount  = 0;
bool          mqttWasConnected     = false;

WiFiClient   espClient;
PubSubClient mqttClient(espClient);
Servo        gateServo;

int readWaterLevel() {
  long sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += analogRead(WATER_SENSOR_PIN);
    delay(2);
  }
  return (int)(sum / 5);
}

String getStatusLabel(int v) {
  if (v <= LEVEL_AMAN_MAX)    return "aman";
  if (v <= LEVEL_WASPADA_MAX) return "waspada";
  return "bahaya";
}

void setServo(int angle) {
  if (angle == currentAngle) return;
  gateServo.write(angle);
  currentAngle = angle;
  Serial.printf("[servo] angle set to %d deg\n", angle);
}

void setBuzzer(bool state) {
  if (state == buzzerActive) return;
  buzzerActive = state;
  if (!state) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerBlinkState = false;
  }
  Serial.printf("[buzzer] state set to %s\n", state ? "on" : "off");
}

void handleBuzzerBlink() {
  if (!buzzerActive || currentMode == MODE_MANUAL) return;
  unsigned long now = millis();
  if (now - lastBlinkTime >= BLINK_INTERVAL) {
    lastBlinkTime    = now;
    buzzerBlinkState = !buzzerBlinkState;
    digitalWrite(BUZZER_PIN, buzzerBlinkState ? HIGH : LOW);
  }
}

void handleAutoMode(int val) {
  if (val <= LEVEL_AMAN_MAX) {
    setServo(0);
    setBuzzer(false);
  } else if (val <= LEVEL_WASPADA_MAX) {
    setServo(90);
    setBuzzer(false);
  } else {
    setServo(180);
    setBuzzer(true);
  }
}

void publishSensorData(int val, const String& status) {
  if (!mqttClient.connected()) return;

  bool changed = false;

  if (firstPublish || abs(val - lastPublishedSensor) > SENSOR_CHANGE_THR) {
    if (mqttClient.publish(TOPIC_NILAI, String(val).c_str(), true)) {
      lastPublishedSensor = val;
      Serial.printf("[mqtt pub] nilai = %d\n", val);
      changed = true;
    }
  }

  if (firstPublish || status != lastPublishedStatus) {
    if (mqttClient.publish(TOPIC_STATUS, status.c_str(), true)) {
      lastPublishedStatus = status;
      Serial.printf("[mqtt pub] statusair = %s\n", status.c_str());
      changed = true;
    }
  }

  const char* modeStr = (currentMode == MODE_OTOMATIS) ? "AUTO" : "MANUAL";
  if (firstPublish || currentMode != lastPublishedMode) {
    if (mqttClient.publish(TOPIC_MODE, modeStr, true)) {
      lastPublishedMode = currentMode;
      Serial.printf("[mqtt pub] mode = %s\n", modeStr);
      changed = true;
    }
  }

  if (firstPublish || currentAngle != lastPublishedAngle) {
    if (mqttClient.publish(TOPIC_SERVO, String(currentAngle).c_str(), true)) {
      lastPublishedAngle = currentAngle;
      Serial.printf("[mqtt pub] servo = %d\n", currentAngle);
      changed = true;
    }
  }

  if (firstPublish || buzzerActive != lastPublishedBuzzer) {
    const char* buzzerStr = buzzerActive ? "ON" : "OFF";
    if (mqttClient.publish(TOPIC_BUZZER, buzzerStr, true)) {
      lastPublishedBuzzer = buzzerActive;
      Serial.printf("[mqtt pub] buzzer = %s\n", buzzerStr);
      changed = true;
    }
  }

  if (!changed && !firstPublish) {
    Serial.println("[mqtt] no changes, publish skipped");
  }

  firstPublish = false;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  Serial.printf("[mqtt in] %s = %s\n", topic, msg.c_str());

  String t = String(topic);

  if (t == TOPIC_CONTROL) {
    if (msg == "MANUAL" && currentMode != MODE_MANUAL) {
      currentMode = MODE_MANUAL;
      setServo(0);
      setBuzzer(false);
      digitalWrite(BUZZER_PIN, LOW);
      mqttClient.publish(TOPIC_MODE,   "MANUAL", true);
      mqttClient.publish(TOPIC_SERVO,  "0",      true);
      mqttClient.publish(TOPIC_BUZZER, "OFF",    true);
      lastPublishedMode   = MODE_MANUAL;
      lastPublishedAngle  = 0;
      lastPublishedBuzzer = false;
      Serial.println("[mode] switched to manual");

    } else if (msg == "AUTO" && currentMode != MODE_OTOMATIS) {
      currentMode = MODE_OTOMATIS;
      mqttClient.publish(TOPIC_MODE, "AUTO", true);
      lastPublishedMode = MODE_OTOMATIS;
      Serial.println("[mode] switched to auto");
    }

  } else if (t == TOPIC_CMD_SERVO) {
    if (currentMode != MODE_MANUAL) {
      Serial.println("[servo] command ignored, not in manual mode");
      return;
    }
    int angle = msg.toInt();
    if (angle == 0 || angle == 90 || angle == 180) {
      setServo(angle);
      mqttClient.publish(TOPIC_SERVO, String(angle).c_str(), true);
      lastPublishedAngle = angle;
      Serial.printf("[manual servo] angle set to %d deg\n", angle);
    }

  } else if (t == TOPIC_CMD_BUZZER) {
    if (currentMode != MODE_MANUAL) {
      Serial.println("[buzzer] command ignored, not in manual mode");
      return;
    }
    bool newState = (msg == "ON");
    if (newState != buzzerActive) {
      buzzerActive = newState;
      digitalWrite(BUZZER_PIN, newState ? HIGH : LOW);
      mqttClient.publish(TOPIC_BUZZER, newState ? "ON" : "OFF", true);
      lastPublishedBuzzer = newState;
      Serial.printf("[manual buzzer] state set to %s\n", newState ? "on" : "off");
    }
  }
}

void setupWifi() {
  Serial.printf("[wifi] connecting to %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++attempt > 120) {
      Serial.println("\n[wifi] connection timeout, restarting...");
      ESP.restart();
    }
  }
  Serial.printf("\n[wifi] connected\n");
  Serial.printf("  ip   : %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("  rssi : %d dBm\n", WiFi.RSSI());
}

void handleMqttReconnect() {
  if (mqttClient.connected()) {
    mqttWasConnected    = true;
    reconnectRetryCount = 0;
    return;
  }

  unsigned long now   = millis();
  unsigned long waitMs = min(3000UL * (1UL << min(reconnectRetryCount, 3)), 30000UL);

  if (now - lastReconnectAttempt < waitMs) return;

  lastReconnectAttempt = now;
  reconnectRetryCount++;

  Serial.printf("[mqtt] reconnect attempt #%d (backoff %lums)\n",
                reconnectRetryCount, waitMs);

  if (mqttClient.connect(client_id)) {
    reconnectRetryCount = 0;
    Serial.println("[mqtt] connected to broker");

    mqttClient.subscribe(TOPIC_CONTROL);
    mqttClient.subscribe(TOPIC_CMD_SERVO);
    mqttClient.subscribe(TOPIC_CMD_BUZZER);

    const char* modeNow = (currentMode == MODE_OTOMATIS) ? "AUTO" : "MANUAL";
    mqttClient.publish(TOPIC_MODE,   modeNow,                      true);
    mqttClient.publish(TOPIC_SERVO,  String(currentAngle).c_str(), true);
    mqttClient.publish(TOPIC_BUZZER, buzzerActive ? "ON" : "OFF",  true);

    firstPublish = true;

    Serial.printf("[mqtt] state synced: mode=%s servo=%d buzzer=%s\n",
                  modeNow, currentAngle, buzzerActive ? "on" : "off");
  } else {
    Serial.printf("[mqtt] connection failed (rc=%d), retry in %lus\n",
                  mqttClient.state(), waitMs / 1000 * 2);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n[system] bendungan pintar v3.0");
  Serial.printf("  chip : %s\n", ESP.getChipModel());
  Serial.printf("  heap : %d bytes\n", ESP.getFreeHeap());

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  int testADC = analogRead(WATER_SENSOR_PIN);
  Serial.printf("[adc test] pin %d = %d %s\n",
                WATER_SENSOR_PIN, testADC,
                testADC == 0 ? "(check sensor wiring)" : "(ok)");

  gateServo.attach(SERVO_PIN);
  gateServo.write(0);
  currentAngle = 0;
  Serial.println("[servo] initial position: 0 deg");

  digitalWrite(BUZZER_PIN, HIGH); delay(100);
  digitalWrite(BUZZER_PIN, LOW);  delay(100);
  digitalWrite(BUZZER_PIN, HIGH); delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  setupWifi();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(15);
  mqttClient.setBufferSize(512);

  Serial.println("[system] ready, mode: auto");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[wifi] disconnected, reconnecting...");
    WiFi.reconnect();
    delay(2000);
    return;
  }

  handleMqttReconnect();

  if (mqttClient.connected()) {
    mqttClient.loop();
  }

  handleBuzzerBlink();

  unsigned long now = millis();
  if (now - lastPublishTime >= PUBLISH_INTERVAL) {
    lastPublishTime = now;

    int    val    = readWaterLevel();
    String status = getStatusLabel(val);

    Serial.printf("[sensor] adc=%d | status=%s | mode=%s | rssi=%d dBm\n",
                  val, status.c_str(),
                  currentMode == MODE_OTOMATIS ? "auto" : "manual",
                  WiFi.RSSI());

    if (currentMode == MODE_OTOMATIS) {
      handleAutoMode(val);
    }

    if (mqttClient.connected()) {
      publishSensorData(val, status);
    } else {
      Serial.println("[mqtt] publish skipped, not connected");
    }
  }
}