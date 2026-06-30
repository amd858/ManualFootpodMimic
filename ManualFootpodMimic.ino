#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <math.h>

#define RX_PIN 6
#define TX_PIN 5  // We won't use TX, but the function requires a pin assignment

// const int buttonPin[] = { 5, 18, 19 };  // the number of the pushbutton pins
//const int ledPin =  13;      // the number of the LED pin

// variables will change:
int buttonState = 0;                 // variable for reading the pushbutton status
bool is_inst_stride_len_present = 1; /**< True if Instantaneous Stride Length is present in the measurement. */
bool is_total_distance_present = 1;  /**< True if Total Distance is present in the measurement. */
bool is_running = 1;                 /**< True if running, False if walking. */
uint16_t inst_speed = 40;            /**< Instantaneous Speed. */
uint8_t inst_cadence = 1;            /**< Instantaneous Cadence. */
uint16_t inst_stride_length = 1;     /**< Instantaneous Stride Length. */
uint32_t total_distance = 1000;


float kmphinterval = .5;
float minorkmphinterval = .1;

int delayint = 500;

const int CLK = 22;  //Set the CLK pin connection to the display
const int DIO = 23;  //Set the DIO pin connection to the display

volatile unsigned int counter;
int rpm;

volatile long lastRiseTime = 0;  //Time at which pin2 (interrupt 0) goes from LOW to HIGH

volatile long firstButtonPress = 0;

float kmph;
float mps;

byte fakePos[1] = { 1 };

bool _BLEClientConnected = false;

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
  }
};

void InitBLE() {
  BLEDevice::init("FootpodMimic");
  // CBLE Server
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Do some BLE Setup
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
  // initialize the LED pin as an output:
  //pinMode(ledPin, OUTPUT);
  // initialize the Serial Monitor @ 9600
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  // initialize the keypad pin(s) as an input:
  // for (int x = 0; x < 3; x++) {
  //   pinMode(buttonPin[x], INPUT_PULLUP);
  // }
  Serial.print("started");

  InitBLE();

  kmph = 0;
  delay(2000);
}

void loop() {
  if (Serial1.available() > 0) {
    String inputString = Serial1.readStringUntil('\n');  // Read until newline
    inputString.trim();                                  // Remove any hidden spaces or carriage returns

    if (inputString.length() > 0) {
      float targetSpeed = inputString.toFloat();  // Convert text to a decimal number
      targetSpeed = targetSpeed * 0.125;
      
      if (targetSpeed >= 0.0 && targetSpeed <= 40.0) {  // Safety limits (0 to 60 km/h)
        kmph = targetSpeed;
        Serial.print("UART Speed Updated to: ");
        Serial.println(kmph);
      }
    }
  }
  if ((millis() - lastRiseTime) > 500) {
    //debouncing
    //some unrequired rubbish
    mps = kmph / 3.6;
    //get speed ble ready
    inst_speed = mps * 256;
    //inst_speed =rpm/210;
    //kmph=mps*3.6;

    //Create the bytearray to send to Zwift via BLE
    byte charArray[10] = {
      3,
      (unsigned byte)inst_speed,
      (unsigned byte)(inst_speed >> 8),
      (unsigned byte)inst_cadence,
      (unsigned byte)inst_stride_length,
      (unsigned byte)(inst_stride_length >> 8),
      (unsigned byte)total_distance,
      (unsigned byte)(total_distance >> 8),
      (unsigned byte)(total_distance >> 16),
      (unsigned byte)(total_distance >> 24)
    };

    RSCMeasurementCharacteristics.setValue(charArray, 10);
    RSCMeasurementCharacteristics.notify();
    sensorPositionCharacteristic.setValue(fakePos, 1);
    lastRiseTime = millis();
  }
}
