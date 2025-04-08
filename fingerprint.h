#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <Adafruit_Fingerprint.h>

#define RX_PIN 4 // Pin 16 for RX
#define TX_PIN 5 // Pin 17 for TX

extern HardwareSerial mySerial;
extern Adafruit_Fingerprint finger;

void setupFingerprint();
uint8_t getFingerprintID();

#endif // FINGERPRINT_H
