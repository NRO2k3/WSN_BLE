#include <Arduino.h>
#include <BLEDevice.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Tang 2

// #define SERVICE_UUID_1          "e8dd76ff-f3e9-4b8d-a7b8-fceeb450734d"
// #define CHARACTERISTIC_UUID_1_1 "b53df61e-a568-4d9e-8c2f-ade30942e056"
// #define CHARACTERISTIC_UUID_1_2 "7db9ed65-9c6d-486c-8870-9f6a726503ce"

// #define SERVICE_UUID_2          "67f13bdf-bf10-4312-878a-70108cb4c64c"
// #define CHARACTERISTIC_UUID_2_1 "69416cc5-b55e-477c-9d20-d7f516887a00"
// #define CHARACTERISTIC_UUID_2_2 "2d8c02fc-80ce-48b4-8a58-08bf6e3565d8"

// #define SERVICE_UUID_2          "65f11c92-db78-4d62-af27-e1509624dac2"
// #define CHARACTERISTIC_UUID_2_1 "79713aa0-e42d-49dd-bb25-44652dfbc6cf"
// #define CHARACTERISTIC_UUID_2_2 "572565de-dcd7-4ec0-b420-dfaab6254a96"

// Tang 3

#define SERVICE_UUID_1          "67f13bdf-bf10-4312-878a-70108cb4c64c"
#define CHARACTERISTIC_UUID_1_1 "69416cc5-b55e-477c-9d20-d7f516887a00"
#define CHARACTERISTIC_UUID_1_2 "2d8c02fc-80ce-48b4-8a58-08bf6e3565d8"

#define SERVICE_UUID_2          "c51aa79d-ffb9-4cd4-8034-91517c86a3f8"
#define CHARACTERISTIC_UUID_2_1 "5a4c43bb-17bf-4479-a4ad-64e0308b534b"
#define CHARACTERISTIC_UUID_2_2 "71a7dbc8-d68e-468c-b1f4-a4d1a084118c"

// #define SERVICE_UUID_1          "65f11c92-db78-4d62-af27-e1509624dac2"
// #define CHARACTERISTIC_UUID_1_1 "79713aa0-e42d-49dd-bb25-44652dfbc6cf"
// #define CHARACTERISTIC_UUID_1_2 "572565de-dcd7-4ec0-b420-dfaab6254a96"

// #define SERVICE_UUID_2          "3284d6d0-cc31-4fb1-9c3c-037c338bbbcc"
// #define CHARACTERISTIC_UUID_2_1 "1e8a0e5d-8e19-4f67-a1db-fbcd9b43b58a"
// #define CHARACTERISTIC_UUID_2_2 "3ffd4802-4463-4cae-bcc2-29bf027aa73d"

// #define SERVICE_UUID_2          "ac600a51-8df9-47fe-8849-f99fd96099f7"
// #define CHARACTERISTIC_UUID_2_1 "a5d76e09-b5fc-4073-9616-38a303e47048"
// #define CHARACTERISTIC_UUID_2_2 "eb237967-e21b-48f8-9844-09c25bc89da7"

#define LED_PIN_GREEN 25
#define LED_PIN_RED 27//27
#define LED_PIN_YELLOW 26//26
#define ONE_WIRE_BUS 22 // Chân kết nối cảm biến DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
const int moistureSensorPin = 34; 

BLEServer* pServer = nullptr;                        
BLECharacteristic* pCharacteristic_1 = nullptr;      
BLECharacteristic* pCharacteristic_2 = nullptr;

BLEClient* pClient = nullptr;
BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteCharacteristic_1 = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic_2 = nullptr;

bool doConnect = false; 
bool connected = false;
bool deviceConnected = false;
float temperatureSend;
int humiditySend;
int temperatureThreshold = 9999;
int humidityThreshold = 9999;
int count = 0;
int numberClient = 3;
unsigned long previousMillis = 0;  
unsigned long currentMillis = 0; 
unsigned long currentTime = 0;
unsigned long previousTime = 0;
unsigned long currentTime1 = 0;
unsigned long previousTime1 = 0;
int numberNode = 11;
String data[11];
std::string messageReceive="";
String node_id = "4";
String Thres="";

class TemperatureCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    messageReceive = pCharacteristic->getValue();
    String value =  String(messageReceive.c_str());       
    if (value.length() > 0) {
      int firstSpace = value.indexOf(' ');
      int node_id = (value.substring(0, firstSpace)).toInt();
      data[node_id-1] = value;
      Serial.print("Received temperature value: ");
      Serial.println(value);
    }
  }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      BLEDevice::startAdvertising();
      Serial.println("Client disconnected, start advertising again.");
    }
};

class MyClientCallback : public BLEClientCallbacks {
public:
    void onConnect(BLEClient* pclient){
        Serial.print("Connected");
    }
    void onDisconnect(BLEClient* pclient){
        connected = false;
        Serial.print("Disconnected !!!");
    }
};

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID_1))) {
        BLEDevice::getScan()->stop();
        if(myDevice != nullptr){
            delete myDevice;
            myDevice = nullptr;
        }
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
    } 
  } 
};

void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
    if (length > 0) {
        Thres = (char*)pData;
        int firstSpace = Thres.indexOf(' ');
        temperatureThreshold = (Thres.substring(0, firstSpace)).toFloat();
        int secondSpace = Thres.indexOf(' ', firstSpace);
        humidityThreshold = (Thres.substring(firstSpace + 1)).toInt();
        Serial.print("Ngưỡng nhận từ server: ");
        Serial.println(temperatureThreshold);
        Serial.println(humidityThreshold);
    } 
    else {
        Serial.println("Khong co du lieu nhan duoc.");
    }
}

