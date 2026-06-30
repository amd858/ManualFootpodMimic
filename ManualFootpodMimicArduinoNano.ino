#include <SoftwareSerial.h>

// Passing -1 as the RX pin disables it entirely.
// Syntax: SoftwareSerial(rxPin, txPin);
SoftwareSerial myTxSerial(-1, A0);

void setup() {
  // Start hardware serial for debugging if needed
  Serial.begin(115200);

  // Start your software serial transmission port
  myTxSerial.begin(115200);

  Serial.println("Transmit-only SoftwareSerial started on A0.");
}

void loop() {
  static int i = 0;
 
  Serial.println(++i);
  myTxSerial.println(i);
  if (i > 39) {
    i = 0;
  }
  delay(2000);  // Wait 2 seconds before sending again
}
