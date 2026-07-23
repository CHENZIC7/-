/*
 * LoRa 模拟数据发送程序（演示版）
 *
 * 功能：生成模拟传感器数据，通过 LoRa 发送给网关
 * 适用：实验室演示，无需真实传感器
 *
 * 硬件：ESP32 + SX1278 (Ra-02)
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
 * 作者：陈子川
 * 日期：2026年6月
 */

#include <SPI.h>
#include <LoRa.h>
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

// ==================== 发送间隔 ====================
#define SEND_INTERVAL 10000  // 每10秒发送一次

// ==================== 模拟数据范围 ====================
#define TEMP_MIN    18.0
#define TEMP_MAX    32.0
#define HUMI_MIN    40.0
#define HUMI_MAX    80.0
#define SOIL_MIN    30.0
#define SOIL_MAX    75.0
#define LIGHT_MIN   500
#define LIGHT_MAX   6000
#define PH_MIN      5.5
#define PH_MAX      7.5
#define CO2_MIN     400
#define CO2_MAX     1200

// ==================== 全局变量 ====================
unsigned long lastSendTime = 0;
unsigned long packetCount = 0;

// 模拟数据状态（模拟真实变化）
float simTemp = 25.0;
float simHumi = 60.0;
float simSoil = 55.0;
int simLight = 2000;
float simPH = 6.5;
int simCO2 = 600;

// ==================== LoRa初始化 ====================
void setupLoRa() {
  Serial.println("Initializing LoRa...");

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  // 配置LoRa参数（与网关一致）
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setTxPower(LORA_TX_POWER);
  LoRa.enableCrc();

  Serial.println("LoRa initialized!");
  Serial.print("Frequency: "); Serial.print(LORA_FREQ / 1E6); Serial.println(" MHz");
  Serial.print("SF: "); Serial.println(LORA_SF);
  Serial.print("Bandwidth: "); Serial.print(LORA_BW / 1E3); Serial.println(" kHz");
  Serial.print("TX Power: "); Serial.print(LORA_TX_POWER); Serial.println(" dBm");
}

// ==================== 生成模拟数据 ====================
void generateSimData() {
  // 模拟自然波动（添加随机扰动）
  simTemp += random(-30, 31) / 100.0;  // ±0.3℃
  simHumi += random(-50, 51) / 100.0;  // ±0.5%
  simSoil += random(-20, 21) / 100.0;  // ±0.2%
  simLight += random(-100, 101);        // ±100 lux
  simPH += random(-5, 6) / 100.0;      // ±0.05
  simCO2 += random(-20, 21);            // ±20 ppm

  // 限制在合理范围内
  simTemp = constrain(simTemp, TEMP_MIN, TEMP_MAX);
  simHumi = constrain(simHumi, HUMI_MIN, HUMI_MAX);
  simSoil = constrain(simSoil, SOIL_MIN, SOIL_MAX);
  simLight = constrain(simLight, LIGHT_MIN, LIGHT_MAX);
  simPH = constrain(simPH, PH_MIN, PH_MAX);
  simCO2 = constrain(simCO2, CO2_MIN, CO2_MAX);

  // 模拟昼夜变化（基于运行时间）
  unsigned long hours = millis() / 3600000;
  float dayPhase = sin(hours * 3.14159 / 12);  // 24小时周期

  // 白天温度高、光照强，夜晚反之
  simTemp += dayPhase * 3.0;
  simLight += dayPhase * 2000;
  simCO2 -= dayPhase * 100;  // 白天植物光合作用消耗CO2

  // 再次限制范围
  simTemp = constrain(simTemp, TEMP_MIN, TEMP_MAX);
  simLight = constrain(simLight, LIGHT_MIN, LIGHT_MAX);
  simCO2 = constrain(simCO2, CO2_MIN, CO2_MAX);
}

// ==================== 发送数据包 ====================
void sendDataPacket() {
  // 生成模拟数据
  generateSimData();
  packetCount++;

  // 构建JSON数据包
  StaticJsonDocument<512> doc;

  doc["node_id"] = "node_01";
  doc["packet_id"] = packetCount;
  doc["timestamp"] = millis() / 1000;

  JsonObject data = doc.createNestedObject("data");
  data["temperature"] = round(simTemp * 10) / 10.0;
  data["humidity"] = round(simHumi * 10) / 10.0;
  data["soil_moisture"] = round(simSoil * 10) / 10.0;
  data["light_intensity"] = (int)simLight;
  data["ph"] = round(simPH * 10) / 10.0;
  data["co2"] = simCO2;

  doc["battery"] = random(70, 100);  // 模拟电池电量
  doc["is_alert"] = false;

  // 序列化JSON
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  // 发送LoRa数据包
  Serial.println("\n========================================");
  Serial.println("Sending LoRa Packet #" + String(packetCount));
  Serial.println("========================================");

  LoRa.beginPacket();
  LoRa.print(jsonBuffer);
  LoRa.endPacket();

  // 显示发送的数据
  Serial.println("Data sent:");
  Serial.println(jsonBuffer);
  Serial.println("----------------------------------------");
  Serial.print("Temperature: "); Serial.print(simTemp, 1); Serial.println(" ℃");
  Serial.print("Humidity: "); Serial.print(simHumi, 1); Serial.println(" %");
  Serial.print("Soil Moisture: "); Serial.print(simSoil, 1); Serial.println(" %");
  Serial.print("Light: "); Serial.print(simLight); Serial.println(" lux");
  Serial.print("pH: "); Serial.println(simPH, 1);
  Serial.print("CO2: "); Serial.print(simCO2); Serial.println(" ppm");
  Serial.print("Battery: "); Serial.print(doc["battery"].as<int>()); Serial.println(" %");
  Serial.println("========================================");
}

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   LoRa Sensor Node - Demo Version     ║");
  Serial.println("║   校园微农园智能监测系统 (演示版)       ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();

  // 初始化随机数种子
  randomSeed(analogRead(0));

  // 初始化LoRa
  setupLoRa();

  Serial.println();
  Serial.println("System initialized!");
  Serial.print("Send interval: "); Serial.print(SEND_INTERVAL / 1000); Serial.println(" seconds");
  Serial.println();
  Serial.println("Waiting to send first packet...");
}

// ==================== 主循环 ====================
void loop() {
  unsigned long now = millis();

  // 定时发送数据
  if (now - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = now;
    sendDataPacket();
  }

  // 显示等待进度
  static unsigned long lastDotTime = 0;
  if (now - lastDotTime >= 1000) {
    lastDotTime = now;
    Serial.print(".");
  }

  delay(100);
}
