#include <Arduino.h>
#include "DHT.h"
#include "LoRa_E22.h"
#include <OneWire.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <EEPROM.h>
#include <avr/wdt.h>


//Node_id
const String nodeID = "18";

// 全局变量来跟踪工作模式
bool sendMode = false;

int lastReadAddress = 0;     
int lastWrittenAddress = 0;  
const int EEPROM_SIZE = 1024;
const int DATA_SIZE = 5;

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
int address = 0;

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
  SensorData average = {0, 0, 0, 0}; 
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

// void sleep(unsigned long sleepTime) {
//   unsigned long startMillis = millis();  // 记录当前时间
//   unsigned long currentMillis;

//   set_sleep_mode(SLEEP_MODE_IDLE); // 设置睡眠模式为闲置模式
//   sleep_enable(); // 启用睡眠模式

//   // 循环直到达到设定的睡眠时间
//   //Serial.println("Entering sleep mode");
//   do {
//     sleep_mode(); // 进入睡眠模式
//     currentMillis = millis();
//   } while(currentMillis - startMillis < sleepTime);

//   sleep_disable(); // 禁用睡眠模式
//   //Serial.println("Exiting sleep mode");
// }


void sleep(unsigned long sleepTime) {
  const unsigned long maxSleepTime = 8000; // 单次最大睡眠时间，单位毫秒
  unsigned long remainingTime = sleepTime;
  unsigned long startMillis;
  unsigned long currentMillis;
  bool get18Received = false; // 用于标记是否收到GET18数据

  while (remainingTime > 0 && !get18Received) {
    // 每次睡眠最大 maxSleepTime 毫秒
    unsigned long sleepDuration = min(remainingTime, maxSleepTime);
    startMillis = millis();
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();

    while (millis() - startMillis < sleepDuration) {
      // 每8秒检查一次是否收到GET18数据
      unsigned long checkIntervalStart = millis();
      while (millis() - checkIntervalStart < 8000) {
        sleep_mode(); // 进入睡眠模式
        if (receiveMessage()) { // 检查是否收到GET18数据
          get18Received = true;
          break;
        }
      }
      if (get18Received) {
        break;
      }
    }

    sleep_disable();

    // 输出每段睡眠的时间
    Serial.print("Slept for ");
    Serial.print(sleepDuration);
    Serial.println(" milliseconds");

    remainingTime -= sleepDuration;
  }

  if (get18Received) {
    Serial.println("Received GET18 data, stopping sleep.");
  } else {
    Serial.println("Completed sleep without receiving GET18 data.");
  }
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
      //delay(y);
  }


  // 计算平均值
  SensorData avgData = calculateAverage(dataPoints, dataCount);

  //输出平均值以进行检查
  // Serial.print("Average Env T: ");
  // Serial.println(avgData.env_T);
  // Serial.print("Average Env H: ");
  // Serial.println(avgData.env_H);
  // Serial.print("Average Soil T: ");
  // Serial.println(avgData.soil_T);
  // Serial.print("Average Soil H: ");
  // Serial.println(avgData.soil_H);

  return avgData;
}

void writeToEEPROM(uint8_t encodedData[]) {
    // 写入数据
    for (int i = 0; i < DATA_SIZE; i++) {
        EEPROM.write(lastWrittenAddress + i, encodedData[i]);
    }
    // 更新下一次写入的地址
    lastWrittenAddress += DATA_SIZE;
    // 如果超出 EEPROM 容量，从头开始
    if (lastWrittenAddress + DATA_SIZE > EEPROM_SIZE) {
        lastWrittenAddress = 0;
    }
}

void readFromEEPROM() {
    int startAddress = lastReadAddress;
    int endAddress = lastWrittenAddress;

    Serial.print("Start Address: ");
    Serial.println(startAddress);
    Serial.print("End Address: ");
    Serial.println(endAddress);

    if (startAddress <= endAddress) {
        for (int addr = startAddress; addr < endAddress; addr += DATA_SIZE) {
            uint8_t data[DATA_SIZE];
            for (int i = 0; i < DATA_SIZE; ++i) {
                data[i] = EEPROM.read(addr + i);
            }
            float envHumidity, envTemperature, soilHumidity, soilTemperature;
            uint32_t seconds;
            decodeData(data, envHumidity, envTemperature, soilHumidity, soilTemperature, seconds);
            sendSensorData(envHumidity, envTemperature, soilHumidity, soilTemperature, seconds);
        }
    } else {
        for (int addr = startAddress; addr < EEPROM_SIZE; addr += DATA_SIZE) {
            uint8_t data[DATA_SIZE];
            for (int i = 0; i < DATA_SIZE; ++i) {
                data[i] = EEPROM.read(addr + i);
            }
            float envHumidity, envTemperature, soilHumidity, soilTemperature;
            uint32_t seconds;
            decodeData(data, envHumidity, envTemperature, soilHumidity, soilTemperature, seconds);
            sendSensorData(envHumidity, envTemperature, soilHumidity, soilTemperature, seconds);
        }
        for (int addr = 0; addr < endAddress; addr += DATA_SIZE) {
            uint8_t data[DATA_SIZE];
            for (int i = 0; i < DATA_SIZE; ++i) {
                data[i] = EEPROM.read(addr + i);
            }
            float envHumidity, envTemperature, soilHumidity, soilTemperature;
            uint32_t seconds;
            decodeData(data, envHumidity, envTemperature, soilHumidity, soilTemperature, seconds);
            sendSensorData(envHumidity, envTemperature, soilHumidity, soilTemperature, seconds);
        }
    }
    //lastReadAddress = endAddress;

    // 调试信息
    Serial.print("Updated Last Read Address: ");
    Serial.println(lastReadAddress);
}