bool connectToServer(){
    Serial.print("Attempt to connect: ");
    Serial.println(myDevice->getAddress().toString().c_str()); 
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    
    if (pClient->connect(myDevice)) {
        Serial.println("Connected to server");
        BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID_1));

        if (pRemoteService == nullptr) {
            Serial.print("Failed to find our service UUID: ");
            Serial.println(BLEUUID(SERVICE_UUID_1).toString().c_str());
            pClient->disconnect();
            delete myDevice;
            myDevice = nullptr;
            return false;
        }
        Serial.println("Found service");
        pRemoteCharacteristic_1 = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID_1_1);
        pRemoteCharacteristic_2 = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID_1_2);

        if (pRemoteCharacteristic_1 == nullptr || pRemoteCharacteristic_2 == nullptr) {
            connected = false;
            pClient->disconnect();
            delete myDevice;
            myDevice = nullptr;
            Serial.println("At least one characteristic UUID not found");
            return false;
        }

        if (pRemoteCharacteristic_1->canNotify()) {
        pRemoteCharacteristic_1->registerForNotify(notifyCallback);
        }
        connected = true;
    }
    return true;
}
 
void setup() 
{
    Serial.begin(115200);
    pinMode(LED_PIN_GREEN, OUTPUT);  
    digitalWrite(LED_PIN_GREEN, LOW);
    pinMode(LED_PIN_RED, OUTPUT);  
    digitalWrite(LED_PIN_RED, LOW);
    pinMode(LED_PIN_YELLOW, OUTPUT);  
    digitalWrite(LED_PIN_YELLOW, HIGH);
    pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
    sensors.begin();
    for(int i = 0 ;i < numberNode; i++) {
        data[i]="";
    }
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("ESP32 RELAY");

    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1500);
    pBLEScan->setWindow(500);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(10, false);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID_2);
    pCharacteristic_1 = pService->createCharacteristic(
                                CHARACTERISTIC_UUID_2_1,
                                BLECharacteristic::PROPERTY_NOTIFY
                                ); 
    pCharacteristic_2 = pService->createCharacteristic(
                                CHARACTERISTIC_UUID_2_2,
                                BLECharacteristic::PROPERTY_WRITE
                                );
    pCharacteristic_2->setCallbacks(new TemperatureCallbacks());  
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising(); 
    pAdvertising->addServiceUUID(SERVICE_UUID_2);
    pAdvertising->setScanResponse(true); // Bật chế độ phản hồi khi quét
    pAdvertising->setMinPreferred(0x06); // Cấu hình quảng cáo đầy đủ
    pAdvertising->setMinPreferred(0x12); // Cấu hình thêm cho BLE
} 

void loop() 
{
    if (doConnect){
        if (connectToServer()) {
            Serial.println("We are now connected to the BLE Server.");
        } else {
            Serial.println("We have failed to connect to the server; there is nothing more we will do.");
        }
        doConnect = false;
    }

    if (connected && pRemoteCharacteristic_2 && pRemoteCharacteristic_2->canWrite()) {
        currentMillis = millis();
        if (currentMillis - previousMillis >= 2000) {
            previousMillis = currentMillis;
            sensors.requestTemperatures();
            float temperature = sensors.getTempCByIndex(0);  
            Serial.println("Nhiệt độ: " + String(temperature));
            int moistureValue = analogRead(moistureSensorPin);  
            int moisturePercentage = 100 - (moistureValue / 4095.0) * 100.0;  
            temperatureSend = temperature;
            humiditySend = moisturePercentage;
            // temperatureSend = 20;
            // humiditySend = 15;
            String messageSend = node_id + " " + String(temperatureSend) + " " + String(humiditySend);
            Serial.println("Attempting to write value");
            Serial.println(messageSend);
            pRemoteCharacteristic_2->writeValue(messageSend.c_str(), messageSend.length());
            Serial.println("Write attempted.");
            for(int i = 0 ;i < numberNode; i++) {
                if(data[i].length()>0) {
                    Serial.println("Attempting to write value receive");
                    pRemoteCharacteristic_2->writeValue(data[i].c_str(), data[i].length());
                    Serial.println(data[i].c_str());
                    Serial.println("Write attempted.");
                    messageReceive = "";
                }
            }
            for(int i = 0 ;i < numberNode; i++) {
                data[i] = "";
            }
             
        }
    } else {
        Serial.println("Characteristic is not writable or is nullptr.");
    }

    if (temperatureSend > temperatureThreshold || humiditySend > humidityThreshold) {
        digitalWrite(LED_PIN_RED, HIGH);  
    } else {
        digitalWrite(LED_PIN_RED, LOW); 
    }

    if (temperatureSend < temperatureThreshold || humiditySend < humidityThreshold) {
        digitalWrite(LED_PIN_GREEN, HIGH);  
    } else {
        digitalWrite(LED_PIN_GREEN, LOW); 
    }

    if (!connected) {
        BLEDevice::getScan()->start(10, false);
    }

    if (deviceConnected) {
      currentTime1 = millis();
      if(currentTime1 - previousTime1 > 1000){
        previousTime1 = currentTime1;
        pCharacteristic_1->setValue(Thres.c_str());
        pCharacteristic_1->notify();
      }
    }

    count = pServer->getConnectedCount();

    if (count != numberClient) {
        Serial.println("NUMBER CONNECT: ");
        Serial.println(count);
        currentTime = millis();
        if(currentTime - previousTime > 5000) {
          previousTime = currentTime;
          BLEDevice::startAdvertising();
          Serial.println("Advertising started.");
        }
    }
    delay(1000);
}
