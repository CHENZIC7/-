/*
 * LoRa数据转发程序
 *
 * 功能：接收远端LoRa传感器节点的数据，通过WiFi转发至云平台
 *
 * 硬件：ESP32 + SX1278 (Ra-02) LoRa模块
 *
 * 引脚连接：
 *   ESP32    ->  SX1278
 *   GPIO5    ->  NSS (SS)
 *   GPIO14   ->  RST
 *   GPIO2    ->  DIO0
 *   GPIO18   ->  SCK
 *   GPIO23   ->  MOSI
 *   GPIO19   ->  MISO
 *   3.3V     ->  VCC
 *   GND      ->  GND
 *
 * 作者：（填写）
 * 日期：2026年6月
 */

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ==================== LoRa引脚定义 ====================
#define LORA_SS     5
#define LORA_RST    14
#define LORA_DIO0   2
#define LORA_FREQ   433E6   // 频率：433MHz

// ==================== LoRa参数配置 ====================
#define LORA_SF     9       // 扩频因子：SF9
#define LORA_BW     125E3   // 带宽：125kHz
#define LORA_CR     5       // 编码率：4/5
#define LORA_TX_POWER 17    // 发射功率：17dBm

// ==================== WiFi配置 ====================
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// ==================== MQTT配置 ====================
const char* mqtt_server = "zhiyun-iot.com";
const int mqtt_port = 1883;
const char* mqtt_user = "gateway_01";
const char* mqtt_password = "password";
const char* topic_gateway = "gateway/01/status";
const char* topic_data_prefix = "sensor/";

// ==================== 全局对象 ====================
WiFiClient espClient;
PubSubClient client(espClient);

// ==================== 统计变量 ====================
unsigned long packetReceived = 0;
unsigned long packetForwarded = 0;
unsigned long packetErrors = 0;

// ==================== WiFi连接 ====================
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected! IP: ");
  Serial.println(WiFi.localIP());
}

// ==================== MQTT重连 ====================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "LoRaGateway-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      publishGatewayStatus("online");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

// ==================== 网关状态上报 ====================
void publishGatewayStatus(const char* status) {
  StaticJsonDocument<256> doc;
  doc["gateway_id"] = "gateway_01";
  doc["status"] = status;
  doc["uptime"] = millis() / 1000;
  doc["rssi"] = WiFi.RSSI();
  doc["packets_received"] = packetReceived;
  doc["packets_forwarded"] = packetForwarded;
  doc["packets_errors"] = packetErrors;

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);
  client.publish(topic_gateway, jsonBuffer);
}

// ==================== LoRa初始化 ====================
void setupLoRa() {
  Serial.println("Initializing LoRa...");

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  // 配置LoRa参数
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setTxPower(LORA_TX_POWER);
  LoRa.enableCrc();

  Serial.println("LoRa initialized!");
  Serial.print("Frequency: "); Serial.print(LORA_FREQ / 1E6); Serial.println(" MHz");
  Serial.print("SF: "); Serial.println(LORA_SF);
  Serial.print("Bandwidth: "); Serial.print(LORA_BW / 1E3); Serial.println(" kHz");
}

// ==================== 数据转发 ====================
void forwardToCloud(String payload) {
  // 解析接收到的数据
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    packetErrors++;
    return;
  }

  // 获取节点ID
  const char* nodeId = doc["node_id"];
  if (nodeId == nullptr) {
    Serial.println("Missing node_id in payload");
    packetErrors++;
    return;
  }

  // 添加网关信息
  doc["gateway_id"] = "gateway_01";
  doc["forward_time"] = millis() / 1000;

  // 构建MQTT主题
  String topic = topic_data_prefix + String(nodeId) + "/data";

  // 序列化并发送
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  if (client.publish(topic.c_str(), jsonBuffer)) {
    packetForwarded++;
    Serial.print("Forwarded to ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(jsonBuffer);
  } else {
    packetErrors++;
    Serial.println("MQTT publish failed!");
  }
}

// ==================== 数据包验证 ====================
bool validatePacket(String packet) {
  // 基本格式验证
  if (packet.length() < 10 || packet.length() > 512) {
    Serial.println("Invalid packet length");
    return false;
  }

  // 验证JSON格式
  StaticJsonDocument<16> testDoc;
  DeserializationError error = deserializeJson(testDoc, packet);
  if (error) {
    Serial.println("Invalid JSON format");
    return false;
  }

  return true;
}

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== LoRa Gateway ===");

  // 初始化LoRa
  setupLoRa();

  // 连接WiFi
  setupWiFi();

  // 初始化MQTT
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Gateway initialized!");
}

// ==================== 主循环 ====================
void loop() {
  // 保持MQTT连接
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 检查LoRa数据包
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    packetReceived++;

    // 读取数据
    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    // 获取信号强度
    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    Serial.println("\n--- LoRa Packet Received ---");
    Serial.print("Size: "); Serial.print(packetSize); Serial.println(" bytes");
    Serial.print("RSSI: "); Serial.print(rssi); Serial.println(" dBm");
    Serial.print("SNR: "); Serial.print(snr); Serial.println(" dB");
    Serial.print("Data: "); Serial.println(received);

    // 验证数据包
    if (validatePacket(received)) {
      // 在JSON中添加信号信息
      StaticJsonDocument<512> doc;
      deserializeJson(doc, received);
      doc["lora_rssi"] = rssi;
      doc["lora_snr"] = snr;

      String enrichedPayload;
      serializeJson(doc, enrichedPayload);

      // 转发至云平台
      forwardToCloud(enrichedPayload);
    } else {
      packetErrors++;
      Serial.println("Packet validation failed, discarded");
    }
  }

  // 定时上报网关状态（每5分钟）
  static unsigned long lastStatusTime = 0;
  if (millis() - lastStatusTime >= 300000) {
    lastStatusTime = millis();
    publishGatewayStatus("running");
  }

  // 检查WiFi连接
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    setupWiFi();
  }

  delay(10);
}
