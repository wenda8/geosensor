#include <Arduino.h>
#include "DHT.h"
#include "LoRa_E22.h"
#include <OneWire.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <EEPROM.h>

//Node_id
const String nodeID = "18";

//DHT
#define DHTPIN 2         
#define DHTTYPE DHT11     
DHT dht(DHTPIN, DHTTYPE);

//Capacitive Soil Moisture Sensor
const int MOISTURE_SENSOR_PIN = A0; 
const int airValue = 478;   // 空气中的值，最小湿度（干燥）
const int waterValue = 192; // 水中的值，最大湿度（湿润）

//DS18B20
OneWire ds(10); 

//E22
int M0 = 7;
int M1 = 6;
int RX = 4;
int TX = 5;
int AUX = 3;
LoRa_E22 e22(TX, RX, AUX, M0, M1);

int currentAddress = 0;

// 收集数据
struct SensorData {
    float env_T; 
    float env_H; 
    float soil_T; 
    int soil_H; 
  };

SensorData collectData() {
    SensorData data;
    
    // 读取DHT11
    data.env_T = dht.readTemperature();
    data.env_H = dht.readHumidity();
    
    // 读取DS18B20
    ds.reset();
    ds.skip(); // 对于单个传感器，使用skip命令
    ds.write(0x44, 1); 
    delay(750); // 等待转换完成
    ds.reset();
    ds.skip();
    ds.write(0xBE); 
    byte dataBytes[9];
    for (byte i = 0; i < 9; i++) { // 读取9个字节的数据
      dataBytes[i] = ds.read();
    }
    int16_t raw = (dataBytes[1] << 8) | dataBytes[0];
    data.soil_T = (float)raw / 16.0;

    // 读取Capacitive Soil Moisture Sensor
    int sensorValue = analogRead(MOISTURE_SENSOR_PIN);
    data.soil_H = map(sensorValue, airValue, waterValue, 0, 100);
    data.soil_H = constrain(data.soil_H, 0, 100);

    return data; 
}


SensorData calculateAverage(SensorData *data, int count) {
  SensorData average = {0, 0, 0, 0}; // 初始化平均值结构体
  // 累加所有读数
  for (int i = 0; i < count; ++i) {
    average.env_T += data[i].env_T;
    average.env_H += data[i].env_H;
    average.soil_T += data[i].soil_T;
    average.soil_H += data[i].soil_H;
  }
  // 计算平均值
  average.env_T /= count;
  average.env_H /= count;
  average.soil_T /= count;
  average.soil_H /= count;
  return average;
}

void sleep(unsigned long sleepTime) {
  unsigned long startMillis = millis();  
  unsigned long currentMillis;

  set_sleep_mode(SLEEP_MODE_IDLE); 
  sleep_enable(); 

  do {
    sleep_mode(); 
    currentMillis = millis();
  } while(currentMillis - startMillis < sleepTime);

  sleep_disable(); 
}

SensorData calculateAndReportAverages(int x, int y) {
  SensorData dataPoints[x];
  SensorData dataSum = {0, 0, 0, 0};
  int dataCount = 0;

  while (dataCount < x) {
      SensorData newData = collectData();
      // 输出每次测量的结果
      String sensordata = "Measurement: " + String(dataCount + 1) +", env_T: " + String(newData.env_T) + "C, env_H: " + String(newData.env_H) + "%, soil_T: " + String(newData.soil_T) + "C, soil_H: " + String(newData.soil_H) + "%";
      String dataWithID = "nodeID: " + nodeID + ", " + sensordata + "\r\n";
      // Serial.print("Sending message: " + dataWithID);

      dataPoints[dataCount] = newData;
      
      dataSum.env_T += newData.env_T;
      dataSum.env_H += newData.env_H;
      dataSum.soil_T += newData.soil_T;
      dataSum.soil_H += newData.soil_H;

      dataCount++;
      sleep(y); // 睡眠y秒
  }

  // 计算平均值
  SensorData avgData = calculateAverage(dataPoints, dataCount);

  //输出平均值以进行检查
  Serial.print("Average Env T: ");
  Serial.println(avgData.env_T);
  Serial.print("Average Env H: ");
  Serial.println(avgData.env_H);
  Serial.print("Average Soil T: ");
  Serial.println(avgData.soil_T);
  Serial.print("Average Soil H: ");
  Serial.println(avgData.soil_H);

  return avgData;
}

