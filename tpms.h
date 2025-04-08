#ifndef TPMS_H
#define TPMS_H

#include <BLEDevice.h>

// Function prototypes
String retmanData(String txt, int shift);
byte retByte(String Data,int start);
long returnData(String Data,int start);
int returnBatt(String Data);
int returnAlarm(String Data);

#endif 