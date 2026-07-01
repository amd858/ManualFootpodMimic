#include <Arduino.h> 
#include <BLEDevice.h> 
#include <BLEUtils.h> 
#include <BLEServer.h> 
#include <BLE2902.h> 
#include <math.h> 

#define RX_PIN 6 
#define TX_PIN 5 

int buttonState = 0; 
bool is_inst_stride_len_present = 1; 
bool is_total_distance_present = 1; 
bool is_running = 1; 
uint16_t inst_speed = 40; 
uint8_t inst_cadence = 1; 
uint16_t inst_stride_length = 1; 
uint32_t total_distance = 1000; 

float kmphinterval = .5; 
float minorkmphinterval = .1; 
int delayint = 500; 
const int CLK = 22; 
const int DIO = 23; 
volatile unsigned int counter; 
int rpm; 
volatile long lastRiseTime = 0; 
volatile long firstButtonPress = 0; 
float kmph; 
float mps; 
byte fakePos[1] = { 1 }; 
bool _BLEClientConnected = false; 

// Global buffer for non-blocking UART reading
String inputString = "";

#define RSCService BLEUUID((uint16_t)0x1814) 
BLECharacteristic RSCMeasurementCharacteristics(BLEUUID((uint16_t)0x2A53), BLECharacteristic::PROPERTY_NOTIFY); 
BLECharacteristic sensorPositionCharacteristic(BLEUUID((uint16_t)0x2A5D), BLECharacteristic::PROPERTY_READ); 
BLEDescriptor RSCDescriptor(BLEUUID((uint16_t)0x2901)); 
BLEDescriptor sensorPositionDescriptor(BLEUUID((uint16_t)0x2901)); 

class MyServerCallbacks : public BLEServerCallbacks { 
  void onConnect(BLEServer* pServer) { 
    _BLEClientConnected = true; 
  }; 
  void onDisconnect(BLEServer* pServer) { 
    _BLEClientConnected = false; 
    // Restart advertising when disconnected so devices can find it again
    pServer->getAdvertising()->start(); 
  } 
}; 

void InitBLE() { 
  BLEDevice::init("running_sensor"); 
  BLEServer* pServer = BLEDevice::createServer(); 
  pServer->setCallbacks(new MyServerCallbacks()); 
  BLEService* pRSC = pServer->createService(RSCService); 
  pRSC->addCharacteristic(&RSCMeasurementCharacteristics); 
  RSCDescriptor.setValue("Send all your RCSM rubbish here"); 
  RSCMeasurementCharacteristics.addDescriptor(&RSCDescriptor); 
  RSCMeasurementCharacteristics.addDescriptor(new BLE2902()); 
  pRSC->addCharacteristic(&sensorPositionCharacteristic); 
  pServer->getAdvertising()->addServiceUUID(RSCService); 
  pRSC->start(); 
  pServer->getAdvertising()->start(); 
} 

void setup() { 
  Serial.begin(115200); 
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); 
  Serial.print("started"); 
  InitBLE(); 
  kmph = 0; 
  inputString.reserve(30); // Reserve memory upfront to avoid fragmentation
  delay(2000); 
} 

void loop() { 
  // 1. NON-BLOCKING UART READING: Fixes the crash issue
  while (Serial1.available() > 0) { 
    char inChar = (char)Serial1.read(); 
    if (inChar == '\n') { 
      inputString.trim(); 
      if (inputString.length() > 0) { 
        float targetSpeed = inputString.toFloat(); 
        targetSpeed = targetSpeed * 0.125; 
        if (targetSpeed >= 0.0 && targetSpeed <= 40.0) { 
          kmph = targetSpeed; 
          Serial.print("UART Speed Updated to: "); 
          Serial.println(kmph); 
        } 
      } 
      inputString = ""; // Clear the string for next ingest
    } else if (inChar != '\r') { 
      inputString += inChar; // Buffer the character
    } 
  } 

  // 2. TIMED BLE SENDING
  if ((millis() - lastRiseTime) > 500) { 
    mps = kmph / 3.6; 
    inst_speed = mps * 256; 
    
    // Created data payload using standard uint8_t types
    uint8_t charArray[10] = { 
      3, 
      (uint8_t)inst_speed, 
      (uint8_t)(inst_speed >> 8), 
      (uint8_t)inst_cadence, 
      (uint8_t)inst_stride_length, 
      (uint8_t)(inst_stride_length >> 8), 
      (uint8_t)total_distance, 
      (uint8_t)(total_distance >> 8), 
      (uint8_t)(total_distance >> 16), 
      (uint8_t)(total_distance >> 24) 
    }; 

    // Only broadcast notification data if a client is connected to save processing
    if (_BLEClientConnected) {
      RSCMeasurementCharacteristics.setValue(charArray, 10); 
      RSCMeasurementCharacteristics.notify(); 
      sensorPositionCharacteristic.setValue(fakePos, 1); 
    }
    
    lastRiseTime = millis(); 
  } 
}
