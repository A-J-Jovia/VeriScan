#include "Wire.h"
#include "Adafruit_AS7343.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// --- BLE UUIDS ---
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// --- PIN DEFINITIONS ---
#define SDA_PIN 33
#define SCL_PIN 32
#define LED_VIS_1 26  
#define LED_IR_1  27  
#define LED_VIS_2 18  
#define LED_IR_2  19  

Adafruit_AS7343 as7343 = Adafruit_AS7343();

// We pack the 18 channels into a struct. This takes exactly 36 bytes of RAM.
// No strings, no heap fragmentation.
#pragma pack(push, 1)
struct SensorPayload {
  uint16_t channels[18];
};
#pragma pack(pop)

SensorPayload payload;

// --- NON-BLOCKING TIMER ---
unsigned long lastScanTime = 0;
const unsigned long SCAN_INTERVAL = 2000; // 2 seconds

// --- BLE CONNECTION CALLBACKS ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("PHONE CONNECTED!");
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("PHONE DISCONNECTED!");
    }
};

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_VIS_1, OUTPUT); pinMode(LED_IR_1, OUTPUT);
  pinMode(LED_VIS_2, OUTPUT); pinMode(LED_IR_2, OUTPUT);
  
  // Ensure LEDs are OFF on boot
  turnOffLEDs();

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!as7343.begin(0x39, &Wire)) { 
    Serial.println("FATAL: Sensor not found!");
    while (1); // Halt
  }

  as7343.setGain(AS7343_GAIN_1024X); 
  as7343.setATIME(99);               
  as7343.setASTEP(599);              

  // --- START BLE SERVER ---
  BLEDevice::init("VeriScan_BLE");
  
  // Increase MTU so our 36-byte struct fits in one packet
  BLEDevice::setMTU(128); 
  
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
}

void loop() {
  // Handle Disconnection cleanly without blocking
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // Give the Bluetooth stack a moment to reset
      pServer->startAdvertising(); 
      Serial.println("Restarted Advertising...");
      oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }

  // Non-blocking timer for measurements
  if (deviceConnected && (millis() - lastScanTime >= SCAN_INTERVAL)) {
    lastScanTime = millis();
    performScientificScan();
  }
}

// --- HELPER FUNCTIONS ---

void turnOnLEDs() {
  digitalWrite(LED_VIS_1, HIGH); digitalWrite(LED_IR_1, HIGH);
  digitalWrite(LED_VIS_2, HIGH); digitalWrite(LED_IR_2, HIGH);
}

void turnOffLEDs() {
  digitalWrite(LED_VIS_1, LOW); digitalWrite(LED_IR_1, LOW);
  digitalWrite(LED_VIS_2, LOW); digitalWrite(LED_IR_2, LOW);
}

void performScientificScan() {
    uint16_t darkScan[18];
    uint16_t lightScan[18];

    // 1. READ DARK CURRENT (Ambient + Thermal Noise)
    // LEDs are OFF
    as7343.startMeasurement();
    while (!as7343.dataReady()) { delay(1); } // Library limitation, minor block
    as7343.readAllChannels(darkScan);

    // 2. TURN ON ILLUMINATION
    turnOnLEDs();
    delay(50); // Allow LED phosphor to stabilize

    // 3. READ SIGNAL (Pill Reflectance + Noise)
    as7343.startMeasurement();
    while (!as7343.dataReady()) { delay(1); }
    as7343.readAllChannels(lightScan);

    // 4. TURN OFF LEDs IMMEDIATELY (Prevent thermal buildup)
    turnOffLEDs();

    // 5. DIFFERENTIAL CALCULATION & PACKING
    for(int i = 0; i < 18; i++) {
      // Subtract dark noise. Prevent negative overflow if noise spiked.
      if (lightScan[i] > darkScan[i]) {
        payload.channels[i] = lightScan[i] - darkScan[i];
      } else {
        payload.channels[i] = 0; 
      }
    }

    // 6. TRANSMIT BINARY STRUCT OVER BLE
    // We send the memory address of the struct, and its exact size (36 bytes)
    pCharacteristic->setValue((uint8_t*)&payload, sizeof(payload));
    pCharacteristic->notify();
    
    Serial.println("Sent Differential Binary Data.");
}