void senddata(SensorData avgData)  {
  String dataWithID = "nodeID: " + nodeID +", env_T: " + String(avgData.env_T) + "C, env_H: " + String(avgData.env_H) + "%, soil_T: " + String(avgData.soil_T) + "C, soil_H: " + String(avgData.soil_H) + "%" + "\r\n";
  unsigned int dataLength = dataWithID.length() + 1; // 加1为字符串结束符'\0'
  char payload[dataLength];
  dataWithID.toCharArray(payload, dataLength);
  e22.sendMessage(payload);

  // // 定义包含地址、信道和数据的完整消息数组，加3是因为地址高、地址低、信道
  // unsigned int messageLength = dataLength + 3;
  // char message[messageLength];

  // // 设置地址和信道
  // message[0] = 0x00; // 地址高位
  // message[1] = 0x64; // 地址低位
  // message[2] = 0x04; // 信道

  // // 将数据复制到消息数组中
  // for (unsigned int i = 0; i < dataLength; i++) {
  //   message[i + 3] = payload[i];
  // }

  // // 发送消息
  // ResponseStatus rs = e22.sendMessage(message, messageLength);
}


void writeToEEPROM(uint8_t encodedData[], int &currentAddress) {
    const int EEPROM_SIZE = 1024;  // Arduino Uno 的 EEPROM 容量
    const int DATA_SIZE = 5; 
    // 写入数据
    for (int i = 0; i < DATA_SIZE; i++) {
        EEPROM.write(currentAddress + i, encodedData[i]);
    }
    // 更新下一次写入的地址
    currentAddress += DATA_SIZE;
    // 如果超出 EEPROM 容量，从头开始
    if (currentAddress + DATA_SIZE > EEPROM_SIZE) {
        currentAddress = 0;
    }
}

// 编码部分
// 编码函数
uint8_t encodeHumidity(int humidity) {
  return (uint8_t)(humidity * 63 / 100); // 0-100% -> 0-63
}

uint8_t encodeenvTemperature(float temperature, float minTemp, float maxTemp) {
  return (uint8_t)((temperature - minTemp) * 127 / (maxTemp - minTemp)); // Temp -> 0-127
}

uint8_t encodesoilTemperature(float temperature, float minTemp, float maxTemp) {
  return (uint8_t)((temperature - minTemp) * 255 / (maxTemp - minTemp)); 
}

// 时间编码到两个uint8_t中
void encodeTime(uint32_t seconds, uint8_t &highByte, uint8_t &lowByte) {
    uint16_t encodedTime = seconds / 443;
    highByte = (encodedTime >> 8) & 0x1F; // 高位字节
    lowByte = encodedTime & 0xFF;         // 低位字节
}

void encodeData(float envHumidity, float envTemperature, float soilHumidity, float soilTemperature, uint32_t seconds, uint8_t data[5]) {
    uint8_t highByte;
    uint8_t lowByte;
    encodeTime(seconds, highByte, lowByte); 
    data[0] = (encodeHumidity(envHumidity) << 2) | (encodeenvTemperature(envTemperature, -20, 60) >> 5);
    data[1] = (encodeenvTemperature(envTemperature, -20, 60) << 3) | (encodeHumidity(soilHumidity) >> 3);
    data[2] = (encodeHumidity(soilHumidity) << 5) | (encodesoilTemperature(soilTemperature, -55, 125) >> 3);
    data[3] = (encodesoilTemperature(soilTemperature, -55, 125) << 5) | (highByte);
    data[4] = lowByte;
}

//

int dataSum = 0;

void setup() {
    Serial.begin(9600);
    dht.begin();
    e22.begin();
    randomSeed(analogRead(0));
}

void loop() {
    int d = 0;
    SensorData averageData = calculateAndReportAverages(30, 2000);
    d += 1;
    int delayTime = 2000 + random(4000); 
    delay(delayTime);
    senddata(averageData);
    if (digitalRead(AUX) != HIGH) {
        d = 0;
      }else {
        SensorData savedata; 
        while (d <= 1){
          SensorData averageDatas[2];
          averageDatas[0] = averageData;
          SensorData averageData2 = calculateAndReportAverages(30, 2000);
          averageDatas[1] = averageData2;
          savedata = calculateAverage(averageDatas, 2);
          d += 1;
        }
        unsigned long currentMillis = millis(); 
        uint32_t currentSeconds = currentMillis / 1000; 
        uint8_t encodedData[5];
        uint8_t highByte, lowByte;
        
        encodeTime(currentSeconds, highByte, lowByte);
        encodeData(savedata.env_H, savedata.env_T, savedata.soil_H, savedata.soil_T, currentSeconds, encodedData);
        writeToEEPROM(encodedData, currentAddress);
        // 输出平均值以进行检查
        Serial.print("avgAverage Env T: ");
        Serial.println(savedata.env_T);
        Serial.print("avgAverage Env H: ");
        Serial.println(savedata.env_H);
        Serial.print("avgAverage Soil T: ");
        Serial.println(savedata.soil_T);
        Serial.print("avgAverage Soil H: ");
        Serial.println(savedata.soil_H);
        Serial.print("currentSeconds: ");
        Serial.println(currentSeconds);
        Serial.print("Encode: "); 
        for (int i = 0; i < 6; i++) {
          Serial.print(encodedData[i], HEX);
          Serial.print(" ");
        }
        Serial.println(" ");
      }

}