#include "Wire.h"
#include "Adafruit_AS7343.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// --- BLE UUIDS (Send these to your Flutter Dev!) ---
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

// --- PIN DEFINITIONS ---
#define SDA_PIN 33
#define SCL_PIN 32
#define LED_VIS_1 26  
#define LED_IR_1  27  
#define LED_VIS_2 18  
#define LED_IR_2  19  

Adafruit_AS7343 as7343 = Adafruit_AS7343();

// --- BLE CONNECTION CALLBACKS ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("PHONE CONNECTED!");
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("PHONE DISCONNECTED! Restarting advertising...");
      BLEDevice::startAdvertising(); // Keep broadcasting so she can reconnect
    }
};

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  pinMode(LED_VIS_1, OUTPUT); pinMode(LED_IR_1, OUTPUT);
  pinMode(LED_VIS_2, OUTPUT); pinMode(LED_IR_2, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!as7343.begin(0x39, &Wire)) { 
    Serial.println("Error: Sensor not found!");
    while (1); 
  }

  as7343.setGain(AS7343_GAIN_1024X); 
  as7343.setATIME(99);               
  as7343.setASTEP(599);              

  // --- START BLE SERVER ---
  BLEDevice::init("VeriScan_BLE");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("\n--- BLE READY ---");
  Serial.println("Waiting for Flutter app to connect...");
}

void loop() {
  // Only flash and scan if the phone is actually connected!
  if (deviceConnected) {
    digitalWrite(LED_VIS_1, HIGH); digitalWrite(LED_IR_1, HIGH);
    digitalWrite(LED_VIS_2, HIGH); digitalWrite(LED_IR_2, HIGH);
    delay(50); 

    as7343.startMeasurement();
    while (!as7343.dataReady()) { delay(1); }

    uint16_t currentScan[18]; 
    as7343.readAllChannels(currentScan);

    digitalWrite(LED_VIS_1, LOW); digitalWrite(LED_IR_1, LOW);
    digitalWrite(LED_VIS_2, LOW); digitalWrite(LED_IR_2, LOW);

    // Format the 18 numbers into a string
    String payload = "";
    for(int i = 0; i < 18; i++) {
      payload += String(currentScan[i]);
      if(i < 17) payload += ","; 
    }

    // Send it to the Flutter App via BLE Notify
    pCharacteristic->setValue(payload.c_str());
    pCharacteristic->notify();
    
    Serial.print("Notified App: ");
    Serial.println(payload);
    
    delay(2000); 
  }
}