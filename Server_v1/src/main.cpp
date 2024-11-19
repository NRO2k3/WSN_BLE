#include <Arduino.h>
#include <BLEDevice.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SERVICE_UUID          "e8dd76ff-f3e9-4b8d-a7b8-fceeb450734d"
#define CHARACTERISTIC_UUID_1 "b53df61e-a568-4d9e-8c2f-ade30942e056" 
#define CHARACTERISTIC_UUID_2 "7db9ed65-9c6d-486c-8870-9f6a726503ce"       

const char* ssid = "Hai San Bao Linh"; 
const char* password = "141887bl";   
const char* server = "https://demo.thingsboard.io";  
const char* token = "KNdbCnYNfz7b0Qthj2gy";        

BLEServer* pServer = nullptr;                        
BLECharacteristic* pCharacteristic_1 = nullptr;      
BLECharacteristic* pCharacteristic_2 = nullptr;   

bool deviceConnected = false;
float temperature;
int count = 0;
int numberClient = 2;
unsigned long currentTime = 0;
unsigned long previousTime = 0;
unsigned long currentTime1 = 0;
unsigned long previousTime1 = 0;
unsigned long currentTime2 = 0;
unsigned long previousTime2 = 0;
int node_id ;
int numberNode = 11;
String data[11];
String response;

class TemperatureCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string stdvalue = pCharacteristic->getValue();
    String value =  String(stdvalue.c_str());    
    if (value.length() > 0) {
      int firstSpace = value.indexOf(' ');
      node_id = (value.substring(0, firstSpace)).toInt();
      data[node_id-1] = value;
      Serial.print("Received temperature value: ");
      Serial.println(value);
    }
  }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      count++;
    };
    void onDisconnect(BLEServer* pServer) {
      count--;
      deviceConnected = false;
      BLEDevice::startAdvertising();
      Serial.println("Client disconnected, start advertising again.");
    }
};

void sendDataToThingsBoard(String node_id, String temp, String humd) {
  HTTPClient http;
  String url = String(server) + "/api/v1/" + token + "/telemetry";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"temp\":" + temp + ",\"humd\":" + humd + ",\"id\":" + node_id + "}";
  int httpResponseCode = http.POST(payload);
  Serial.print("Payload: ");
  Serial.println(payload);
  if (httpResponseCode > 0) {
    Serial.println("Dữ liệu gửi thành công!");
    Serial.println(http.getString());
  } else {
    Serial.print("Lỗi gửi dữ liệu. HTTP Response code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void getThresholdsFromThingsBoard() {
  HTTPClient http;
  String url = String(server) + "/api/v1/" + token + "/attributes?sharedKeys=thresholdTemp,thresholdHumd"; 
  Serial.println(url);
  http.begin(url);
  int httpResponseCode = http.GET();  
  if (httpResponseCode > 0) {
    response = http.getString();  
    Serial.println("Phản hồi từ ThingsBoard: " + response);
  } else {
    Serial.print("Lỗi lấy thuộc tính. HTTP Response code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void setup() 
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối với WiFi...");
  }
  Serial.println("Đã kết nối với WiFi");
  for(int i = 0 ;i < numberNode; i++){
    data[i]="";
  }
  BLEDevice::init("ESP32 GATEWAY");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic_1 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_1,
                      BLECharacteristic::PROPERTY_NOTIFY
                    ); 
  pCharacteristic_2 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_2,
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pCharacteristic_2->setCallbacks(new TemperatureCallbacks());  
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising(); 
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true); // Bật chế độ phản hồi khi quét
  pAdvertising->setMinPreferred(0x06); // Cấu hình quảng cáo đầy đủ
  pAdvertising->setMinPreferred(0x12); // Cấu hình thêm cho BLE
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() 
{
    if (deviceConnected) {
      currentTime1 = millis();
      if(currentTime1 - previousTime1 > 5000) {
        previousTime1 = currentTime1;
        getThresholdsFromThingsBoard();
        String thresholdSend="";
        int tempStart = response.indexOf("\"thresholdTemp\":") + 16; 
        int tempEnd = response.indexOf(",", tempStart);
        int humdStart = response.indexOf("\"thresholdHumd\":") + 16; 
        int humdEnd = response.indexOf("}", humdStart);
        thresholdSend = response.substring(tempStart,tempEnd) + " " + response.substring(humdStart,humdEnd);
        pCharacteristic_1->setValue(thresholdSend.c_str());
        pCharacteristic_1->notify();
      }
    }
    currentTime2 = millis();
    if(currentTime2 - previousTime2 > 10000) {
      previousTime2 = currentTime2;
      for(int i = 0 ;i < numberNode; i++){
        if(data[i]=="") {
          sendDataToThingsBoard(String(i+1),"disconected","disconected");
        } else {
          int startIndex = 0;
          int endIndex = data[i].indexOf(' ');
          String part1 = data[i].substring(startIndex, endIndex);
          startIndex = endIndex + 1;
          endIndex = data[i].indexOf(' ', startIndex);
          String part2 = data[i].substring(startIndex, endIndex);
          startIndex = endIndex + 1;
          String part3 = data[i].substring(startIndex);
          sendDataToThingsBoard(part1,part2,part3);
        }
      }
      for(int i = 0 ;i < numberNode; i++){
        data[i]="";
      }
      previousTime2 = millis();
    }
    if (count != numberClient) {
        currentTime = millis();
        if(currentTime - previousTime > 2000) {
          previousTime = currentTime;
          BLEDevice::startAdvertising();
          Serial.println("Advertising started.");
        }
    }
}


