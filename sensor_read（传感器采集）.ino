/*
 * 基于ESP32的校园微农园传感器数据采集程序
 *
 * 功能：采集DHT22温湿度、BH1750光照、土壤湿度、pH、MH-Z19B CO₂数据
 *       通过MQTT协议上传至智云平台
 *
 * 硬件：ESP32-WROOM-32 开发板
 *
 * 作者：（填写）
 * 日期：2026年6月
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <ArduinoJson.h>

// ==================== 引脚定义 ====================
#define DHTPIN          4       // DHT22 数据引脚
#define DHTTYPE         DHT22   // 传感器类型
#define SOIL_MOISTURE_PIN 36    // 土壤湿度传感器（ADC）
#define PH_PIN          39      // pH传感器（ADC）
#define RELAY_PIN       25      // 继电器控制引脚
#define CO2_RX          16      // MH-Z19B RX
#define CO2_TX          17      // MH-Z19B TX

// ==================== WiFi配置 ====================
const char* ssid = "YourWiFiSSID";         // WiFi名称
const char* password = "YourWiFiPassword"; // WiFi密码

// ==================== MQTT配置 ====================
const char* mqtt_server = "zhiyun-iot.com"; // MQTT服务器
const int mqtt_port = 1883;                 // MQTT端口
const char* mqtt_user = "device_01";        // 设备ID
const char* mqtt_password = "password";     // 设备密码
const char* topic_data = "sensor/node_01/data";      // 数据上传主题
const char* topic_control = "sensor/node_01/control"; // 控制指令主题
const char* topic_status = "sensor/node_01/status";   // 设备状态主题

// ==================== 采集参数 ====================
#define SAMPLE_INTERVAL     600000  // 采集间隔：10分钟（毫秒）
#define ALERT_INTERVAL      60000   // 异常时采集间隔：1分钟
#define UPLOAD_INTERVAL     600000  // 上传间隔：10分钟

// ==================== 阈值配置 ====================
#define TEMP_MIN            10.0    // 温度下限（℃）
#define TEMP_MAX            35.0    // 温度上限（℃）
#define HUMI_MIN            30.0    // 湿度下限（%RH）
#define HUMI_MAX            85.0    // 湿度上限（%RH）
#define SOIL_MOISTURE_MIN   20.0    // 土壤湿度下限（%）
#define SOIL_MOISTURE_MAX   80.0    // 土壤湿度上限（%）
#define PH_MIN              5.5     // pH下限
#define PH_MAX              8.0     // pH上限
#define CO2_MAX             1500.0  // CO₂上限（ppm）

// ==================== 全局对象 ====================
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;
WiFiClient espClient;
PubSubClient client(espClient);
HardwareSerial co2Serial(2);  // UART2用于CO₂传感器

// ==================== 全局变量 ====================
float temperature = 0;
float humidity = 0;
float soilMoisture = 0;
float lightIntensity = 0;
float phValue = 0;
int co2Concentration = 0;
bool isAlert = false;

unsigned long lastSampleTime = 0;
unsigned long lastUploadTime = 0;
unsigned long lastReconnectAttempt = 0;

// ==================== WiFi连接 ====================
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed!");
  }
}

// ==================== MQTT重连 ====================
void reconnect() {
  unsigned long now = millis();
  if (now - lastReconnectAttempt < 5000) {
    return;
  }
  lastReconnectAttempt = now;

  Serial.print("Attempting MQTT connection...");
  String clientId = "ESP32Client-" + String(random(0xffff), HEX);

  if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
    Serial.println("connected");
    client.subscribe(topic_control);
    Serial.println("Subscribed to: " + String(topic_control));
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" retrying in 5 seconds");
  }
}

// ==================== MQTT回调 ====================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // 处理控制指令
  if (String(topic) == topic_control) {
    if (message == "pump_on") {
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Pump ON");
      publishStatus("pump_running");
    } else if (message == "pump_off") {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Pump OFF");
      publishStatus("pump_stopped");
    } else if (message == "reset") {
      ESP.restart();
    }
  }
}

// ==================== 数据采集函数 ====================

// 读取DHT22温湿度
void readDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity = h;
  } else {
    Serial.println("DHT read failed!");
  }
}

// 读取土壤湿度
void readSoilMoisture() {
  int adcValue = analogRead(SOIL_MOISTURE_PIN);
  // ADC值转换为百分比（根据传感器校准调整）
  soilMoisture = map(adcValue, 0, 4095, 0, 100);
  soilMoisture = constrain(soilMoisture, 0, 100);
}

// 读取光照强度
void readLight() {
  float lux = lightMeter.readLightLevel();
  if (lux >= 0) {
    lightIntensity = lux;
  }
}

// 读取pH值
void readPH() {
  int adcValue = analogRead(PH_PIN);
  float voltage = adcValue / 4095.0 * 3.3;
  // pH校准公式（需根据实际传感器校准）
  phValue = 3.5 * voltage + 0.5;
  phValue = constrain(phValue, 0, 14);
}

// 读取CO₂浓度（MH-Z19B UART协议）
int readCO2() {
  byte cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  byte response[9];

  co2Serial.write(cmd, 9);
  delay(100);

  if (co2Serial.available() >= 9) {
    for (int i = 0; i < 9; i++) {
      response[i] = co2Serial.read();
    }

    if (response[0] == 0xFF && response[1] == 0x86) {
      int co2 = response[2] * 256 + response[3];
      return co2;
    }
  }
  return -1;  // 读取失败
}

// ==================== 滑动平均滤波 ====================
#define FILTER_SIZE 3
float tempBuffer[FILTER_SIZE] = {0};
float humiBuffer[FILTER_SIZE] = {0};
int filterIndex = 0;

float filterValue(float* buffer, float newValue) {
  buffer[filterIndex] = newValue;
  float sum = 0;
  for (int i = 0; i < FILTER_SIZE; i++) {
    sum += buffer[i];
  }
  return sum / FILTER_SIZE;
}

// ==================== 数据上传 ====================
void uploadData() {
  StaticJsonDocument<512> doc;

  doc["node_id"] = "node_01";
  doc["timestamp"] = millis() / 1000;

  JsonObject data = doc.createNestedObject("data");
  data["temperature"] = round(temperature * 10) / 10.0;
  data["humidity"] = round(humidity * 10) / 10.0;
  data["soil_moisture"] = round(soilMoisture * 10) / 10.0;
  data["light_intensity"] = (int)lightIntensity;
  data["ph"] = round(phValue * 10) / 10.0;
  data["co2"] = co2Concentration;

  doc["battery"] = 85;  // 电池电量（需要实际测量）
  doc["rssi"] = WiFi.RSSI();
  doc["is_alert"] = isAlert;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  if (client.publish(topic_data, jsonBuffer)) {
    Serial.println("Data uploaded successfully");
  } else {
    Serial.println("Data upload failed");
  }

  Serial.println(jsonBuffer);
}

// ==================== 状态上报 ====================
void publishStatus(const char* status) {
  StaticJsonDocument<128> doc;
  doc["node_id"] = "node_01";
  doc["status"] = status;
  doc["uptime"] = millis() / 1000;

  char jsonBuffer[128];
  serializeJson(doc, jsonBuffer);
  client.publish(topic_status, jsonBuffer);
}

// ==================== 预警检测 ====================
bool checkAlert() {
  bool alert = false;

  if (temperature < TEMP_MIN || temperature > TEMP_MAX) {
    Serial.println("ALERT: Temperature out of range!");
    alert = true;
  }
  if (humidity < HUMI_MIN || humidity > HUMI_MAX) {
    Serial.println("ALERT: Humidity out of range!");
    alert = true;
  }
  if (soilMoisture < SOIL_MOISTURE_MIN || soilMoisture > SOIL_MOISTURE_MAX) {
    Serial.println("ALERT: Soil moisture out of range!");
    alert = true;
  }
  if (phValue < PH_MIN || phValue > PH_MAX) {
    Serial.println("ALERT: pH out of range!");
    alert = true;
  }
  if (co2Concentration > CO2_MAX) {
    Serial.println("ALERT: CO2 concentration too high!");
    alert = true;
  }

  return alert;
}

// ==================== 自动灌溉逻辑 ====================
void autoIrrigation() {
  static bool pumpRunning = false;
  static unsigned long pumpStartTime = 0;

  if (soilMoisture < SOIL_MOISTURE_MIN && !pumpRunning) {
    // 土壤湿度低于阈值，启动灌溉
    digitalWrite(RELAY_PIN, HIGH);
    pumpRunning = true;
    pumpStartTime = millis();
    Serial.println("Auto irrigation started");
    publishStatus("auto_irrigation_start");
  }

  if (pumpRunning && (soilMoisture > 60.0 || millis() - pumpStartTime > 300000)) {
    // 湿度达标或超时5分钟，停止灌溉
    digitalWrite(RELAY_PIN, LOW);
    pumpRunning = false;
    Serial.println("Auto irrigation stopped");
    publishStatus("auto_irrigation_stop");
  }
}

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Campus Micro-Garden IoT System ===");

  // 初始化引脚
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // 初始化传感器
  dht.begin();
  Wire.begin();
  lightMeter.begin();
  co2Serial.begin(9600, SERIAL_8N1, CO2_RX, CO2_TX);

  // 连接WiFi
  setupWiFi();

  // 初始化MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  Serial.println("System initialized!");
  publishStatus("online");
}

// ==================== 主循环 ====================
void loop() {
  // 保持MQTT连接
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  // 定时采集数据
  unsigned long sampleInterval = isAlert ? ALERT_INTERVAL : SAMPLE_INTERVAL;
  if (now - lastSampleTime >= sampleInterval) {
    lastSampleTime = now;

    // 采集所有传感器数据
    readDHT();
    readSoilMoisture();
    readLight();
    readPH();

    int co2 = readCO2();
    if (co2 > 0) {
      co2Concentration = co2;
    }

    // 滑动平均滤波
    temperature = filterValue(tempBuffer, temperature);
    humidity = filterValue(humiBuffer, humidity);
    filterIndex = (filterIndex + 1) % FILTER_SIZE;

    // 检测预警
    isAlert = checkAlert();

    // 打印数据
    Serial.println("--- Sensor Data ---");
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" ℃");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %RH");
    Serial.print("Soil Moisture: "); Serial.print(soilMoisture); Serial.println(" %");
    Serial.print("Light: "); Serial.print(lightIntensity); Serial.println(" lux");
    Serial.print("pH: "); Serial.println(phValue);
    Serial.print("CO2: "); Serial.print(co2Concentration); Serial.println(" ppm");
    Serial.print("Alert: "); Serial.println(isAlert ? "YES" : "NO");
    Serial.println("-------------------");
  }

  // 定时上传数据
  if (now - lastUploadTime >= UPLOAD_INTERVAL) {
    lastUploadTime = now;
    uploadData();
  }

  // 自动灌溉逻辑
  autoIrrigation();

  // 检查WiFi连接
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    setupWiFi();
  }

  delay(100);  // 短暂延时，避免CPU占用过高
}