void sendSensorData(float envHumidity, float envTemperature, float soilHumidity, float soilTemperature, uint32_t seconds) {
    String message = "Env_T: " + String(envTemperature) + "C, Env_H: " + String(envHumidity) +
                     "%, Soil_T: " + String(soilTemperature) + "C, Soil_H: " + String(soilHumidity) +
                     "%, Time: " + String(seconds) + "s \r\n";
    e22.sendMessage(message);
}

// 编码部分
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

//解码部分
float decodeHumidity(uint8_t data) {
  return data * 100 / 63.0;
}

float decodeEnvTemperature(uint8_t data, float minTemp, float maxTemp) {
  return data * (maxTemp - minTemp) / 127.0 + minTemp;
}

float decodeSoilTemperature(uint8_t data, float minTemp, float maxTemp) {
  return data * (maxTemp - minTemp) / 255.0 + minTemp;
}

uint32_t decodeTime(uint8_t highByte, uint8_t lowByte) {
  uint16_t encodedTime = (((uint16_t)highByte & 0x1F) << 8) | lowByte;
  return encodedTime;
}

void decodeData(uint8_t data[5], float &envHumidity, float &envTemperature, float &soilHumidity, float &soilTemperature, uint32_t &seconds) {
  envHumidity = decodeHumidity(data[0] >> 2);
  uint8_t envTempPart1 = (data[0] & 0x03) << 5;
  uint8_t envTempPart2 = (data[1] >> 3) & 0x1F;
  envTemperature = decodeEnvTemperature(envTempPart1 | envTempPart2, -20, 60);
  uint8_t soilHumidityPart1 = (data[1] & 0x07) << 3;
  uint8_t soilHumidityPart2 = (data[2] >> 5) & 0x07;
  soilHumidity = decodeHumidity(soilHumidityPart1 | soilHumidityPart2);
  uint8_t soilTempPart1 = (data[2] & 0x1F) << 3;
  uint8_t soilTempPart2 = (data[3] >> 5) & 0x07;
  soilTemperature = decodeSoilTemperature(soilTempPart1 | soilTempPart2, -55, 125);
  uint8_t highByte = data[3] & 0x1F;
  uint8_t lowByte = data[4];
  seconds = decodeTime(highByte, lowByte)*443;
}

// 接收消息并处理的函数
// void receiveMessage() {
//     if (e22.available()) {
//         ResponseContainer rsc = e22.receiveMessage();
//         if (rsc.status.code == SUCCESS) {
//             String receivedMessage = rsc.data;
//             Serial.print("Received message: ");
//             Serial.println(receivedMessage);

//             // 检查是否收到特定命令来切换模式
//             if (receivedMessage.equalsIgnoreCase("GET18")) {
//                 // 切换到发送模式
//                 sendMode = true;
//               }else { 
//                 sendMode = false;
//                 Serial.println("Received message != GET18");
//             }
//         }
//     }
// }

bool receiveMessage() {
    if (e22.available()) {
        ResponseContainer rsc = e22.receiveMessage();
        if (rsc.status.code == SUCCESS) {
            String receivedMessage = rsc.data;
            Serial.print("Received message: ");
            Serial.println(receivedMessage);

            // 检查是否收到特定命令来切换模式
            if (receivedMessage.equalsIgnoreCase("GET18")) {
                // 切换到发送模式
                sendMode = true;
                return true; // 返回true表示收到GET18消息
            } else {
                sendMode = false;
                Serial.println("Received message != GET18");
            }
        }
    }
    return false; // 返回false表示未收到GET18消息
}


void setup() {
    Serial.begin(9600);
    dht.begin();
    e22.begin();
    randomSeed(analogRead(0));

    lastReadAddress = 0;
    lastWrittenAddress = 0;
}

void loop() {
    // 接收并处理消息
    receiveMessage();
    const int DATA_SIZE = 5;
    const int EEPROM_SIZE = 1024;
    

    //在发送模式下执行数据收集和发送
    if (sendMode) {

        readFromEEPROM();
        sendMode = false;
    }else {
      sleep(3592000); // 睡眠 20000 毫秒（20 秒）
      SensorData averageData = calculateAndReportAverages(1, 8000);
      unsigned long currentMillis = millis(); // 获取当前毫秒数
      uint32_t currentSeconds = currentMillis / 1000; // 将毫秒数转换为秒
      uint8_t encodedData[5];
      uint8_t highByte, lowByte;
      
      encodeTime(currentSeconds, highByte, lowByte);
      encodeData(averageData.env_H, averageData.env_T, averageData.soil_H, averageData.soil_T, currentSeconds, encodedData);
      writeToEEPROM(encodedData);
      // 输出平均值以进行检查
      Serial.print("Average Env T: ");
      Serial.println(averageData.env_T);
      Serial.print("Average Env H: ");
      Serial.println(averageData.env_H);
      Serial.print("Average Soil T: ");
      Serial.println(averageData.soil_T);
      Serial.print("Average Soil H: ");
      Serial.println(averageData.soil_H);
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

